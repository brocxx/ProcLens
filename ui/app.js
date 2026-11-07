/* ProcLens presentation layer. All scheduling and metrics come from the C API.
 * Sorting, chart coordinates and playback are visual transformations only. */
const pages = [
  ['overview','Overview','◈'],['processes','Processes','≡'],['tree','Process tree','⑂'],
  ['simulator','Simulator','⊞'],['gantt','Gantt timeline','▥'],['compare','Compare','▤'],
  ['aging','Aging & starvation','◷'],['adaptive','Adaptive & advisor','✧'],
  ['log','Decision log','☷'],['about','Export & about','↗']
];
const policies = ['FCFS','SJF','SRTF','Round Robin','Priority','Priority with Aging'];
const descriptions = ['Arrival order · non-preemptive','Shortest burst · non-preemptive','Shortest remaining · preemptive','Equal time slices · preemptive','Importance first · preemptive','Waiting-time priority boosts'];
const colors = ['#82e3bf','#8bb9f0','#d5b4f3','#f1c68a','#ed9daa','#8ed6df','#cadb8a','#b3b9f0'];
const $ = (selector, node=document) => node.querySelector(selector);
const $$ = (selector, node=document) => [...node.querySelectorAll(selector)];
const esc = value => String(value ?? '').replace(/[&<>"']/g, char=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[char]));
const fmt = (value, digits=1) => Number.isFinite(Number(value)) && value !== null && value !== undefined ? Number(value).toLocaleString(undefined,{maximumFractionDigits:digits}) : '—';
const color = pid => colors[Math.abs(Number(pid)) % colors.length];
let state = {observed:[],jobs:[],runs:[],groups:[],recommendation:null,validation:null,mode:'empty'};
let connected=false, busy=false, error='', active='overview', view='observed', filter='', sort='pid', cursor=0, playing=false, frame=0, chosen=0, second=1, secondarySelected=false;
let config={quantum:2,context:0,interval:4,amount:1,algorithms:[0,1,2,3,4,5]};
let sweep=null, advice=null, toastTimer;
const content=$('#content');

function notify(message){$('#toast').textContent=message;$('#toast').classList.add('show');clearTimeout(toastTimer);toastTimer=setTimeout(()=>$('#toast').classList.remove('show'),5000);}
async function api(path, method='GET'){
  const response=await fetch(path,{method,headers:{Accept:'application/json'},cache:'no-store'});
  const data=await response.json().catch(()=>({error:'The engine returned an invalid response.'}));
  if(!response.ok || data.error) throw new Error(typeof data.error==='string'?data.error:`Request failed (${response.status}).`);
  return data;
}
function normalize(data){
  state={...state,...data};
  for(const key of ['observed','jobs','runs','groups']) if(!Array.isArray(state[key])) state[key]=[];
  state.runs=state.runs.map(run=>({...run,...run.result,name:run.name,events:run.events??run.result?.events??[],metrics:run.metrics??run.result?.metrics??[],decisions:run.decisions??run.result?.decisions??[]}));
  if(state.runs.length){chosen=Math.min(chosen,state.runs.length-1);if(!secondarySelected&&state.runs.length>1)second=1;else second=Math.min(second,state.runs.length-1);}
}
async function refresh(){
  try{normalize(await api('/api/state'));connected=true;error='';}catch(e){connected=false;error=e.message;}
  $('#connection').textContent=connected?'C engine connected':'Engine unavailable';
  $('#connection-dot').classList.toggle('offline',!connected);
  $('#mode').textContent=state.mode==='capture'?'OBSERVED WORKLOAD':state.mode==='demo'?'SYNTHETIC DEMO':'NO WORKLOAD';
  render();
}
async function action(path,success){
  if(busy)return;
  busy=true;error='';render();
  try{const data=await api(path,'POST');await refresh();if(success)notify(success);return data;}
  catch(e){error=e.message;notify(e.message);}
  finally{busy=false;render();}
}
const heading=(eyebrow,title,subtitle,buttons='')=>`<div class="page-heading"><div><div class="eyebrow">${eyebrow}</div><h1>${title}</h1><p>${subtitle}</p></div><div class="actions">${buttons}</div></div>`;
const button=(id,label,primary=false)=>`<button id="${id}" class="${primary?'primary':''}" ${busy?'disabled':''}>${label}</button>`;
const empty=(title,text,buttonHTML='')=>`<div class="empty"><div class="empty-symbol">◇</div><strong>${title}</strong><p>${text}</p>${buttonHTML}</div>`;
const notice=()=>`<div class="notice"><span>ⓘ</span><div><strong>A model, not a recording.</strong> One simulated CPU. We never control, pause, kill or reprioritize real processes. Bursts are estimates from sampled CPU time; captures represent a limited observation window.${state.omitted_jobs>0?` ${fmt(state.omitted_jobs,0)} low-activity or out-of-bounds process estimates were omitted to keep the simulated workload bounded.`:''}</div></div>`;
const stat=(label,value,unit,note)=>`<div class="card"><div class="stat-label">${label}<span>↗</span></div><div class="stat-value"><span data-count>${esc(value)}</span> <small>${unit}</small></div><div class="stat-note">${note}</div></div>`;
const runOptions=selected=>state.runs.map((run,i)=>`<option value="${i}" ${i===selected?'selected':''}>${esc(run.name)}</option>`).join('');
const metrics=(run)=>`<div class="grid stats">${stat('AVERAGE WAITING',fmt(run?.average_waiting),'ms','Time spent ready, not running')}${stat('AVERAGE RESPONSE',fmt(run?.average_response),'ms','Arrival to first CPU dispatch')}${stat('CPU UTILIZATION',run?fmt(run.utilization*100):'—','%','Useful execution / elapsed time')}${stat('CONTEXT SWITCHES',fmt(run?.context_switches,0),'','Changes between different jobs')}</div>`;

function overview(){
  return heading('YOUR SCHEDULING LABORATORY','A closer look at what runs.','Capture a workload. Explore the trade-offs. Understand every decision.',button('demo','Load demo')+button('capture',busy?'Sampling…':'◎ Capture processes',true))+
  `<section class="hero"><div><div class="eyebrow">REAL OBSERVATIONS. NEW PERSPECTIVES.</div><h2>One workload.<br>Every scheduling possibility.</h2><p>Take your processes out of the black box. Replay them on a single simulated CPU and see how each policy changes the outcome.</p><div class="actions"><a class="button primary" href="#simulator">Open simulator <span>↗</span></a><a class="button quiet" href="#about">How it works →</a></div></div><div class="chip-art" aria-hidden="true"><div class="chip"><div>⌘<small>PROCLENS</small></div></div></div></section>`+
  `<div class="grid stats">${stat('SIMULATION JOBS',fmt(state.jobs.length,0),'','Estimated or synthetic workload')}${stat('OBSERVED RECORDS',fmt(state.observed.length,0),'','Read-only process metadata')}${stat('POLICIES COMPARED',fmt(state.runs.length,0),'','Independent C simulations')}${stat('EXECUTION MODEL','1','CPU','Deterministic, millisecond ticks')}</div>`+
  `<div class="grid two"><section class="card"><div class="section-head"><div><h2>Experiment snapshot</h2><p>Your latest scheduling results</p></div><a class="button quiet" href="#compare">Explore →</a></div>${state.runs.length?bars('average_waiting','ms'):empty('Your first experiment starts here','Load the built-in demo or capture your processes, then run the simulator.')}</section><section class="card"><h2>From observation to understanding</h2><div class="steps">${[['Capture','Read process metadata without changing your system.'],['Model','Separate observed CPU time from estimated jobs.'],['Compare','Inspect policy-specific metrics and decision logs.']].map(([a,b],i)=>`<div class="step"><span class="step-number">0${i+1}</span><div><h3>${a}</h3><p>${b}</p></div></div>`).join('')}</div></section></div>`+notice();
}
function processes(){
  const isObserved=view==='observed';
  const rows=(isObserved?state.observed:state.jobs).filter(p=>`${p.pid} ${p.name??''}`.toLowerCase().includes(filter.toLowerCase()));
  rows.sort((a,b)=>sort==='name'?String(a.name??'').localeCompare(String(b.name??'')):Number(a[sort]??0)-Number(b[sort]??0));
  const headers=isObserved?['PID','Parent','Process','CPU time · ms','Memory · KiB','Raw priority','Sample · ms','Start time']:['PID','Parent','Process','Arrival · ms','Burst estimate · ms','Remaining · ms','Priority'];
  return heading('WORKLOAD EXPLORER','Processes','Observed records and simulated jobs are deliberately kept separate.',button('capture','◎ Capture',true))+
    `<div class="tabs"><button data-view="observed" class="${isObserved?'active':''}">Observed data · ${state.observed.length}</button><button data-view="simulated" class="${!isObserved?'active':''}">Simulated jobs · ${state.jobs.length}</button></div><div class="section-head"><input id="process-filter" type="search" aria-label="Search process name or PID" placeholder="Search process name or PID…" value="${esc(filter)}"><label>Sort <select id="process-sort"><option value="pid">PID</option><option value="name" ${sort==='name'?'selected':''}>Name</option><option value="${isObserved?'cpu_time_ms':'burst_tick'}" ${sort==='cpu_time_ms'||sort==='burst_tick'?'selected':''}>${isObserved?'CPU time':'Burst'}</option></select></label></div>`+
    (rows.length?`<div class="table-wrap"><table><thead><tr>${headers.map(h=>`<th>${h}</th>`).join('')}</tr></thead><tbody>${rows.slice(0,500).map(p=>`<tr><td class="mono">${esc(p.pid)}</td><td class="mono">${esc(p.ppid??0)}</td><td>${esc(p.name||`Process ${p.pid}`)}</td>${isObserved?`<td>${fmt(p.cpu_time_ms)}<div class="micro"><span style="width:${Math.min(100,Number(p.cpu_time_ms)/Math.max(1,...rows.map(r=>Number(r.cpu_time_ms)))*100)}%"></span></div></td><td>${p.memory_kb?fmt(p.memory_kb):'Unavailable'}</td><td>${esc(p.priority_raw)}</td><td>${fmt(p.timestamp_ms)}</td><td class="mono">${esc(p.start_time??'Unavailable')}</td>`:`<td>${fmt(p.arrival_tick)}</td><td>${fmt(p.burst_tick)}</td><td>${fmt(p.remaining_tick)}</td><td>${esc(p.priority)}</td>`}</tr>`).join('')}</tbody></table></div><p class="stat-note">${rows.length>500?'Showing the first 500 matches. Refine search to inspect more.':`${rows.length} records shown.`}</p>`:empty('No matching records',isObserved?'Capture processes to inspect observations. The synthetic demo contains simulated jobs only.':'Load a workload to inspect estimated jobs.'))+notice();
}
function tree(){
  const records=state.observed.length?state.observed:state.jobs;
  const byId=new Map();records.forEach(p=>byId.set(p.pid,p));
  const children=new Map();byId.forEach(p=>{const parent=byId.has(p.ppid)&&p.ppid!==p.pid?p.ppid:0;if(!children.has(parent))children.set(parent,[]);children.get(parent).push(p);});
  const visited=new Set();
  function node(p,depth=0){if(visited.has(p.pid)||depth>40)return '';visited.add(p.pid);const memory=p.memory_available===false||!p.memory_kb?'Memory unavailable':`${fmt(p.memory_kb)} KiB`;return `<details open><summary>${esc(p.name||`Process ${p.pid}`)} <span class="meta">PID ${esc(p.pid)} · ${memory}</span></summary>${(children.get(p.pid)||[]).map(c=>node(c,depth+1)).join('')}</details>`;}
  let forest=(children.get(0)||[]).map(p=>node(p)).join('');byId.forEach(p=>{if(!visited.has(p.pid))forest+=node(p);});
  return heading('PROCESS RELATIONSHIPS','See the bigger picture.','Explore parent-child relationships and group resource totals.')+`<div class="grid two"><section class="card tree"><div class="section-head"><h2>Process hierarchy</h2><button id="collapse-tree" class="quiet">Collapse all</button></div>${forest||empty('No process hierarchy','Capture a workload or load demo jobs.')}</section><section class="card"><h2>Group roll-ups</h2><p>Backend-computed totals for the current observation.</p>${state.groups.length?state.groups.map(g=>`<div class="feature"><span>Root PID ${esc(g.root_pid)}</span><span>${fmt(g.cpu_time_ms)} ms · ${g.memory_available===false||!g.memory_kb?'Memory unavailable':`${fmt(g.memory_kb)} KiB`}</span></div>`).join(''):empty('No observed groups','Synthetic jobs do not imply measured CPU or memory use.')}</section></div>`;
}
function slider(id,label,min,max,help){return `<div class="control"><label for="${id}">${label}<output id="${id}-value">${config[id]}${id==='amount'?'':' ms'}</output></label><input id="${id}" type="range" min="${min}" max="${max}" value="${config[id]}"><small>${help}</small></div>`;}
function simulator(){
  return heading('CONTROL ROOM','Design your experiment.','Every selected policy receives the same workload and configuration.',button('run','▷ Run simulation',true))+
    `<div class="notice">${state.jobs.length?`${state.jobs.length} jobs ready · ${esc(state.mode)} workload`:'No workload loaded yet.'} <button class="quiet" id="demo" ${busy?'disabled':''}>Load a synthetic demo →</button></div><div class="section-head"><h2>Choose your policies</h2><span class="stat-note">Select one or compare several</span></div><div class="grid algorithm-grid">${policies.map((name,i)=>`<label class="algorithm"><input type="checkbox" data-policy="${i}" ${config.algorithms.includes(i)?'checked':''}><h3>${name}</h3><small>${descriptions[i]}</small></label>`).join('')}</div><section class="card"><div class="section-head"><div><h2>Simulation parameters</h2><p>1 tick = 1 millisecond · smaller priority number = higher priority</p></div><span class="badge">SINGLE CPU</span></div><div class="controls">${slider('quantum','Round Robin quantum',1,50,'Smaller slices can increase context-switch overhead.')}${slider('context','Context-switch cost',0,10,'Charged only on transitions between different jobs.')}${slider('interval','Aging interval',1,50,'Ready-queue waiting interval before a priority boost.')}${slider('amount','Aging improvement',0,9,'Zero disables aging; effective priority is clamped at 0.')}</div></section>`+notice();
}
function timeline(run,id='timeline'){
  if(!run?.events?.length)return empty('No execution timeline','Run a simulation to inspect its Gantt chart.');
  const events=run.events, end=Math.max(1,...events.map(e=>Number(e.end_tick))), pids=[...new Set(events.filter(e=>e.kind===1).map(e=>e.pid))];
  const labels=[...pids,'IDLE','CS'], width=900, left=85, plot=790, height=labels.length*34+45;
  const rows=labels.map((pid,i)=>`<text x="4" y="${i*34+23}">${typeof pid==='number'?'P'+esc(pid):pid}</text><line x1="${left}" y1="${i*34+33}" x2="885" y2="${i*34+33}" stroke="var(--border)"/>`).join('');
  const slices=events.map(e=>{const label=e.kind===1?e.pid:e.kind===2?'IDLE':'CS',row=labels.indexOf(label),x=left+e.start_tick/end*plot,w=Math.max(.8,(e.end_tick-e.start_tick)/end*plot);return `<rect x="${x}" y="${row*34+8}" width="${w}" height="20" rx="3" fill="${e.kind===1?color(e.pid):e.kind===2?'var(--border)':'var(--warn)'}"><title>${esc(label)} · ${e.start_tick}–${e.end_tick} ms</title></rect>`;}).join('');
  const ticks=Array.from({length:6},(_,i)=>`<text x="${left+i*plot/5}" y="${height-4}" text-anchor="middle">${fmt(i*end/5)} ms</text>`).join('');
  return `<div class="timeline-wrap"><svg id="${id}" class="timeline" viewBox="0 0 ${width} ${height}" role="img" aria-label="${esc(run.name)} simulated Gantt chart">${rows}${slices}<line class="cursor-line" x1="${left+Math.min(cursor,end)/end*plot}" x2="${left+Math.min(cursor,end)/end*plot}" y1="0" y2="${height-25}"/>${ticks}</svg></div><div class="legend">${pids.map(pid=>`<span><i style="background:${color(pid)}"></i>PID ${esc(pid)}</span>`).join('')}<span><i style="background:var(--warn)"></i>Context switch</span></div>`;
}
function gantt(){
  const run=state.runs[chosen],end=run?.makespan??Math.max(1,...(run?.events||[]).map(e=>e.end_tick));
  return heading('EXECUTION REPLAY','Every slice tells a story.','Play back the simulated schedule. Compare two policies on one workload.')+
    (state.runs.length?`<div class="section-head"><label>Primary policy <select id="gantt-primary">${runOptions(chosen)}</select></label><div class="actions">${button('play',playing?'Ⅱ Pause':'▷ Play')}<output id="cursor-label">${fmt(cursor)} ms</output></div></div><label for="scrub" class="stat-note">Simulation time</label><input id="scrub" type="range" min="0" max="${end}" step="0.1" value="${Math.min(cursor,end)}"><div class="grid two"><section class="card"><h2>${esc(run.name)}</h2>${timeline(run)}</section><section class="card"><label>Compare with <select id="gantt-secondary">${runOptions(second)}</select></label>${timeline(state.runs[second],'timeline-second')}</section></div><div class="spacer">${metrics(run)}</div>`:empty('No timeline yet','Select policies and run the simulator.',`<a class="button primary" href="#simulator">Open simulator →</a>`));
}
function bars(metric,unit){
  const max=Math.max(1,...state.runs.map(r=>Number(r[metric])||0));
  return `<div class="bars">${state.runs.map(r=>`<div class="bar-row"><span>${esc(r.name)}</span><div class="bar-track"><div class="bar-fill" style="width:${Math.max(0,Number(r[metric])/max*100)}%"></div></div><output>${fmt(r[metric])} ${unit}</output></div>`).join('')}</div>`;
}
function radar(){
  const keys=['average_waiting','average_response','average_turnaround','context_switches','utilization'];
  const labels=['Wait','Response','Turnaround','Switches','Utilization'];
  const point=(i,v)=>[160+Math.sin(i*2*Math.PI/5)*85*v,130-Math.cos(i*2*Math.PI/5)*85*v];
  const grid=[.25,.5,.75,1].map(v=>`<polygon points="${keys.map((_,i)=>point(i,v).join(',')).join(' ')}" fill="none" stroke="var(--border)"/>`).join('');
  const polygons=state.runs.slice(0,6).map((run,index)=>{
    const points=keys.map((key,i)=>point(i,Number(run[key])/Math.max(.00001,...state.runs.map(other=>Number(other[key])))));
    return `<polygon points="${points.map(p=>p.join(',')).join(' ')}" fill="${colors[index]}15" stroke="${colors[index]}" stroke-width="1.5"/>`;
  }).join('');
  return `<svg class="chart-svg" viewBox="0 0 320 270" role="img" aria-label="Metric values normalized to the largest value among selected policies">${grid}${polygons}${labels.map((label,i)=>{const [x,y]=point(i,1.3);return `<text x="${x}" y="${y}" text-anchor="middle">${label}</text>`;}).join('')}</svg><div class="legend">${state.runs.slice(0,6).map((r,i)=>`<span><i style="background:${colors[i]}"></i>${esc(r.name)}</span>`).join('')}</div><p class="stat-note">Values normalized by each metric’s maximum. Smaller is better for wait, response, turnaround and switches; larger for utilization. Area is not an overall score.</p>`;
}
function compare(){
  return heading('POLICY COMPARISON','Trade-offs, made visible.','No best-overall crown. Inspect each objective separately.',`<a class="button" href="/api/export?format=csv" download>↓ Export CSV</a>`)+
    (state.runs.length?`<div class="grid two"><section class="card"><h2>Average waiting time</h2><p class="stat-note">Lower is better · milliseconds</p>${bars('average_waiting','ms')}</section><section class="card"><h2>Metric profiles</h2>${radar()}</section></div><div class="table-wrap spacer"><table><thead><tr><th>Policy</th><th>Wait · ms</th><th>Response · ms</th><th>Turnaround · ms</th><th>Fairness</th><th>Switches</th></tr></thead><tbody>${state.runs.map(r=>`<tr><td>${esc(r.name)}</td>${['average_waiting','average_response','average_turnaround','fairness','context_switches'].map(k=>`<td>${fmt(r[k],k==='fairness'?3:1)} ${state.runs.length>1 && Number(r[k])===(k==='fairness'?Math.max:Math.min)(...state.runs.map(x=>Number(x[k])))?'<span class="badge">BEST</span>':''}</td>`).join('')}</tr>`).join('')}</tbody></table></div><div class="grid two spacer"><section class="card"><h2>Quantum vs. switching</h2><p>Run the C engine at several quantum values, with the current context-switch cost.</p>${button('sweep','Run quantum sweep')}${sweep?curve(sweep):'<div class="callout">No sweep has been run. This chart will use actual simulated results.</div>'}</section><section class="card"><h2>Read the trade-offs</h2><div class="steps"><p>FCFS can leave short jobs behind a long one. SJF favors short bursts but depends on estimates. SRTF can preempt long jobs as shorter ones arrive.</p><p>Round Robin spreads first CPU access, at the cost of additional switches. Priority expresses importance; aging can improve long waits. These effects depend on workload and settings.</p></div></section></div>`:empty('No comparison available','Run at least two policies to explore their trade-offs.',`<a class="button primary" href="#simulator">Choose policies →</a>`));
}
function curve(data){
  const points=Array.isArray(data)?data:(data.points??data.sweep??[]);
  if(!points.length)return empty('No sweep points','The engine has not returned sweep data.');
  const maxQ=Math.max(...points.map(p=>p.quantum)),maxC=Math.max(1,...points.map(p=>p.context_switches));
  return `<svg class="chart-svg spacer" viewBox="0 0 420 180" role="img" aria-label="Quantum versus context switches"><path d="M35 10V150H405" fill="none" stroke="var(--border)"/><polyline points="${points.map(p=>`${35+p.quantum/maxQ*360},${150-p.context_switches/maxC*125}`).join(' ')}" fill="none" stroke="var(--accent)" stroke-width="2"/>${points.map(p=>`<circle cx="${35+p.quantum/maxQ*360}" cy="${150-p.context_switches/maxC*125}" r="4" fill="var(--accent)"><title>q=${p.quantum} ms · ${p.context_switches} switches</title></circle>`).join('')}<text x="35" y="175">Quantum (ms)</text><text x="2" y="20">${maxC}</text><text x="390" y="175">${maxQ}</text></svg>`;
}
function aging(){
  const plain=state.runs.find(r=>r.name==='Priority'),aged=state.runs.find(r=>/aging/i.test(r.name));
  return heading('FAIRNESS UNDER PRESSURE','Who is still waiting?','Compare the same priority workload with and without aging.')+
  (plain&&aged?`<div class="grid two">${[plain,aged].map(r=>`<section class="card"><h2>${esc(r.name)}</h2><div class="stat-value ${r.starved_count?'warning':'good'}">${fmt(r.starved_count,0)} <small>jobs above wait threshold</small></div><p>Maximum waiting: ${fmt(r.max_waiting)} ms</p>${timeline(r,`aging-${r===plain?'before':'after'}`)}<div class="table-wrap spacer"><table><thead><tr><th>PID</th><th>Wait · ms</th><th>Turnaround · ms</th></tr></thead><tbody>${r.metrics.map(m=>`<tr><td class="mono">${esc(m.pid)}</td><td>${fmt(m.waiting_tick)}</td><td>${fmt(m.turnaround_tick)}</td></tr>`).join('')}</tbody></table></div></section>`).join('')}</div><div class="notice">${aged.starved_count<plain.starved_count?'Aging reduced the number of jobs above the wait threshold.':aged.starved_count>plain.starved_count?'More jobs exceeded the wait threshold with these aging settings.':'The number of jobs above the wait threshold did not change.'} This finite-run threshold is a warning, not proof of infinite starvation.</div>`:empty('Compare both priority variants','Select Priority and Priority with Aging in the simulator, then run.',`<a class="button primary" href="#simulator">Configure comparison →</a>`));
}
function adaptive(){
  const r=state.recommendation,features=r?.features;
  return heading('EXPLAINABLE ASSISTANCE','A recommendation, with reasons.','Rule-based guidance and modeled what-if experiments.',button('recommend','Analyze workload',true))+
  `<div class="grid two"><section class="card recommendation"><div class="eyebrow">SUGGESTED POLICY</div>${r?`<h2>${esc(r.name??r.policy_name??policies[r.policy]??r.policy)}</h2><p>${esc(r.explanation)}</p><span class="badge">RULE-BASED · NOT AN OPTIMUM GUARANTEE</span>`:empty('Waiting for analysis','Load a workload, then ask the C engine for a recommendation.')}</section><section class="card"><h2>What drove this decision?</h2>${features?Object.entries(features).map(([k,v])=>`<div class="feature"><span>${esc(k.replaceAll('_',' '))}</span><span class="mono">${fmt(v,3)}</span></div>`).join(''):empty('No features yet','Features come directly from the backend analyzer.')}</section></div>`+
  `<div class="grid two spacer"><section class="card"><h2>Validation, not a promise</h2><p>Measure the heuristic against candidate policies on seeded workloads.</p>${button('validate','Run 500-workload validation')}<div class="callout">${state.validation?`<pre>${esc(JSON.stringify(state.validation,null,2))}</pre>`:'No validation run recorded. No accuracy percentage is claimed.'}</div></section><section class="card"><h2>Priority what-if</h2><p>Model a priority change. No operating-system settings are changed.</p><div class="actions"><label>Target PID <select id="advisor-pid">${state.jobs.map(j=>`<option value="${j.pid}">PID ${esc(j.pid)}</option>`).join('')}</select></label><label>New priority <input id="advisor-priority" type="number" min="0" max="9" value="9"></label>${button('advisor','Model change')}</div>${advice?`<div class="callout"><strong>MODELED RESULT</strong><pre>${esc(JSON.stringify(advice,null,2))}</pre></div>`:'<div class="callout">Smaller numbers mean higher priority. This backend currently models a single job; group-wide modeling is not claimed.</div>'}</section></div>`;
}
function log(){
  const run=state.runs[chosen];
  const entries=(run?.decisions??[]).filter(d=>String(typeof d==='string'?d:d.message).toLowerCase().includes(filter.toLowerCase()));
  return heading('DECISION TRACE','Understand every dispatch.','Read the C scheduler’s recorded decisions, linked to simulation time.')+
    `<div class="section-head"><label>Policy <select id="log-policy">${runOptions(chosen)}</select></label><input id="log-filter" type="search" aria-label="Filter decision log" placeholder="Filter by PID or reason…" value="${esc(filter)}"></div>`+
    (run?`<section class="card">${timeline(run,'log-timeline')}<label for="scrub">Time cursor · <output id="cursor-label">${fmt(cursor)} ms</output></label><input id="scrub" type="range" min="0" max="${run.makespan??Math.max(1,...run.events.map(e=>e.end_tick))}" value="${cursor}" step="1"><div class="log-scroll">${entries.length?entries.map(d=>{const text=typeof d==='string'?d:d.message,time=Number(text?.match(/Time (\d+)/)?.[1]??0);return `<div class="log-entry ${time<=cursor?'current':''}" data-time="${time}">${esc(text)}</div>`;}).join(''):empty('No matching decisions','Try another policy or clear the filter.')}</div></section>`:empty('No decisions recorded','Run a simulation first.'));
}
function about(){return heading('PORTABLE & TRANSPARENT','Take your findings with you.','Local files. Offline interface. An explainable C backend.')+`<div class="grid three">${[['json','JSON','Structured results for tooling'],['csv','CSV','Compare policy metrics in a spreadsheet'],['text','Text','A readable experiment summary']].map(([format,title,desc])=>`<section class="card"><h2>${title}</h2><p>${desc}</p><a class="button" href="/api/export?format=${format}" download>↓ Download ${title}</a></section>`).join('')}</div>`+notice()+`<div class="grid two"><section class="card"><h2>How the application works</h2><div class="steps"><div class="step"><span class="step-number">01</span><div><h3>Observe</h3><p>Platform-specific collectors read process metadata. Permission-denied or unavailable fields must not be treated as measurements.</p></div></div><div class="step"><span class="step-number">02</span><div><h3>Simulate</h3><p>C calculates scheduling decisions, Gantt intervals and metrics. JavaScript only presents these results.</p></div></div><div class="step"><span class="step-number">03</span><div><h3>Inspect</h3><p>Compare the same workload across policies and export the results. Nothing changes the real scheduler.</p></div></div></div></section><section class="card"><h2>Data structures under the hood</h2><div class="feature"><span>Dynamic array · events / jobs</span><span>O(1) amortized append</span></div><div class="feature"><span>Linked list · decision log</span><span>O(1) append</span></div><div class="feature"><span>Circular queue · Round Robin</span><span>O(1) enqueue / dequeue</span></div><div class="feature"><span>Ready-job scan · current SJF / priority</span><span>O(n) selection</span></div><p class="stat-note">The existing heap implementation is not yet connected to those scheduler paths; this page describes current code, not planned claims.</p></section></div><section class="card spacer"><h2>Scope & limitations</h2><p>Single CPU. Estimated bursts. No I/O blocking model. Finite workloads use a waiting-time threshold to flag starvation risk. A recommendation is a heuristic, not evidence of a universal best policy.</p><p>Keyboard: Ctrl/Cmd+K opens navigation. Escape closes it. All charts respect reduced-motion preferences; Gantt playback is user-controlled.</p></section>`;}

const screens={overview,processes,tree,simulator,gantt,compare,aging,adaptive,log,about};
function render(){
  $('#navigation').innerHTML=pages.map(([id,name,icon])=>`<a href="#${id}" ${active===id?'aria-current="page"':''}><span class="nav-icon" aria-hidden="true">${icon}</span>${name}</a>`).join('');
  $('#breadcrumb').textContent=pages.find(p=>p[0]===active)?.[1]??'Overview';
  content.innerHTML=`<div class="page">${error?`<div class="notice warn" role="alert">${esc(error)} ${!connected?'Start the local C server and press Refresh to reconnect.':''}</div>`:''}${busy?'<div class="notice" role="status"><span class="busy-ring"></span> C engine is working… <progress class="capture-progress" aria-label="Backend operation in progress"></progress></div>':''}${screens[active]()}</div>`;
  bind();
  animateCounts();
}
/* Animate only the displayed number; the target is always a backend value. */
function animateCounts(){
  if(matchMedia('(prefers-reduced-motion: reduce)').matches)return;
  $$('[data-count]').forEach(element=>{
    const text=element.textContent,target=Number(text.replaceAll(',',''));
    if(!Number.isFinite(target)||target===0)return;
    const start=performance.now();
    function tick(now){if(!element.isConnected)return;const progress=Math.min(1,(now-start)/250);element.textContent=progress===1?text:fmt(target*(1-(1-progress)**3),0);if(progress<1)requestAnimationFrame(tick);}
    requestAnimationFrame(tick);
  });
}
function listen(id,event,callback){const element=$('#'+id);if(element)element.addEventListener(event,callback);}
function updateCursor(value){
  cursor=Number(value);const label=$('#cursor-label');if(label)label.textContent=fmt(cursor)+' ms';
  $$('.cursor-line').forEach(line=>{const svg=line.closest('svg');let run=state.runs[chosen];if(svg.id==='timeline-second')run=state.runs[second];const end=Math.max(1,...(run?.events??[]).map(e=>e.end_tick));const x=85+Math.min(cursor,end)/end*790;line.setAttribute('x1',x);line.setAttribute('x2',x);});
  $$('.log-entry').forEach(el=>el.classList.toggle('current',Number(el.dataset.time)<=cursor));
}
function stop(){playing=false;cancelAnimationFrame(frame);}
function play(){
  if(playing){stop();render();return;}playing=true;render();let last=0;
  function step(now){if(!playing)return;const input=$('#scrub');if(!input){stop();return;}if(last)updateCursor(Math.min(Number(input.max),cursor+(now-last)/1000*10));last=now;input.value=cursor;if(cursor>=Number(input.max)){stop();render();return;}frame=requestAnimationFrame(step);}
  if(cursor>=Number($('#scrub').max))cursor=0;frame=requestAnimationFrame(step);
}
function bind(){
  listen('demo','click',async()=>{sweep=null;advice=null;cursor=0;await action('/api/demo','Synthetic workload loaded.');});
  listen('capture','click',async()=>{sweep=null;advice=null;cursor=0;await action('/api/capture?seconds=3','Read-only capture complete.');});
  listen('run','click',async()=>{if(!state.jobs.length){notify('Load a demo or capture processes first.');return;}if(!config.algorithms.length){notify('Select at least one policy.');return;}const result=await action('/api/simulate?'+new URLSearchParams({...config,algorithms:config.algorithms.join(',')}),'Simulation complete.');if(result)location.hash='gantt';});
  $$('[data-policy]').forEach(el=>el.addEventListener('change',()=>{config.algorithms=$$('[data-policy]:checked').map(c=>Number(c.dataset.policy));}));
  ['quantum','context','interval','amount'].forEach(id=>listen(id,'input',event=>{config[id]=Number(event.target.value);$('#'+id+'-value').textContent=config[id]+(id==='amount'?'':' ms');}));
  $$('[data-view]').forEach(el=>el.addEventListener('click',()=>{view=el.dataset.view;filter='';sort='pid';render();}));
  const filterChange=id=>listen(id,'input',event=>{const position=event.target.selectionStart;filter=event.target.value;render();const input=$('#'+id);input.focus();input.setSelectionRange(position,position);});
  filterChange('process-filter');filterChange('log-filter');
  listen('process-sort','change',e=>{sort=e.target.value;render();});
  listen('collapse-tree','click',()=>$$('.tree details').forEach(el=>el.open=false));
  listen('gantt-primary','change',e=>{stop();chosen=Number(e.target.value);cursor=0;render();});
  listen('gantt-secondary','change',e=>{second=Number(e.target.value);secondarySelected=true;render();});
  listen('log-policy','change',e=>{chosen=Number(e.target.value);cursor=0;render();});
  listen('scrub','input',e=>{stop();updateCursor(e.target.value);});listen('play','click',play);
  listen('recommend','click',()=>action('/api/recommend','Recommendation updated.'));
  listen('validate','click',()=>action('/api/validate','Validation complete.'));
  listen('sweep','click',async()=>{const data=await action(`/api/sweep?context=${config.context}`,'Quantum sweep complete.');if(data){sweep=data;render();}});
  listen('advisor','click',async()=>{const pid=$('#advisor-pid').value,priority=$('#advisor-priority').value;if(!pid){notify('Load jobs first.');return;}const data=await action('/api/advisor?'+new URLSearchParams({pid,priority}),'Modeled experiment complete.');if(data){advice=data;render();}});
}
function route(){stop();active=location.hash.slice(1);if(!screens[active])active='overview';filter='';render();}
function palette(){const dialog=$('#palette');if(!dialog.open)dialog.showModal();$('#command-search').value='';populateCommands('');$('#command-search').focus();}
function populateCommands(query){$('#command-results').innerHTML=pages.filter(p=>p[1].toLowerCase().includes(query.toLowerCase())).map(([id,name,icon])=>`<button data-go="${id}"><span>${icon}</span>${name}</button>`).join('');$$('[data-go]').forEach(el=>el.addEventListener('click',()=>{location.hash=el.dataset.go;$('#palette').close();}));}
$('#command-open').addEventListener('click',palette);$('#command-search').addEventListener('input',e=>populateCommands(e.target.value));
document.addEventListener('keydown',e=>{if(e.key==='Escape'&&$('#palette').open){e.preventDefault();$('#palette').close();return;}if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='k'){e.preventDefault();palette();}});
$('#refresh').addEventListener('click',refresh);
let theme='dark';try{theme=localStorage.getItem('proclens-theme')||'dark';}catch{/* Storage can be disabled; the app still works. */}
function setTheme(){document.documentElement.dataset.theme=theme;$('#theme span').textContent=theme==='dark'?'Switch to light':'Switch to dark';}
setTheme();$('#theme').addEventListener('click',()=>{theme=theme==='dark'?'light':'dark';setTheme();try{localStorage.setItem('proclens-theme',theme);}catch{}});
window.addEventListener('hashchange',route);route();refresh();

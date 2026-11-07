/* Rendering-function checks in a minimal DOM double, not a visual browser test. */
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import vm from 'node:vm';

const source=await fs.readFile(new URL('../ui/app.js',import.meta.url),'utf8');
const elements=new Map();
const documentListeners=new Map();
function element(selector){
  if(!elements.has(selector))elements.set(selector,{
    innerHTML:'',textContent:'',value:'',dataset:{},isConnected:true,
    classList:{add(){},remove(){},toggle(){}},addEventListener(){},
    querySelectorAll(){return [];},focus(){},setSelectionRange(){},
    open:false,showModal(){this.open=true;},close(){this.open=false;}
  });
  return elements.get(selector);
}
const document={querySelector:element,querySelectorAll:()=>[],documentElement:{dataset:{}},addEventListener(name,callback){documentListeners.set(name,callback);}};
const context=vm.createContext({
  document,window:{addEventListener(){}},location:{hash:''},console,
  matchMedia:()=>({matches:true}),localStorage:{getItem(){return null;},setItem(){}},
  URLSearchParams,performance:{now:()=>0},requestAnimationFrame:()=>0,
  cancelAnimationFrame(){},setTimeout(){return 0;},clearTimeout(){},
  fetch:async()=>({ok:true,status:200,json:async()=>({observed:[],jobs:[],runs:[],groups:[]})}),
});
vm.runInContext(source,context,{filename:'ui/app.js'});
await new Promise(resolve=>setImmediate(resolve));
const names=['overview','processes','tree','simulator','gantt','compare','aging','adaptive','log','about'];
for(const name of names){
  const result=vm.runInContext(`screens[${JSON.stringify(name)}]()`,context);
  assert.equal(typeof result,'string');
  assert.ok(result.length>100,`${name}: empty markup`);
}
context.fixture={
  mode:'demo',observed:[{pid:1,ppid:0,name:'<script>alert(1)</script>',cpu_time_ms:10,memory_kb:0,memory_available:false,timestamp_ms:1000}],
  jobs:[{pid:1,ppid:0,name:'<script>alert(1)</script>',arrival_tick:0,burst_tick:2,remaining_tick:2,priority:1}],
  runs:['Priority','Priority with Aging'].map(name=>({name,average_waiting:0,average_response:0,average_turnaround:2,fairness:1,utilization:1,context_switches:0,starved_count:0,max_waiting:0,makespan:2,
    events:[{pid:1,start_tick:0,end_tick:2,kind:1}],metrics:[{pid:1,waiting_tick:0,response_tick:0,turnaround_tick:2}],decisions:['Time 0: selected PID 1.']})),
  groups:[{root_pid:1,cpu_time_ms:10,memory_kb:0,memory_available:false}],
  recommendation:{policy:1,explanation:'Test rule',features:{job_count:1}},
  validation:{runs:500,matches:1,accuracy:0.002,objective:'average_waiting',seed:42},
};
vm.runInContext('normalize({runs:[{name:"FCFS"}]})',context);
vm.runInContext('normalize(fixture)',context);
assert.ok(vm.runInContext('gantt()',context).includes('<option value="1" selected>Priority with Aging</option>'),
  'The secondary Gantt defaults to a distinct policy after initially empty state');
for(const name of names){
  const result=vm.runInContext(`screens[${JSON.stringify(name)}]()`,context);
  assert.ok(result.length>100);
  assert.ok(!result.includes('<script>alert(1)</script>'),`${name}: unescaped process name`);
  assert.ok(!result.includes('NaN'),`${name}: invalid chart coordinate`);
  assert.ok(!result.includes('undefined'),`${name}: missing value displayed literally`);
}
vm.runInContext("view='simulated'",context);
assert.ok(vm.runInContext('processes()',context).includes('Burst estimate'));
assert.ok(vm.runInContext('tree()',context).includes('Memory unavailable'));
vm.runInContext("error='Test engine error';active='overview';render()",context);
assert.ok(element('#content').innerHTML.includes('Test engine error'));
assert.ok(element('#content').innerHTML.includes('role="alert"'));
element('#palette').open=true;
let escapePrevented=false;
documentListeners.get('keydown')({key:'Escape',preventDefault(){escapePrevented=true;}});
assert.equal(element('#palette').open,false);
assert.ok(escapePrevented);
console.log('PASS: 10 empty screens + 10 populated screens, escaping, chart values, simulated tab, error state and Escape key handling (DOM double)');

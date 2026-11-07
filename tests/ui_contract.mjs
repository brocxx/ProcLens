/* Real HTTP smoke tests. Run with the C server already listening on loopback.
 * This does not replace browser rendering/interaction verification. */
import assert from 'node:assert/strict';

const base = process.argv[2] || 'http://127.0.0.1:8080';
const url = new URL(base);
assert.equal(url.hostname, '127.0.0.1', 'Only a local ProcLens instance may be tested');
let checks = 0;
async function json(path, method = 'GET') {
  const response = await fetch(base + path, {method});
  const body = await response.json();
  assert.equal(response.status, 200, `${method} ${path}: ${JSON.stringify(body)}`);
  assert.ok(!body.error, `${path}: ${body.error}`);
  checks++;
  return body;
}
for (const [path, type] of [['/', 'text/html'], ['/styles.css', 'text/css'], ['/app.js', 'javascript']]) {
  const response = await fetch(base + path);
  assert.equal(response.status, 200);
  assert.ok(response.headers.get('content-type').includes(type));
  const text = await response.text();
  assert.ok(text.length > 100);
  assert.ok(!/https?:\/\/[^\s"']+\.(?:js|css)/.test(text), 'No external runtime assets');
  checks++;
}
await json('/api/health');
await json('/api/demo', 'POST');
const initial = await json('/api/state');
assert.ok(initial.jobs.length > 0, 'Demo must load real synthetic jobs');
assert.equal(initial.mode, 'demo');
const simulationQuery = new URLSearchParams({
  quantum:2,context:0,interval:4,amount:1,algorithms:'0,1,2,3,4,5'
});
await json(`/api/simulate?${simulationQuery}`, 'POST');
const state = await json('/api/state');
assert.equal(state.runs.length, 6);
for (const run of state.runs) {
  assert.ok(run.events.length > 0, `${run.name}: no Gantt events`);
  assert.equal(run.metrics.length, state.jobs.length);
  assert.ok(run.decisions.length > 0);
  assert.ok(Number.isFinite(run.average_waiting));
  assert.ok(run.utilization >= 0 && run.utilization <= 1);
  let previousEnd = 0;
  for (const event of run.events) {
    assert.ok(event.start_tick >= previousEnd, `${run.name}: overlapping Gantt`);
    assert.ok(event.end_tick > event.start_tick);
    previousEnd = event.end_tick;
  }
  for (const job of state.jobs) {
    const slices = run.events.filter(e => e.kind === 1 && e.pid === job.pid);
    assert.equal(slices.reduce((sum,e)=>sum+e.end_tick-e.start_tick,0),job.burst_tick);
    assert.ok(slices.every(e=>e.start_tick>=job.arrival_tick));
  }
  checks++;
}
await json('/api/recommend', 'POST');
const recommended = await json('/api/state');
assert.ok(recommended.recommendation?.explanation);
const validation = await json('/api/validate', 'POST');
assert.ok(validation.validation?.runs > 0);
await json('/api/sweep?context=1', 'POST');
await json(`/api/advisor?pid=${state.jobs[0].pid}&priority=9`, 'POST');
for (const format of ['json','csv','text']) {
  const response = await fetch(`${base}/api/export?format=${format}`);
  assert.equal(response.status,200);
  assert.ok((await response.text()).length > 50);
  checks++;
}
for (const path of ['/api/simulate?quantum=0','/api/capture?seconds=9999']) {
  const response = await fetch(base+path,{method:'POST'});
  assert.ok(response.status >= 400, `${path} must reject invalid input`);
  assert.ok((await response.json()).error);
  checks++;
}
const captureResponse = await fetch(base+'/api/capture?seconds=1',{method:'POST'});
const capture = await captureResponse.json();
assert.equal(captureResponse.status,200,JSON.stringify(capture));
assert.equal(capture.mode,'capture');
assert.ok(capture.observed.length>0,'Live capture must return process observations');
assert.ok(capture.jobs.length>0,'Live capture must produce simulated jobs');
assert.ok(capture.jobs.length<=128,'Captured workload must be bounded by job count');
assert.ok(capture.observed.every(record=>typeof record.memory_available==='boolean'));
assert.ok(capture.jobs.every(job=>typeof job.name==='string'&&job.name.length>0),
  'Captured jobs must retain process names');
const pids = new Set(capture.observed.map(record=>record.pid));
assert.equal(pids.size,capture.observed.length,'Observed capture rows must be unique by PID');
assert.ok(Number.isInteger(capture.omitted_jobs)&&capture.omitted_jobs>=0);
assert.ok(capture.jobs.reduce((sum,job)=>sum+job.burst_tick,0)<=20000,
  'Captured workload must respect the total burst bound');
checks++;
const capturedRun = await json('/api/simulate?algorithms=0','POST');
assert.equal(capturedRun.runs.length,1);
assert.ok(capturedRun.runs[0].events.length>0);
const missing=await fetch(base+'/not-a-real-page');
assert.equal(missing.status,404);
checks++;
console.log(`PASS: ${checks} API/UI contract checks (real C backend; no browser rendering claim)`);

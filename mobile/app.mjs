import {Session,AttemptStore,FIXTURE,bodies} from './core.mjs';
const $=id=>document.getElementById(id), session=new Session();
let store, storage;try{storage=window.localStorage;store=new AttemptStore(storage);}catch{}
const pendingKey='uts.mobile.pending.v1';
const message=text=>{$('message').textContent=text;};
let persistFailed=false;
function save(record){try{if(!store)throw Error();store.save(record);storage.removeItem(pendingKey);persistFailed=false;return true;}catch{persistFailed=true;message('Could not save locally. This attempt may be lost on reload. Export/recovery is not implemented yet.');return false;}}
function checkpoint(){if(!['active','paused'].includes(session.status))return;try{if(!storage)throw Error();storage.setItem(pendingKey,JSON.stringify({id:session.id,startedAt:session.startedAt,outcome:'interrupted',ticks:session.ticks,fixture:FIXTURE.id,model:FIXTURE.model,config:{...FIXTURE},validation:'synthetic-unvalidated',events:[...session.events,{type:'interrupted',tick:session.ticks}],frames:session.frames,inputs:session.inputs,finalState:session.vehicle}));}catch{message('Local checkpoint failed. Keep this window open; progress may not survive a restart.');}}
try{const pending=storage?.getItem(pendingKey);if(pending){save(JSON.parse(pending));message(persistFailed?'Recovery failed; local data was retained for inspection.':'Recovered an interrupted attempt from its last checkpoint.');}store?.read();}catch{message('Local history is damaged or unsupported. It has not been overwritten. Use History to explicitly delete it.');}
function sync(){const active=session.status==='active',running=['active','paused'].includes(session.status);
 $('start').disabled=running||persistFailed;$('pause').disabled=!running;$('pause').textContent=session.status==='paused'?'Resume':'Pause';
 for(const id of ['reset','end'])$(id).disabled=!running;
 for(const id of ['forward','reverse','throttle','brake','steer','center'])$(id).disabled=!active;
 $('reverse').setAttribute('aria-pressed',String(session.direction===-1));$('forward').setAttribute('aria-pressed',String(session.direction===1));
 $('state').textContent=session.status==='idle'?'Ready':session.status[0].toUpperCase()+session.status.slice(1);
 $('motion').textContent=Math.abs(session.vehicle.speed)<.001?'Stopped':session.vehicle.speed>0?'Moving forward':'Reversing';
 $('events').textContent=(session.events?.filter(e=>e.type==='synthetic_boundary_entry').length||0)+' boundary entries';
 $('steer').value=Math.round(session.input.steer*100);$('steer-value').textContent=session.input.steer===0?'Centered':Math.abs(Math.round(session.input.steer*100))+'% '+(session.input.steer<0?'left':'right');
}
function clearPedals(){session.input.throttle=false;session.input.brake=false;for(const id of ['throttle','brake'])$(id).classList.remove('held');}
function pause(){session.pause();clearPedals();checkpoint();sync();}
function begin(){if(persistFailed){message('Resolve unsaved history before starting another attempt.');return;}session.start(crypto.randomUUID(),new Date().toISOString());message('Synthetic developer test — no completion rules or pass/fail score.');checkpoint();sync();}
function finish(outcome){clearPedals();const record=session.finish(outcome);if(record)save(record);sync();return record;}
function modal(title,text){pause();$('modal-title').textContent=title;$('modal-body').replaceChildren();const p=document.createElement('p');p.textContent=text;$('modal-body').append(p);$('delete-history').hidden=true;$('modal').showModal();}
function confirmReset(){if(!confirm('End and keep this attempt as reset, then start again?'))return;finish('reset');if(!persistFailed)begin();}
$('start').onclick=begin;
$('pause').onclick=()=>{if(session.status==='paused'){session.resume();sync();}else pause();};
$('reset').onclick=confirmReset;
$('end').onclick=()=>{const r=finish('ended');if(r)modal('Attempt ended',`${(r.ticks/60).toFixed(1)} seconds of simulated movement time. ${r.events.filter(e=>e.type==='synthetic_boundary_entry').length} synthetic boundary entries. ${persistFailed?'Not saved.':'Saved on this device.'} No completion criteria or score are approved.`);};
$('steer').oninput=e=>{session.input.steer=Number(e.target.value)/100;sync();};
$('center').onclick=()=>{session.input.steer=0;sync();};
for(const [id,d] of [['forward',1],['reverse',-1]])$(id).onclick=()=>{if(!session.setDirection(d))message('Stop the vehicle before changing direction.');else message(d===1?'Forward selected.':'Reverse selected.');sync();};
for(const id of ['throttle','brake']){
 const btn=$(id);const held=new Set();
 btn.addEventListener('pointerdown',e=>{if(session.status!=='active')return;e.preventDefault();btn.setPointerCapture(e.pointerId);held.add(e.pointerId);session.input[id]=true;btn.classList.add('held');});
 function release(e){held.delete(e.pointerId);session.input[id]=held.size>0&&session.status==='active';btn.classList.toggle('held',session.input[id]);}
 btn.addEventListener('pointerup',release);btn.addEventListener('lostpointercapture',release);btn.addEventListener('pointercancel',e=>{held.clear();release(e);pause();});
 btn.addEventListener('keydown',e=>{if([' ','Enter'].includes(e.key)&&session.status==='active'){e.preventDefault();session.input[id]=true;btn.classList.add('held');}});
 btn.addEventListener('keyup',e=>{if([' ','Enter'].includes(e.key)){session.input[id]=false;btn.classList.remove('held');}});
 btn.addEventListener('blur',()=>{session.input[id]=false;held.clear();btn.classList.remove('held');});
}
$('help').onclick=()=>modal('Controls & limits','Hold Accelerate to move. Drag steering left or right; it stays where you leave it. Hold Brake to stop, then select Forward or Reverse. Center steering explicitly when desired. Keyboard users can focus a pedal and hold Space. This overhead developer demo uses arbitrary scene units, simplified articulation and artificial response parameters. It is not an accurate truck or a CDL evaluation. There are no collision forces, automatic completion, mirrors or steering assistance. Each demo records up to three minutes; the last ten ended attempts are retained locally.');
$('history').onclick=()=>{modal('Local attempts','Only this device/browser. Last ten attempts. No account or cloud upload.');$('delete-history').hidden=false;try{if(!store)throw Error();const list=store.read();if(!list.length){const p=document.createElement('p');p.textContent='No saved attempts yet.';$('modal-body').append(p);}for(const a of list){const p=document.createElement('p');p.textContent=`${a.outcome} · ${new Date(a.startedAt).toLocaleString()} · ${(a.ticks/60).toFixed(1)}s`;const small=document.createElement('small');small.textContent=`${a.events.filter(e=>e.type==='synthetic_boundary_entry').length} boundary entries · ${a.fixture} · unvalidated`;p.append(small);$('modal-body').append(p);}}catch{message('History could not be read. Existing data has not been overwritten.');}};
$('delete-history').onclick=()=>{if(!confirm('Permanently delete saved local attempts and recovery data?'))return;try{if(!store)throw Error();store.clear();storage.removeItem(pendingKey);persistFailed=false;$('modal').close();message('Local history deleted.');sync();}catch{message('Could not delete local history.');}};
$('close').onclick=()=>$('modal').close();
window.addEventListener('blur',pause);document.addEventListener('visibilitychange',()=>{if(document.hidden)pause();});window.addEventListener('pagehide',pause);
const canvas=$('scene'),ctx=canvas.getContext('2d');
function render(){const rect=canvas.getBoundingClientRect(),dpr=Math.min(devicePixelRatio||1,2);if(canvas.width!==Math.round(rect.width*dpr)||canvas.height!==Math.round(rect.height*dpr)){canvas.width=Math.round(rect.width*dpr);canvas.height=Math.round(rect.height*dpr);}ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,rect.width,rect.height);
 const scale=Math.min(rect.width/34,rect.height/40);const ox=rect.width/2-16*scale,oy=rect.height/2-25*scale;
 ctx.save();ctx.translate(ox,oy);ctx.scale(scale,scale);ctx.fillStyle='#22363e';ctx.fillRect(0,-10,32,80);ctx.strokeStyle='#30464d';ctx.lineWidth=.04;
 for(let x=0;x<=32;x+=2){ctx.beginPath();ctx.moveTo(x,-10);ctx.lineTo(x,70);ctx.stroke();}for(let y=-10;y<70;y+=2){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(32,y);ctx.stroke();}
 ctx.fillStyle='#abcdaa0a';ctx.fillRect(11,9,10,38);ctx.setLineDash([.7,.7]);ctx.lineWidth=.12;ctx.strokeStyle='#b5c6bc';for(const x of [11,21]){ctx.beginPath();ctx.moveTo(x,9);ctx.lineTo(x,47);ctx.stroke();}ctx.setLineDash([]);
 for(let y=10;y<48;y+=4)for(const x of [10,22]){ctx.fillStyle='#f5a75d';ctx.beginPath();ctx.moveTo(x,y-.4);ctx.lineTo(x-.3,y+.3);ctx.lineTo(x+.3,y+.3);ctx.closePath();ctx.fill();}
 const bs=bodies(session.vehicle);for(const [i,b] of [bs[1],bs[0]].entries()){ctx.save();ctx.translate(b.x,b.y);ctx.rotate(b.heading);ctx.fillStyle='#09182070';ctx.fillRect(-b.width/2+.2,-b.length/2+.3,b.width,b.length);ctx.fillStyle=i===0?'#cad8d9':'#aee7c6';ctx.fillRect(-b.width/2,-b.length/2,b.width,b.length);ctx.strokeStyle='#759494';ctx.lineWidth=.09;ctx.strokeRect(-b.width/2,-b.length/2,b.width,b.length);if(i===1){ctx.fillStyle='#23414b';ctx.fillRect(-.9,-1.9,1.8,.9);}else{ctx.strokeStyle='#adbec1';for(let k=-3;k<3;k+=.7){ctx.beginPath();ctx.moveTo(-.9,k);ctx.lineTo(.9,k);ctx.stroke();}}ctx.restore();}
 ctx.restore();}
let previous=performance.now(),accumulator=0;
function frame(now){const elapsed=(now-previous)/1000;previous=now;if(elapsed>.25&&session.status==='active'){pause();message('Paused after a long frame interruption. Tap Resume when ready.');}if(session.status==='active'){accumulator+=Math.min(elapsed,.25);while(accumulator>=FIXTURE.dt){session.tick();accumulator-=FIXTURE.dt;if(session.ticks%60===0)checkpoint();if(session.status!=='active'){clearPedals();message('Developer recording limit reached. End or reset this attempt.');break;}}}else accumulator=0;sync();render();requestAnimationFrame(frame);}
sync();requestAnimationFrame(frame);

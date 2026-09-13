// SYNTHETIC developer fixture. Distances are arbitrary scene units, not feet/meters.
// No values below are approved truck dimensions, steering characteristics or CDL rules.
export const FIXTURE = Object.freeze({id:'synthetic-yard-v1',model:'toy-articulation-v1',dt:1/60,wheelbase:4,drawbar:7,tractorLength:5,trailerLength:8,width:2.4,maxSteer:0.55,accel:1.5,drag:0.5,brake:5,maxSpeed:3,laneMin:11,laneMax:21});
export const initialVehicle=()=>({x:16,y:23,heading:0,trailerHeading:0,speed:0});
export const clearInput=()=>({steer:0,throttle:false,brake:false});
export function step(s,input,direction,c=FIXTURE) {
  const dt=c.dt;
  const steer=Math.max(-1,Math.min(1,Number.isFinite(input.steer)?input.steer:0));
  let speed=s.speed;
  if(input.brake) speed=Math.sign(speed)*Math.max(0,Math.abs(speed)-c.brake*dt);
  else if(input.throttle) speed=Math.max(-c.maxSpeed,Math.min(c.maxSpeed,speed+direction*c.accel*dt));
  else speed=Math.sign(speed)*Math.max(0,Math.abs(speed)-c.drag*dt);
  if (Math.abs(speed)<1e-12) speed=0;
  const rate=speed/c.wheelbase*Math.tan(steer*c.maxSteer);
  return {x:s.x+speed*Math.sin(s.heading)*dt,y:s.y-speed*Math.cos(s.heading)*dt,heading:s.heading+rate*dt,trailerHeading:s.trailerHeading+speed/c.drawbar*Math.sin(s.heading-s.trailerHeading)*dt,speed};
}
export function bodies(s,c=FIXTURE){
  const rear={x:s.x-c.drawbar*Math.sin(s.trailerHeading),y:s.y+c.drawbar*Math.cos(s.trailerHeading)};
  return [{x:s.x,y:s.y,heading:s.heading,length:c.tractorLength,width:c.width},{x:(s.x+rear.x)/2,y:(s.y+rear.y)/2,heading:s.trailerHeading,length:c.trailerLength,width:c.width}];
}
export function outside(s,c=FIXTURE){return bodies(s,c).some(b=>{
  const dx=Math.abs(Math.sin(b.heading))*b.length/2+Math.abs(Math.cos(b.heading))*b.width/2;
  return b.x-dx<c.laneMin||b.x+dx>c.laneMax;
});}
export class Session {
  constructor(){this.status='idle';this.input=clearInput();this.vehicle=initialVehicle();this.direction=-1;}
  start(id,now){this.id=id;this.startedAt=now;this.status='active';this.vehicle=initialVehicle();this.input=clearInput();this.direction=-1;this.ticks=0;this.events=[{type:'started',tick:0}];this.frames=[];this.inputs=[];this.lastInput='';this.wasOutside=false;}
  event(type){this.events.push({type,tick:this.ticks});}
  tick(){if(this.status!=='active')return;
    const signature=JSON.stringify([this.input,this.direction]);
    if(signature!==this.lastInput){this.inputs.push({tick:this.ticks,...this.input,direction:this.direction});this.lastInput=signature;}
    this.vehicle=step(this.vehicle,this.input,this.direction);this.ticks++;
    const o=outside(this.vehicle);if(o&&!this.wasOutside)this.event('synthetic_boundary_entry');this.wasOutside=o;
    if(this.ticks%6===0)this.frames.push({tick:this.ticks,...this.vehicle});
    // Bound memory/storage for this developer fixture, not a training time limit.
    if(this.ticks>=60*180){this.pause();this.event('developer_recording_limit');}
  }
  pause(){if(this.status==='active'){this.status='paused';this.event('paused');}this.input={...this.input,throttle:false,brake:false};}
  resume(){if(this.status==='paused'&&this.ticks<60*180){this.status='active';this.event('resumed');}}
  setDirection(d){if(this.status!=='active'||![-1,1].includes(d)||Math.abs(this.vehicle.speed)>0.001)return false;this.direction=d;this.event(d===1?'forward_selected':'reverse_selected');return true;}
  finish(outcome='ended'){if(!['active','paused'].includes(this.status))return null;this.event(outcome);this.status='ended';this.input=clearInput();return {id:this.id,startedAt:this.startedAt,outcome,ticks:this.ticks,fixture:FIXTURE.id,model:FIXTURE.model,config:{...FIXTURE},validation:'synthetic-unvalidated',events:this.events,frames:this.frames,inputs:this.inputs,finalState:{...this.vehicle}};}
}
export class AttemptStore {
  constructor(storage){this.storage=storage;this.key='uts.mobile.history.v1';}
  read(){const raw=this.storage.getItem(this.key);if(!raw)return [];const p=JSON.parse(raw);if(p.version!==1||!Array.isArray(p.attempts)||p.attempts.some(a=>!a||typeof a.id!=='string'||!Array.isArray(a.events)))throw Error('Unsupported or damaged history');return p.attempts;}
  save(attempt){const list=this.read().filter(a=>a.id!==attempt.id);list.unshift(attempt);this.storage.setItem(this.key,JSON.stringify({version:1,attempts:list.slice(0,10)}));}
  clear(){this.storage.removeItem(this.key);}
}

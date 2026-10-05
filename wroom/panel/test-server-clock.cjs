const vm=require('vm'),fs=require('fs'),assert=require('assert');
for(const skew of [140000,-300000,0])for(const name of ['ping-widget','ota-widget','diagnostic']){
 let mono=1000,epoch=1791196272000,clockReply,last,reply;
 const fields={},buttons={};class SkewDate extends Date{static now(){return epoch+skew;}}
 const self={ctx:{$container:[{querySelector(q){const a=q.match(/data-action="([^"]+)"/);if(a)return buttons[a[1]]||(buttons[a[1]]={});if(q==='button')return buttons.ping||(buttons.ping={});const f=q.match(/data-field="([^"]+)"/);return fields[f[1]]||(fields[f[1]]={});}}],http:{get:u=>{assert.equal(u,'/api/dashboard/serverTime');return{subscribe:o=>{clockReply=o;return{unsubscribe(){}};}};}},servicesMap:new Map([['deviceService','d']]),$scope:{$injector:{get:()=>({sendTwoWayRpcCommand:(id,b)=>{last=b;return{subscribe:o=>{reply=o;return{unsubscribe(){}};}};}})}},data:[]}};
 const c={self,performance:{now:()=>mono},Date:SkewDate,Math,Number,String,JSON,isFinite,setInterval:()=>1,clearInterval(){}};vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/'+name+'.js','utf8'),c);
 function data(age){self.ctx.data=Object.entries({firmware_version:'0.0.7',origin:'ESP32_REAL',simulated:false}).map(([name,value])=>({dataKey:{name},data:[[epoch-age,value]]}));self.onDataUpdated();}
 self.onInit();data(55000);assert(!self.ready);clockReply.next(epoch);data(55000);
 if(name==='diagnostic'){assert.equal(fields.presence.textContent,'CONTATO RECENTE');assert(fields.age.textContent.includes('55s'));data(195000);assert.equal(fields.presence.textContent,'SEM CONTATO RECENTE');}
 else{assert(self.ready);const b=name==='ping-widget'?buttons.ping:buttons.check;b.onclick();assert.equal(last.params.issued_at_ms,epoch);reply.error();data(195000);assert(!self.ready);data(-30000);assert(!self.ready);data(55000);mono+=120001;self.onDataUpdated();assert(!self.ready);}
 self.onDestroy();
}
console.log('SERVER_CLOCK_TESTS_OK: +/- wall-clock skew, server issued_at, true stale/future data, expired clock fails closed');

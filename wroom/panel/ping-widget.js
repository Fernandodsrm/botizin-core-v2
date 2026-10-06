// Server epoch anchored to monotonic elapsed time; browser wall clock is never trusted.
function labMono(){return performance.now();}
function labNow(){var c=self.labClock;return c&&labMono()-c.at<120000?c.epoch+labMono()-c.at:NaN;}
function labRecent(ts){var age=labNow()-Number(ts);return isFinite(age)&&age>=-2000&&age<180000;}
function labClockStart(){
 self.labClock=null;
 function sync(){
  if(self.labClockPending)return;
  self.labClockPending=true;var start=labMono();
  try{self.labClockSubscription=self.ctx.http.get('/api/dashboard/serverTime').subscribe({
   next:function(epoch){var end=labMono();self.labClockPending=false;
    if(typeof epoch==='number'&&isFinite(epoch)&&epoch>0&&end-start<=10000){
     // Upper-bound age by adding full round-trip time; never make old data look newer.
     self.labClock={epoch:epoch+end-start,at:end};
    }
    if(self.root)self.onDataUpdated();
   },error:function(){self.labClockPending=false;if(self.root)self.onDataUpdated();}
  });}catch(e){self.labClockPending=false;}
 }
 sync();self.labClockTimer=setInterval(sync,60000);
}
function labClockStop(){clearInterval(self.labClockTimer);if(self.labClockSubscription)self.labClockSubscription.unsubscribe();self.labClock=null;}

self.onInit = function () {
 self.root = self.ctx.$container[0]; self.busy=false;
 self.button=self.root.querySelector('button');
 self.button.onclick=sendPing;
 self.timer=setInterval(updateReady,1000);
 labClockStart();self.onDataUpdated();
};
function text(key,value){var el=self.root.querySelector('[data-field="'+key+'"]');if(el)el.textContent=value;}
self.onDataUpdated=function(){
 self.v={};(self.ctx.data||[]).forEach(function(r){if(r.data&&r.data.length){var p=r.data[r.data.length-1];if(p[1]!==null)self.v[r.dataKey.name]={value:p[1],ts:Number(p[0])};}});updateReady();
};
function updateReady(){
 if(!self.root)return;
 var v=self.v||{},version=v.firmware_version;
 self.ready=version&&['0.0.3','0.0.4','0.0.5','0.0.6','0.0.7','0.0.8','0.0.9','0.0.10','0.0.11','0.0.12'].indexOf(String(version.value))>=0&&v.origin&&String(v.origin.value)==='ESP32_REAL'&&v.simulated&&String(v.simulated.value)==='false'&&labRecent(version.ts);
 self.button.disabled=self.busy||!self.ready;self.button.textContent=self.busy?'Aguardando PONG…':'Testar resposta (PING)';
 var reason='Aguardando telemetria da WROOM. Recarregue o painel e aguarde até 1 minuto.';
 if(version){
  var age=Math.max(0,Math.floor((labNow()-version.ts)/1000));
  if(age>=180)reason='Contato exibido há '+age+' segundos. Recarregue o painel; os dados precisam ser recentes.';
  else if(['0.0.3','0.0.4','0.0.5','0.0.6','0.0.7','0.0.8','0.0.9','0.0.10','0.0.11','0.0.12'].indexOf(String(version.value))<0)reason='Versão exibida: '+version.value+'. Aguardando firmware compatível.';
  else if(!v.origin||!v.simulated)reason='Aguardando identificação do relato real da placa. Recarregue o painel.';
 }
 if(!isFinite(labNow()))reason='Sincronizando horário com o servidor. Aguarde ou recarregue o painel.';
 text('readiness',self.busy?'Aguardando PONG real, por até 60 segundos…':self.ready?'Pronto. A placa consulta comandos a cada 15 segundos.':reason);
}
function sendPing(){
 if(!self.ready||self.busy)return;
 self.busy=true;updateReady();
 var elapsedStart=labMono(), started=Math.floor(labNow()), command='ping-'+started+'-'+Math.random().toString(16).slice(2,10);
 self.command=command;text('result','Pedido enviado: '+command);text('details','Aguardando PONG; envio aceito pelo servidor não é confirmação da placa.');
 try{
  var service=self.ctx.$scope.$injector.get(self.ctx.servicesMap.get('deviceService'));
  self.subscription=service.sendTwoWayRpcCommand('368826d0-c082-11f1-8cda-091689c2cb7f',{method:'ping',params:{command_id:command,issued_at_ms:started},timeout:60000,persistent:false}).subscribe({
   next:function(reply){
    self.busy=false;
    if(reply&&reply.result==='PONG'&&reply.origin==='ESP32_REAL'&&reply.command_id===command&&reply.firmware_version&&reply.boot_id){
     text('result','PONG confirmado em '+((labMono()-elapsedStart)/1000).toFixed(1)+' segundos');
     text('details','Pedido: '+command+'\nVersão: '+reply.firmware_version+'\nBoot: '+reply.boot_id+'\nExecutando: '+reply.running_partition+'\nBoot configurado: '+reply.boot_partition+'\nEstado OTA: '+reply.ota_state+'\nReset: '+reply.reset_reason);
    }else{text('result','Resposta sem correspondência — não confirmado');text('details',JSON.stringify(reply,null,2));}
    updateReady();
   },error:function(){self.busy=false;text('result','Sem confirmação da placa');text('details','Sem PONG em até 60 segundos. Recarregue o painel e confira o último contato antes de repetir.');updateReady();}
  });
 }catch(e){self.busy=false;text('result','Falha ao enviar PING');text('details','Nenhuma confirmação recebida.');updateReady();}
}
self.onDestroy=function(){labClockStop();clearInterval(self.timer);if(self.subscription)self.subscription.unsubscribe();if(self.button)self.button.onclick=null;self.root=null;};
self.typeParameters=function(){return{maxDatasources:1,singleEntity:true};};

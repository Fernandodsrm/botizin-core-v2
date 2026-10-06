self.onInit = function () {
 self.root = self.ctx.$container[0]; self.busy=false;
 self.button=self.root.querySelector('button');
 self.button.onclick=sendPing;
 self.timer=setInterval(updateReady,1000);
 self.onDataUpdated();
};
function text(key,value){var el=self.root.querySelector('[data-field="'+key+'"]');if(el)el.textContent=value;}
self.onDataUpdated=function(){
 self.v={};(self.ctx.data||[]).forEach(function(r){if(r.data&&r.data.length){var p=r.data[r.data.length-1];if(p[1]!==null)self.v[r.dataKey.name]={value:p[1],ts:Number(p[0])};}});updateReady();
};
function updateReady(){
 if(!self.root)return;
 var v=self.v||{},version=v.firmware_version;
 self.ready=version&&['0.0.10','0.0.11'].indexOf(String(version.value))>=0&&v.origin&&String(v.origin.value)==='ESP32_REAL'&&v.simulated&&String(v.simulated.value)==='false'&&Date.now()-version.ts<180000;
 self.button.disabled=self.busy||!self.ready;
 text('readiness',self.busy?'Aguardando resposta da placa…':self.ready?'Pronto. A placa consulta comandos a cada 15 segundos.':'Disponível quando a placa informar 0.0.10 ou 0.0.11 com contato recente.');
}
function sendPing(){
 if(!self.ready||self.busy)return;
 self.busy=true;updateReady();
 var started=Date.now(), command='ping-'+started+'-'+Math.random().toString(16).slice(2,10);
 self.command=command;text('result','Pedido enviado: '+command);text('details','Aguardando PONG; envio aceito pelo servidor não é confirmação da placa.');
 try{
  var service=self.ctx.$scope.$injector.get(self.ctx.servicesMap.get('deviceService'));
  self.subscription=service.sendTwoWayRpcCommand('4a771080-c036-11f1-b143-c924012d9fae',{method:'ping',params:{command_id:command,issued_at_ms:started},timeout:30000,persistent:false}).subscribe({
   next:function(reply){
    self.busy=false;
    if(reply&&reply.result==='PONG'&&reply.origin==='ESP32_REAL'&&reply.command_id===command&&reply.firmware_version&&reply.boot_id){
     text('result','PONG confirmado em '+((Date.now()-started)/1000).toFixed(1)+' segundos');
     text('details','Pedido: '+command+'\nVersão: '+reply.firmware_version+'\nBoot: '+reply.boot_id+'\nExecutando: '+reply.running_partition+'\nBoot configurado: '+reply.boot_partition+'\nEstado OTA: '+reply.ota_state+'\nReset: '+reply.reset_reason);
    }else{text('result','Resposta sem correspondência — não confirmado');text('details',JSON.stringify(reply,null,2));}
    updateReady();
   },error:function(){self.busy=false;text('result','Sem confirmação da placa');text('details','Tempo esgotado ou falha de comunicação. Nenhuma ação física foi solicitada.');updateReady();}
  });
 }catch(e){self.busy=false;text('result','Falha ao enviar PING');text('details','Nenhuma confirmação recebida.');updateReady();}
}
self.onDestroy=function(){clearInterval(self.timer);if(self.subscription)self.subscription.unsubscribe();if(self.button)self.button.onclick=null;self.root=null;};
self.typeParameters=function(){return{maxDatasources:1,singleEntity:true};};

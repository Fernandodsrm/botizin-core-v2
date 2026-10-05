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

#pragma once
#include <stdint.h>
// Procedural 128x64 character: no images, heap allocation, floating point or delays.
class BotizinFace {
  uint32_t rng,nextMood=0,nextLook=0,nextBlink=0,blinkAt=0;
  int x=0,y=0,tx=0,ty=0,height=34*256,width=36*256;
  uint8_t mood=0,bag=0,blinkKind=0;
  uint32_t forcedUntil=0;bool forced=false;
  bool blinking=false,secondBlink=false;
  uint32_t random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
  static bool due(uint32_t now,uint32_t at){return int32_t(now-at)>=0;}
  static int ease(int current,int target){int d=target-current;return current+(d/4?d/4:d);}
  static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
public:
  explicit BotizinFace(uint32_t seed):rng(seed?seed:1){}
  uint8_t expression() const{return mood;}
  void command(uint8_t value,uint32_t now){
    if(value==255){forced=false;nextMood=now;return;}
    if(value>7)return;
    mood=value;forced=true;forcedUntil=now+8000;nextLook=now;
  }
  template<class Canvas> void draw(Canvas &c,uint32_t now){
    if(forced&&due(now,forcedUntil)){forced=false;nextMood=now;}
    if(!forced&&due(now,nextMood)){
      // Shuffle bag guarantees variety without a predictable repeating sequence.
      if(!bag)bag=255;
      uint8_t pick=uint8_t(random()%8);
      while(!(bag&(1u<<pick)))pick=(pick+1)%8;
      bag&=uint8_t(~(1u<<pick));mood=pick;
      nextMood=now+2200+random()%2600;nextLook=now;
    }
    if(due(now,nextLook)){
      tx=(int(random()%17)-8)*256;ty=(int(random()%7)-3)*256;
      if(mood==1){tx=0;ty=-2*256;} // A brief attentive glance back to the viewer.
      if(mood==5){tx=-6*256;ty=-3*256;}
      nextLook=now+550+random()%1450;
    }
    if(!blinking&&due(now,nextBlink)){
      blinking=true;blinkAt=now;blinkKind=mood==6?1:0;
      secondBlink=(random()%4)==0;
    }
    uint32_t age=now-blinkAt;
    if(blinking&&age>=160){
      blinking=false;nextBlink=now+(secondBlink?110:1700+random()%2700);
      secondBlink=false;
    }
    x=ease(x,tx);y=ease(y,ty);
    int targetH=mood==3?23:mood==4?40:mood==7?27:34;
    int targetW=mood==4?32:mood==3?38:36;
    height=ease(height,targetH*256);width=ease(width,targetW*256);
    int breath=int((now/220)%8);breath=breath>4?8-breath:breath;
    int gx=x/256,gy=y/256+breath/3;
    c.clearDisplay();
    for(int eye=0;eye<2;++eye){
      int w=width/256,h=height/256;
      if(mood==2)h+=eye==0?-9:3; // Curiosity: genuinely different eye shapes.
      if(mood==5&&eye==0)h-=13;
      if(blinking&&(blinkKind==0||eye==0)){
        int closure=age<70?int(age)*100/70:age<100?100:int(160-age)*100/60;
        h=h*(100-clamp(closure,0,100))/100;
      }
      h=clamp(h,2,42);
      int ex=(eye?75:17)+gx,ey=clamp(37+gy-h/2,17,62-h);
      c.fillRoundRect(ex,ey,w,h,clamp(h/3,1,9),1);
      if(h>12){
        if(mood==3){ // Smiling eyes; lower lids rise in an arc.
          c.fillRoundRect(ex-1,ey+h/2,w+2,h,9,0);
        }else{
          int px=ex+w/2-5+gx/3,py=ey+h/2-6+gy/2;
          c.fillRoundRect(px,py,10,12,4,0);
          c.fillCircle(px+7,py+3,1,1);
        }
      }
      int bx=ex+3,by=10;
      if(mood==2||mood==5)by+=eye==0?3:-2;
      if(mood==7){
        c.fillTriangle(bx,by,bx+w-6,by+4,bx+w-6,by+1,1);
      }else{
        c.fillRoundRect(bx,by,w-6,3,1,1);
      }
    }
    // Tiny mouth stays secondary to the eyes and inside the blue band.
    if(mood==4)c.drawCircle(64+gx/2,60,2,1);
    else if(mood==3||mood==6){c.drawLine(59,59,61,61,1);c.drawLine(61,61,67,61,1);c.drawLine(67,61,69,59,1);}
    else c.drawLine(61+gx/2,60,66+gx/2,60,1);
  }
};

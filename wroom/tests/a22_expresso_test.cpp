#include <cassert>
#include <cstdio>
#include "../a22-expresso/face_engine.h"
struct Canvas {
  unsigned draws=0;
  void clearDisplay(){}
  void fillRoundRect(int x,int y,int w,int h,int radius,int color){
    assert(w>0&&h>0&&radius>=0);assert(x>=0&&x<128&&y>=0&&y<64);
    if(color)assert(x+w<=128&&y+h<=64);
    ++draws;
  }
  void fillCircle(int x,int y,int r,int){assert(x>=0&&x<128&&y>=0&&y<64&&r>0);++draws;}
  void drawCircle(int x,int y,int r,int){assert(x-r>=0&&x+r<128&&y-r>=0&&y+r<64);++draws;}
  void fillTriangle(int x,int y,int a,int b,int d,int e,int){assert(x>=0&&a<128&&d<128&&y>=0&&b<64&&e<64);++draws;}
  void drawLine(int x,int y,int a,int b,int){assert(x>=0&&a<128&&y>=0&&b<64);++draws;}
};
#include "../a22-expresso/command_model.h"
int main(){
  uint32_t value=99;
  assert(!validCommand("motor",value)&&value==99);
  assert(!validCommand(nullptr,value));
  for(unsigned i=0;i<8;++i){assert(validCommand(expressionName(i),value)&&value==i);}
  assert(validCommand("auto",value)&&value==255);
  BotizinFace commanded(42);Canvas cc;
  for(unsigned i=0;i<8;++i){
    uint32_t begin=100000+i*10000;
    commanded.command(i,begin);
    for(uint32_t t=begin;t<begin+8000;t+=62){commanded.draw(cc,t);assert(commanded.expression()==i);}
  }
  commanded.command(3,0xfffff000u);
  for(uint32_t t=0;t<8000;t+=62){commanded.draw(cc,uint32_t(0xfffff000u+t));assert(commanded.expression()==3);}
  commanded.command(255,1000000);unsigned after=0;
  for(uint32_t t=1000000;t<1060000;t+=62){commanded.draw(cc,t);after|=1u<<commanded.expression();}
  assert(after==255);
  commanded.command(2,2000000);unsigned expired=0;
  for(uint32_t t=2008000;t<2068000;t+=62){commanded.draw(cc,t);expired|=1u<<commanded.expression();}
  assert(expired==255);

  for(uint32_t seed=1;seed<=100;++seed){
    BotizinFace face(seed);Canvas c;unsigned moods=0;
    for(uint32_t now=0;now<60000;now+=62){face.draw(c,now);moods|=1u<<face.expression();}
    assert(moods==255&&c.draws>5000);
    // Timer rollover cannot stop or invalidate drawing.
    for(uint32_t step=0;step<10000;step+=62)face.draw(c,uint32_t(0xfffff000u+step));
  }
  puts("FACE_ENGINE_OK: all 8 expressions, bounded lit geometry, 100 seeds, timer rollover");
}

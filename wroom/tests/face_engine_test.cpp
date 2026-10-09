#include <cassert>
#include <cstdio>
#include "../firmware/face_engine.h"
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
int main(){
  for(uint32_t seed=1;seed<=100;++seed){
    BotizinFace face(seed);Canvas c;unsigned moods=0;
    for(uint32_t now=0;now<60000;now+=62){face.draw(c,now);moods|=1u<<face.expression();}
    assert(moods==255&&c.draws>5000);
    // Timer rollover cannot stop or invalidate drawing.
    for(uint32_t step=0;step<10000;step+=62)face.draw(c,uint32_t(0xfffff000u+step));
  }
  puts("FACE_ENGINE_OK: all 8 expressions, bounded lit geometry, 100 seeds, timer rollover");
}

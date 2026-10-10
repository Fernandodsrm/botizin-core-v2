#pragma once
#include <stdint.h>
#include <string.h>
static int expressionCode(const char *name){
  if(!name)return -1;
  const char *names[]={"normal","atento","curioso","feliz","surpreso","pensando","piscadinha","serio"};
  for(int i=0;i<8;++i)if(strcmp(name,names[i])==0)return i;
  return strcmp(name,"auto")==0?255:-1;
}
static const char *expressionName(unsigned code){
  const char *names[]={"normal","atento","curioso","feliz","surpreso","pensando","piscadinha","serio"};
  return code<8?names[code]:"auto";
}
// Explicit upper bounds keep invalid commands out of the drawing task.
static bool validCommand(const char *name,uint32_t &out){
  int value=expressionCode(name);if(value<0)return false;out=(uint32_t)value;return true;
}

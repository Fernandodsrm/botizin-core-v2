#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct PeerStatusFrame {
  char version[16], ip[16], running[128], boot[128], state[24], reset[40], internet[80];
  uint32_t uptime;
  char bootId[24];bool diagnostics=false;
  uint32_t free=0,minimum=0,largest=0,psram=0,otaFree=0,sampleAge=0,loopMax=0;
  uint32_t gitMax=0,tbMax=0,rpcMax=0,oledMax=0,sampleMax=0;

};

// Read only the current-status block. Persisted OTA journal is never current state.
static bool peerField(const char *body, const char *key, char *out, size_t capacity) {
  const size_t n = strlen(key);
  const char *line = strchr(body, '\n');
  if (!line) return false;
  for (++line; *line && *line != '\n' && *line != '\r';) {
    const char *end = strchr(line, '\n');
    if (!end) end = line + strlen(line);
    size_t len = end - line;
    if (len && line[len - 1] == '\r') --len;
    if (len > n + 2 && !strncmp(line, key, n) && line[n] == ':' && line[n + 1] == ' ') {
      const size_t size = len - n - 2;
      if (!size || size >= capacity) return false;
      memcpy(out, line + n + 2, size); out[size] = 0; return true;
    }
    line = *end ? end + 1 : end;
  }
  return false;
}

static bool peerNumber(const char*body,const char*key,uint32_t &out){char value[24];
  if(!peerField(body,key,value,sizeof(value)))return false;
  for(const char*p=value;*p;++p)if(*p<'0'||*p>'9')return false;
  char*end=nullptr;unsigned long long n=strtoull(value,&end,10);if(!end||*end||n>UINT32_MAX)return false;
  out=(uint32_t)n;return true;
}
static void parsePeerDiagnostics(const char *body,PeerStatusFrame &out){uint32_t schema=0;
  out.diagnostics=peerNumber(body,"DIAG_SCHEMA",schema)&&schema==1&&
    peerField(body,"BOOT_ID",out.bootId,sizeof(out.bootId))&&
    peerNumber(body,"DIAG_FREE",out.free)&&peerNumber(body,"DIAG_MIN",out.minimum)&&
    peerNumber(body,"DIAG_LARGEST",out.largest)&&peerNumber(body,"DIAG_PSRAM",out.psram)&&
    peerNumber(body,"DIAG_OTA_FREE",out.otaFree)&&peerNumber(body,"DIAG_SAMPLE_AGE",out.sampleAge)&&
    peerNumber(body,"DIAG_LOOP_MAX_US",out.loopMax)&&peerNumber(body,"DIAG_GIT_MAX_US",out.gitMax)&&
    peerNumber(body,"DIAG_TB_MAX_US",out.tbMax)&&peerNumber(body,"DIAG_RPC_MAX_US",out.rpcMax)&&
    peerNumber(body,"DIAG_OLED_MAX_US",out.oledMax)&&peerNumber(body,"DIAG_SAMPLE_MAX_US",out.sampleMax);
}

static bool parsePeerStatus(const char *body, PeerStatusFrame &out) {
  const char *prefix = "BOTIZIN CORE V";
  if (strncmp(body, prefix, strlen(prefix))) return false;
  const char *v = body + strlen(prefix), *end = strchr(v, '\n');
  if (!end) return false;
  size_t len = end - v;
  if (len && v[len - 1] == '\r') --len;
  if (!len || len >= sizeof(out.version)) return false;
  for (size_t i = 0; i < len; ++i) if ((v[i] < '0' || v[i] > '9') && v[i] != '.') return false;
  memcpy(out.version, v, len); out.version[len] = 0;
  char flash[24], wifi[24], uptime[24];
  if (!peerField(body, "FLASH_BYTES", flash, sizeof(flash)) || strcmp(flash, "16777216") ||
      !peerField(body, "WIFI", wifi, sizeof(wifi)) || strcmp(wifi, "OK") ||
      !peerField(body, "UPTIME_SECONDS", uptime, sizeof(uptime))) return false;
  for (const char *p = uptime; *p; ++p) if (*p < '0' || *p > '9') return false;
  char *tail = nullptr; unsigned long long seconds = strtoull(uptime, &tail, 10);
  if (!tail || *tail || seconds > UINT32_MAX) return false;
  out.uptime = seconds;
  parsePeerDiagnostics(body,out);
  return peerField(body, "IP", out.ip, sizeof(out.ip)) &&
         peerField(body, "RUNNING_PARTITION", out.running, sizeof(out.running)) &&
         peerField(body, "BOOT_PARTITION", out.boot, sizeof(out.boot)) &&
         peerField(body, "RUNNING_OTA_STATE", out.state, sizeof(out.state)) &&
         peerField(body, "RESET_REASON", out.reset, sizeof(out.reset)) &&
         peerField(body, "INTERNET_OTA", out.internet, sizeof(out.internet));
}

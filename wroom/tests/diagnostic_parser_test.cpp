#include <cassert>
#include <cstdio>
#include "../firmware/peer_status_parser.h"
int main(){
 const char* body="BOTIZIN CORE V0.0.12\nDIAG_SCHEMA: 1\nBOOT_ID: abc123\nDIAG_FREE: 98000\nDIAG_MIN: 81000\nDIAG_LARGEST: 60000\nDIAG_PSRAM: 8000000\nDIAG_OTA_FREE: 1900000\nDIAG_SAMPLE_AGE: 4\nDIAG_LOOP_MAX_US: 3500000\nDIAG_GIT_MAX_US: 2100000\nDIAG_TB_MAX_US: 800000\nDIAG_RPC_MAX_US: 1200000\nDIAG_PEER_MAX_US: 0\nDIAG_OLED_MAX_US: 0\nDIAG_SAMPLE_MAX_US: 400\n\nLAST_PERSISTED_OTA_ATTEMPT:\nDIAG_FREE: 1\n";
 PeerStatusFrame invalid={};assert(!parsePeerStatus("garbage",invalid));
 PeerStatusFrame f={};parsePeerDiagnostics(body,f);assert(f.diagnostics&&f.free==98000&&f.sampleAge==4&&f.rpcMax==1200000);
 PeerStatusFrame older={};parsePeerDiagnostics("BOTIZIN CORE V0.0.11\nWIFI: OK\n\n",older);assert(!older.diagnostics);
 uint32_t v=42;assert(!peerNumber("HEADER\nVALUE: 4294967296\n","VALUE",v)&&v==42);
 assert(!peerNumber("HEADER\nVALUE: -1\n","VALUE",v));
 assert(!peerNumber("HEADER\n\nHISTORY:\nVALUE: 5\n","VALUE",v));
 puts("DIAGNOSTIC_PARSER_OK: current block, missing schema, overflow and history isolation");
}

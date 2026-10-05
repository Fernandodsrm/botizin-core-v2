#include "../firmware/peer_status_parser.h"
#include <assert.h>
#include <string>
int main() {
  const std::string status = "BOTIZIN CORE V0.0.10\nIP: 192.168.0.36\nUPTIME_SECONDS: 123\nRESET_REASON: SOFTWARE (3)\nFLASH_BYTES: 16777216\nRUNNING_PARTITION: app0 / ota_0 / address=0x00010000 / size=3145728\nBOOT_PARTITION: app0 / ota_0 / address=0x00010000 / size=3145728\nRUNNING_OTA_STATE: VALID\nWIFI: OK\nINTERNET_OTA: UP_TO_DATE 0.0.10\n\nLAST_PERSISTED_OTA_ATTEMPT:\nTARGET_VERSION: 0.0.9\nRUNNING_OTA_STATE: NEW\n";
  PeerStatusFrame f = {};
  assert(parsePeerStatus(status.c_str(), f)); assert(std::string(f.version)=="0.0.10");
  assert(f.uptime==123 && std::string(f.state)=="VALID");
  auto crlf=status;size_t pos=0;while((pos=crlf.find('\n',pos))!=std::string::npos){crlf.insert(pos,"\r");pos+=2;}
  assert(parsePeerStatus(crlf.c_str(), f));
  auto wrong=status;wrong.replace(0,strlen("BOTIZIN CORE"),"BOTIZIN WROOM");assert(!parsePeerStatus(wrong.c_str(),f));
  auto overflow=status;overflow.replace(overflow.find("123"),3,"4294967296");assert(!parsePeerStatus(overflow.c_str(),f));
  auto journal=status;journal.erase(journal.find("RUNNING_OTA_STATE: VALID\n"),strlen("RUNNING_OTA_STATE: VALID\n"));assert(!parsePeerStatus(journal.c_str(),f));
  assert(!parsePeerStatus("BOTIZIN CORE V0.0.10\n",f));
}

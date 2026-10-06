#pragma once
static String peerNonce;
static uint32_t peerNonceUntil=0;
static String jsonText(cJSON *o,const char *key) {
  cJSON *v=cJSON_GetObjectItemCaseSensitive(o,key);return cJSON_IsString(v)?String(v->valuestring):String();
}
static String peerOtaReply(const String &id,const String &result) {
  cJSON *o=cJSON_CreateObject(); if(!o)return "";
  String run=partitionInfo(esp_ota_get_running_partition()),boot=partitionInfo(esp_ota_get_boot_partition());
  String state=stateInfo(esp_ota_get_running_partition());
  bool ok=cJSON_AddStringToObject(o,"board","esp32s3-n16r8") &&
    cJSON_AddStringToObject(o,"origin","ESP32_REAL") && cJSON_AddStringToObject(o,"command_id",id.c_str()) &&
    cJSON_AddStringToObject(o,"result",result.c_str()) && cJSON_AddStringToObject(o,"firmware_version",BOTIZIN_VERSION) &&
    cJSON_AddStringToObject(o,"boot_id",telemetryBootId.c_str()) && cJSON_AddStringToObject(o,"ota_state",state.c_str()) &&
    cJSON_AddStringToObject(o,"running_partition",run.c_str()) && cJSON_AddStringToObject(o,"boot_partition",boot.c_str()) &&
    cJSON_AddStringToObject(o,"manual_status",manualOtaStatus.c_str()) &&
    cJSON_AddStringToObject(o,"internet_status",internetStatus.c_str()) && cJSON_AddStringToObject(o,"phase",s3OtaPhase.c_str()) &&
    cJSON_AddBoolToObject(o,"automatic_enabled",true) && cJSON_AddBoolToObject(o,"candidate_ready",candidateReady()) &&
    cJSON_AddStringToObject(o,"candidate_id",otaCandidateId.c_str()) && cJSON_AddStringToObject(o,"target_version",otaTargetVersion.c_str()) &&
    cJSON_AddStringToObject(o,"sha256",otaTargetSHA.c_str()) && cJSON_AddNumberToObject(o,"size",otaTargetBytes) &&
    cJSON_AddNumberToObject(o,"expires_in_seconds",manualWindowActive()?(otaManualUntil-millis())/1000:0) &&
    cJSON_AddBoolToObject(o,"busy",peerOtaBusy||otaCheckQueued||otaInstallQueued||rebootScheduled) &&
    cJSON_AddNumberToObject(o,"written_bytes",writtenBytes) && cJSON_AddNumberToObject(o,"expected_bytes",expectedBytes);
  char *p=ok?cJSON_PrintUnformatted(o):nullptr;cJSON_Delete(o);
  String s=p?String(p):String();if(p)cJSON_free(p);return s;
}
static void beginPeerOtaServer() {
  const char *headers[]={"X-Botizin-Nonce","X-Botizin-MAC"};server.collectHeaders(headers,2);
  server.on("/peer/challenge",HTTP_GET,[](){
    if(peerKey.length()!=64){server.send(503,"text/plain","PAIR_REQUIRED");return;}
    peerNonce=telemetryBootId+"-"+String((unsigned long)esp_random(),HEX)+String((unsigned long)esp_random(),HEX);
    peerNonceUntil=millis()+10000;server.send(200,"text/plain",peerNonce);
  });
  server.on("/peer/ota",HTTP_POST,[](){
    String body=server.arg("plain"),nonce=server.header("X-Botizin-Nonce");
    if(body.length()>1024 || body.isEmpty() || peerKey.length()!=64 || peerNonce.isEmpty() || nonce!=peerNonce ||
       (int32_t)(peerNonceUntil-millis())<=0 || !peerMACMatches(peerMAC("request\n"+nonce+"\n"+body),server.header("X-Botizin-MAC"))){
      server.send(403,"text/plain","AUTH_REJECTED");return;
    }
    peerNonce="";peerNonceUntil=0; // one-use nonce; never replay a timed-out install
    cJSON *o=cJSON_Parse(body.c_str());String action=jsonText(o,"action"),id=jsonText(o,"command_id");
    if(id.isEmpty()||id.length()>64){cJSON_Delete(o);server.send(400,"text/plain","INVALID_COMMAND");return;}
    String result="OTA_STATUS";bool check=false,install=false,cancel=false;
    bool idle=!peerOtaBusy&&!otaInstallQueued&&!otaCheckQueued&&!uploadActive&&!uploadOK&&!rebootScheduled;
    if(action=="check") {check=idle&&otaReady&&!internetStopped;result=check?"CHECK_ACCEPTED":"BUSY_OR_BLOCKED";}
    else if(action=="confirm") {
      install=idle&&candidateReady()&&otaConfirmationMatches(millis(),otaManualUntil,
        jsonText(o,"candidate_id").c_str(),otaCandidateId.c_str(),jsonText(o,"target_version").c_str(),otaTargetVersion.c_str(),
        jsonText(o,"sha256").c_str(),otaTargetSHA.c_str(),jsonText(o,"boot_id").c_str(),telemetryBootId.c_str());
      result=install?"INSTALL_ACCEPTED":"CONFIRMATION_REJECTED";
    } else if(action=="cancel") {cancel=idle;result=cancel?"CANCEL_ACCEPTED":"BUSY_OR_BLOCKED";}
    else if(action!="status"){cJSON_Delete(o);server.send(400,"text/plain","INVALID_ACTION");return;}
    cJSON_Delete(o);String response=peerOtaReply(id,result);
    if(response.isEmpty()){server.send(503,"text/plain","NO_MEMORY");return;}
    server.sendHeader("X-Botizin-MAC",peerMAC("response\n"+nonce+"\n"+response));
    server.send(200,"application/json",response);
    // Only enqueue after returning acknowledgement. Installation runs in loop, not this handler.
    if(check)queueOtaCheck();else if(install){otaInstallQueued=true;manualOtaStatus="INSTALL_QUEUED";}else if(cancel)cancelOtaCheck();
  });
}

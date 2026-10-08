#pragma once

static String s3OtaStatus="Aguardando S3",s3OtaVersion,s3OtaBoot,s3Candidate,s3Target,s3SHA,s3QueuedAction;
static String s3WatchVersion,s3WatchBoot,s3ConfirmId,s3ConfirmVersion,s3ConfirmSHA,s3ConfirmBoot;
static uint32_t s3CandidateUntil=0,s3WindowUntil=0,s3ReplyAt=0,nextS3OtaPoll=0,s3WatchStarted=0;
static bool s3HaveOtaReply=false,s3Busy=false;
static size_t s3Written=0,s3Expected=0;
static uint8_t oledS3Choice=0;
static bool s3OtaRecent(){return s3HaveOtaReply&&WiFi.status()==WL_CONNECTED&&millis()-s3ReplyAt<15000;}
static bool s3CandidateReady(){return s3OtaRecent()&&!s3Busy&&!s3Candidate.isEmpty()&&(int32_t)(s3CandidateUntil-millis())>0&&s3QueuedAction.isEmpty();}
static void stageS3Confirmation(){
  s3ConfirmId=s3Candidate;s3ConfirmVersion=s3Target;s3ConfirmSHA=s3SHA;s3ConfirmBoot=s3OtaBoot;
}
static bool queueS3Action(const String &action){
  if(!s3QueuedAction.isEmpty()||peerKey.length()!=64||WiFi.status()!=WL_CONNECTED)return false;
  if(action=="confirm"){
    if(!s3CandidateReady() || !otaConfirmationMatches(millis(),s3CandidateUntil,
      s3ConfirmId.c_str(),s3Candidate.c_str(),s3ConfirmVersion.c_str(),s3Target.c_str(),
      s3ConfirmSHA.c_str(),s3SHA.c_str(),s3ConfirmBoot.c_str(),s3OtaBoot.c_str()))return false;
  } else if(action!="check"&&action!="cancel")return false;
  if(s3Busy)return false;
  s3QueuedAction=action;nextS3OtaPoll=0;return true;
}
static String s3JsonText(cJSON *o,const char *key){cJSON*v=cJSON_GetObjectItemCaseSensitive(o,key);return cJSON_IsString(v)?String(v->valuestring):String();}
static bool fetchS3OTA(const String &action){
  DiagnosticScope diagnosticScope(DIAG_PEER);diagnosticScope.failed=true;
  String base=peerBaseURL();if(base.isEmpty()){s3OtaStatus="Endereco S3 pendente";return false;}
  uint32_t started=millis();String nonce;
  {
    NetworkClient c;HTTPClient h;h.setConnectTimeout(350);h.setTimeout(500);h.useHTTP10(true);
    if(h.begin(c,base+"/peer/challenge")){
      int code=h.GET();int size=h.getSize();
      if(code==200&&size>0&&size<=80)nonce=h.getString();
      else s3OtaStatus=code==503?"Pareamento pendente":code==404?"S3 precisa 0.0.11":"S3 sem resposta";
    }h.end();c.stop();
  }
  if(nonce.isEmpty()||nonce.length()>80){invalidatePeerAddress();return false;}
  cJSON *o=cJSON_CreateObject();if(!o)return false;
  String id=telemetryBootId+"-s3-"+String((unsigned long)esp_random(),HEX);
  bool ok=cJSON_AddStringToObject(o,"action",action.c_str())&&cJSON_AddStringToObject(o,"command_id",id.c_str());
  if(action=="confirm")ok=ok&&cJSON_AddStringToObject(o,"candidate_id",s3ConfirmId.c_str())&&
    cJSON_AddStringToObject(o,"target_version",s3ConfirmVersion.c_str())&&cJSON_AddStringToObject(o,"sha256",s3ConfirmSHA.c_str())&&
    cJSON_AddStringToObject(o,"boot_id",s3ConfirmBoot.c_str());
  char *raw=ok?cJSON_PrintUnformatted(o):nullptr;cJSON_Delete(o);if(!raw)return false;
  String body(raw);cJSON_free(raw);String response,mac;
  {
    NetworkClient c;HTTPClient h;h.setConnectTimeout(350);h.setTimeout(700);h.useHTTP10(true);
    const char*headers[]={"X-Botizin-MAC"};h.collectHeaders(headers,1);
    if(h.begin(c,base+"/peer/ota")){
      h.addHeader("Content-Type","application/json");h.addHeader("X-Botizin-Nonce",nonce);
      h.addHeader("X-Botizin-MAC",peerMAC("request\n"+nonce+"\n"+body));
      int code=h.POST((uint8_t*)body.c_str(),body.length()),length=h.getSize();
      if(code==200&&length>0&&length<=2048){response=h.getString();mac=h.header("X-Botizin-MAC");if(response.length()!=(size_t)length)response="";}
      else s3OtaStatus=code==403?"Pareamento invalido":"Sem confirmacao S3";
    }h.end();c.stop();
  }
  if(response.isEmpty()||!peerMACMatches(peerMAC("response\n"+nonce+"\n"+response),mac)){s3Candidate="";return false;}
  o=cJSON_Parse(response.c_str());
  cJSON *expires=cJSON_GetObjectItemCaseSensitive(o,"expires_in_seconds"),*ready=cJSON_GetObjectItemCaseSensitive(o,"candidate_ready");
  cJSON *written=cJSON_GetObjectItemCaseSensitive(o,"written_bytes"),*expected=cJSON_GetObjectItemCaseSensitive(o,"expected_bytes");
  String version=s3JsonText(o,"firmware_version"),boot=s3JsonText(o,"boot_id"),result=s3JsonText(o,"result");
  bool valid=s3JsonText(o,"board")=="esp32s3-n16r8"&&s3JsonText(o,"origin")=="ESP32_REAL"&&
    s3JsonText(o,"command_id")==id&&!version.isEmpty()&&!boot.isEmpty()&&cJSON_IsNumber(expires)&&
    expires->valuedouble>=0&&expires->valuedouble<=300&&cJSON_IsBool(ready)&&cJSON_IsNumber(written)&&cJSON_IsNumber(expected)&&
    written->valuedouble>=0&&written->valuedouble<=3145728&&expected->valuedouble>=0&&expected->valuedouble<=3145728;
  if(!valid){cJSON_Delete(o);s3Candidate="";s3OtaStatus="Resposta invalida";return false;}
  diagnosticScope.failed=false;
  s3OtaVersion=version;s3OtaBoot=boot;s3ReplyAt=millis();s3HaveOtaReply=true;
  s3Busy=cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(o,"busy"));
  s3Written=(size_t)written->valuedouble;s3Expected=(size_t)expected->valuedouble;
  s3OtaStatus=s3JsonText(o,"manual_status");String internet=s3JsonText(o,"internet_status");
  s3Candidate="";uint32_t budget=(uint32_t)expires->valuedouble*1000,elapsed=millis()-started+1000;
  s3WindowUntil=budget>elapsed?millis()+budget-elapsed:0;
  if(cJSON_IsTrue(ready)&&budget>elapsed){
    s3Candidate=s3JsonText(o,"candidate_id");s3Target=s3JsonText(o,"target_version");s3SHA=s3JsonText(o,"sha256");
    if(s3Candidate.isEmpty()||s3Target.isEmpty()||!peerHex(s3SHA,64))s3Candidate="";
    else s3CandidateUntil=millis()+budget-elapsed;
  }
  if(result=="CHECK_ACCEPTED"){s3Candidate="";s3Busy=true;s3OtaStatus="Consulta solicitada";}
  else if(result=="CANCEL_ACCEPTED"){s3Candidate="";s3OtaStatus="Pedido cancelado";}
  else if(result=="INSTALL_ACCEPTED"){
    s3WatchVersion=s3ConfirmVersion;s3WatchBoot=s3ConfirmBoot;s3WatchStarted=millis();s3Candidate="";s3Busy=true;s3OtaStatus="Instalacao aceita";
  }else if(result=="CONFIRMATION_REJECTED"||result=="BUSY_OR_BLOCKED")s3OtaStatus="S3 rejeitou pedido";
  else if(internet.startsWith("UP_TO_DATE"))s3OtaStatus="Sem versao nova";
  else if(s3Busy)s3OtaStatus=s3JsonText(o,"phase");
  // Lost ACK never triggers an automatic retry; prove the final result from authenticated status.
  if(!s3WatchVersion.isEmpty()&&boot!=s3WatchBoot&&version==s3WatchVersion&&
     s3JsonText(o,"ota_state")=="VALID"&&s3JsonText(o,"running_partition")==s3JsonText(o,"boot_partition")){
    s3OtaStatus="Atualizacao concluida";s3WatchVersion="";s3Busy=false;
  }
  if(!s3WatchVersion.isEmpty()&&boot==s3WatchBoot&&(internet.startsWith("STOPPED:")||s3JsonText(o,"manual_status")=="INSTALL_REJECTED")){
    s3WatchVersion="";s3Busy=false;s3OtaStatus="Falha informada S3";
  }
  cJSON_Delete(o);return true;
}
static void pollS3OTA(){
  if(uploadActive||uploadOK||rebootScheduled||ESP.getFreeHeap()<85000)return;
  if(peerKey.length()!=64){s3OtaStatus="Pareamento pendente";return;}
  if(!s3WatchVersion.isEmpty()&&millis()-s3WatchStarted>300000){s3WatchVersion="";s3Busy=false;s3OtaStatus="Resultado nao provado";}
  bool page=navLevel==3&&oledPage==5;
  if(!page&&s3QueuedAction.isEmpty()&&s3WatchVersion.isEmpty())return;
  if((int32_t)(millis()-nextS3OtaPoll)<0)return;
  nextS3OtaPoll=millis()+3000;
  if(WiFi.status()!=WL_CONNECTED){s3OtaStatus="Wi-Fi desconectado";return;}
  String action=s3QueuedAction.isEmpty()?String("status"):s3QueuedAction;s3QueuedAction="";
  if(!fetchS3OTA(action)&&action=="confirm"){
    s3WatchVersion=s3ConfirmVersion;s3WatchBoot=s3ConfirmBoot;s3WatchStarted=millis();s3OtaStatus="Confirmacao incerta";
  }
  if(action!="status")discardNavigation();
}

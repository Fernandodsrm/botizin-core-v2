#pragma once
static bool safeCatalogId(const String &id){
 if(id.isEmpty()||id.length()>24)return false;
 for(size_t i=0;i<id.length();++i)if(!isalnum((unsigned char)id[i])&&id[i]!='-'&&id[i]!='_')return false;
 return true;
}
static bool parseCatalog(const String &body){
 cJSON *root=cJSON_Parse(body.c_str()),*items=cJSON_GetObjectItemCaseSensitive(root,"environments"),*schema=cJSON_GetObjectItemCaseSensitive(root,"schema");
 int count=cJSON_GetArraySize(items);Environment pending[catalogLimit];
 bool valid=cJSON_IsObject(root)&&cJSON_IsNumber(schema)&&schema->valuedouble==1&&cJSON_IsArray(items)&&count>0&&count<=catalogLimit;
 for(int i=0;valid&&i<count;++i){
  cJSON *item=cJSON_GetArrayItem(items,i),*id=cJSON_GetObjectItemCaseSensitive(item,"id"),*title=cJSON_GetObjectItemCaseSensitive(item,"title"),*version=cJSON_GetObjectItemCaseSensitive(item,"version"),*url=cJSON_GetObjectItemCaseSensitive(item,"url"),*sha=cJSON_GetObjectItemCaseSensitive(item,"sha256"),*size=cJSON_GetObjectItemCaseSensitive(item,"size"),*board=cJSON_GetObjectItemCaseSensitive(item,"board"),*protocol=cJSON_GetObjectItemCaseSensitive(item,"return_protocol");
  valid=cJSON_IsString(id)&&cJSON_IsString(title)&&cJSON_IsString(version)&&cJSON_IsString(url)&&cJSON_IsString(sha)&&cJSON_IsNumber(size)&&size->valuedouble==size->valueint&&size->valueint>0&&size->valueint<=0x140000&&cJSON_IsString(board)&&String(board->valuestring)=="esp32-wroom-4mb"&&cJSON_IsNumber(protocol)&&protocol->valuedouble==2;
  if(!valid)break;
  auto &e=pending[i];e.id=id->valuestring;e.title=title->valuestring;e.version=version->valuestring;e.url=url->valuestring;e.sha=sha->valuestring;e.sha.toLowerCase();e.bytes=size->valueint;
  valid=safeCatalogId(e.id)&&e.title.length()>0&&e.title.length()<=32&&e.version.length()>0&&e.version.length()<=16&&e.url.startsWith(BOTIZIN_FIRMWARE_PREFIX)&&e.url.length()<=200&&e.sha.length()==64;
  for(size_t j=0;valid&&j<e.sha.length();++j)valid=isxdigit((unsigned char)e.sha[j]);
  for(int j=0;j<i;++j)if(e.id==pending[j].id)valid=false;
 }
 cJSON_Delete(root);if(!valid)return false;
 for(int i=0;i<count;++i)environments[i]=pending[i];
 environmentCount=count;catalogChoice=0;return true;
}
static void loadCatalog(){
 environments[0].id="ps4";environments[0].title="PS4 + servo";
 environments[1].id="servo";environments[1].title="Servo pelos botoes";
 environments[2].id="led";environments[2].title="LED pelos botoes";
 Preferences p;if(p.begin("catalog",true)){String body=p.getString("json","");p.end();if(body.length()<=4096&&parseCatalog(body))catalogStatus="CACHED";}
}
static void checkCatalog(){
 clearOtaCandidate();oledDetail=false;oledPage=10;navLevel=3;oledMenu=false;
 pauseFace();showOtaProgress("CONSULTANDO LISTA");
 if(WiFi.status()!=WL_CONNECTED||time(nullptr)<1700000000){catalogStatus="OFFLINE_CACHE";return;}
 if(ESP.getFreeHeap()<80000){catalogStatus="LOW_MEMORY_CACHE";return;}
 NetworkClientSecure tls;tls.useBuiltinCACertBundle();tls.setHandshakeTimeout(10);
 HTTPClient http;http.setConnectTimeout(10000);http.setTimeout(10000);http.useHTTP10(true);http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
 if(!http.begin(tls,catalogURL)){catalogStatus="BEGIN_FAILED_CACHE";return;}
 http.addHeader("Cache-Control","no-cache");int code=http.GET(),size=http.getSize();
 String body;if(code==200&&size>0&&size<=4096)body=http.getString();http.end();tls.stop();
 if(body.length()!=(size_t)size||body.length()>4096||!parseCatalog(body)){catalogStatus="REJECTED_CACHE";return;}
 Preferences p;bool saved=p.begin("catalog",false);
 if(saved){if(p.getString("json","")!=body)saved=p.putString("json",body)==body.length();p.end();}
 catalogStatus=saved?"READY_CACHED":"READY_CACHE_SAVE_FAILED";
}
static bool prepareEnvironment(const String &id){
 int i=environmentIndex(id);if(i<0||catalogBusy()||moduleStartQueued||!otaReady||internetStopped)return false;
 auto &e=environments[i];if(!e.bytes||e.sha.length()!=64||WiFi.status()!=WL_CONNECTED)return false;
 clearOtaCandidate();otaEnvironment=e.id;otaTargetVersion=e.version;otaTargetURL=e.url;otaTargetSHA=e.sha;otaTargetBytes=e.bytes;
 otaCandidateId=telemetryBootId+"-"+String((unsigned long)esp_random(),HEX);otaManualUntil=millis()+300000;manualOtaStatus="ENVIRONMENT_AVAILABLE";pauseFace();nextTelemetry=millis();return true;
}
static bool saveInstalledEnvironment(const String &id,const String &version,const String &sha,size_t bytes,const esp_partition_t *part){
 if(!part||!safeCatalogId(id)||sha.length()!=64||bytes>part->size)return false;
 Preferences p;if(!p.begin("environment",false))return false;
 p.putString("id",id);p.putString("version",version);p.putString("sha",sha);p.putUInt("bytes",bytes);p.putUInt("address",part->address);
 bool ok=p.getString("id","")==id&&p.getString("version","")==version&&p.getString("sha","")==sha&&p.getUInt("bytes",0)==bytes&&p.getUInt("address",0)==part->address;p.end();return ok;
}
static cJSON *catalogJSON(){
 cJSON *items=cJSON_CreateArray();if(!items)return nullptr;
 for(uint8_t i=0;i<environmentCount;++i){auto &e=environments[i];cJSON *item=cJSON_CreateObject();
  if(!item||!cJSON_AddStringToObject(item,"id",e.id.c_str())||!cJSON_AddStringToObject(item,"title",e.title.c_str())||!cJSON_AddStringToObject(item,"version",e.version.c_str())||!cJSON_AddBoolToObject(item,"download_available",e.bytes>0)||!cJSON_AddBoolToObject(item,"installed",moduleAvailable&&moduleId==e.id)||!cJSON_AddItemToArray(items,item)){cJSON_Delete(item);cJSON_Delete(items);return nullptr;}
 }return items;
}

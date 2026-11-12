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
// Pinned GitHub catalog from wroom/catalog.json (base-040).
// No unauthenticated LAN catalog update may authorize a new executable image.
static void loadCatalog(){
  struct Entry {const char *id,*title,*version,*sha,*url; size_t bytes;};
  static const Entry entries[]={
    {"ps4","PS4 + servo","0.0.30","225fcc4a132bc8918adb3374b7392e4706c8a6e210b51ee0f1f8a75ff9688c25",
      "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/0.0.30/firmware.bin",761360},
    {"servo","Servo pelos botoes","0.0.31","2f3c152373c097b93941304cec74f2782efdfb30cee50cf8478102fb1a7d20ff",
      "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/0.0.31/firmware.bin",758992},
    {"led","LED pelos botoes","0.0.32","a45d4963571d144b0a316e787b7977e3594cd8c0f9907dceefc207e897d7564b",
      "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/0.0.32/firmware.bin",753328},
    {"a22-expresso","A22 Expresso","0.0.34","ccb52a6ce531775a4d0243d3f02dcb1894583d6dc907c39d124a62e0402fb78a",
      "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/0.0.34/firmware.bin",1021888},
  };
  static_assert(sizeof(entries)/sizeof(entries[0])<=catalogLimit,"catalog overflow");
  environmentCount=sizeof(entries)/sizeof(entries[0]);
  for(uint8_t i=0;i<environmentCount;++i){
    auto &e=environments[i];const auto &x=entries[i];
    e.id=x.id;e.title=x.title;e.version=x.version;e.sha=x.sha;e.url=x.url;e.bytes=x.bytes;
  }
  catalogChoice=0;catalogStatus="CATALOGO_FIXO_GITHUB_040";
}
static void checkCatalog(){
  // Online catalog updates belong to A22. Only pinned images above can be loaded.
  clearOtaCandidate();oledDetail=false;oledPage=10;navLevel=3;oledMenu=false;
  catalogStatus="LISTA_FIXA_040";nextOledRefresh=0;
}
static bool prepareEnvironment(const String &id){
 int i=environmentIndex(id);if(i<0||catalogBusy()||moduleStartQueued||!otaReady)return false;
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

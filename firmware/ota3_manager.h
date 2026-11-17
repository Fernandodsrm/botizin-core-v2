#pragma once
// OTA3: explicit, on-demand catalog and staging in the *inactive* ESP32-S3 OTA slot.
// No scheduled GitHub polling and no automatic boot selection after staging.
// Only this module's worker may use network/flash for catalog/install.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>

static constexpr size_t OTA3_CATALOG_MAX = 4096;
static constexpr size_t OTA3_BIN_MAX = 3145728;
static const char *OTA3_CATALOG_URL =
  "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/candidate/ota3-0.0.29/ota3/catalog.json";
static const char *OTA3_BIN_ORIGIN = "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/";
static portMUX_TYPE ota3Mux = portMUX_INITIALIZER_UNLOCKED;
static bool ota3Busy = false;
static char ota3Stage[30] = "IDLE", ota3Message[90] = "PRONTO", ota3Catalog[OTA3_CATALOG_MAX + 1] = {0};
static char ota3Selected[42] = {0};
static uint32_t ota3Done = 0, ota3Total = 0;
static uint8_t ota3CatalogCount = 0;

static void ota3Progress(const char *stage, const char *message, size_t done = 0, size_t total = 0) {
  portENTER_CRITICAL(&ota3Mux);
  snprintf(ota3Stage, sizeof(ota3Stage), "%s", stage);
  snprintf(ota3Message, sizeof(ota3Message), "%s", message);
  ota3Done = (uint32_t)done;
  ota3Total = (uint32_t)total;
  portEXIT_CRITICAL(&ota3Mux);
}
static void ota3Finish(bool success, const char *message) {
  ota3Progress(success ? "PRONTO" : "ERRO", message);
  // Persist only the latest outcome, not periodic telemetry or full logs.
  Preferences p;
  if (p.begin("ota3-last", false)) {
    p.putString("outcome", String(success ? "OK: " : "ERROR: ") + message);
    p.end();
  }
  portENTER_CRITICAL(&ota3Mux);
  ota3Busy = false;
  portEXIT_CRITICAL(&ota3Mux);
}
static String ota3Status() {
  char stage[30], msg[90]; uint32_t done, total; bool busy; uint8_t count;
  portENTER_CRITICAL(&ota3Mux);
  memcpy(stage, ota3Stage, sizeof(stage));
  memcpy(msg, ota3Message, sizeof(msg));
  done=ota3Done;total=ota3Total;busy=ota3Busy;count=ota3CatalogCount;
  portEXIT_CRITICAL(&ota3Mux);
  return String("STATE: ")+stage+"\nMESSAGE: "+msg+"\nBUSY: "+(busy?"YES":"NO")+
    "\nBYTES: "+String(done)+"/"+String(total)+"\nCATALOG_COUNT: "+String(count)+"\n";
}
static const esp_partition_t *ota3Slot(int slot) {
  if(slot!=0 && slot!=1) return nullptr;
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP,
    slot==0?ESP_PARTITION_SUBTYPE_APP_OTA_0:ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
}
static int ota3RunningSlot() {
  const esp_partition_t *run=esp_ota_get_running_partition();
  if(!run) return -1;
  if(run->subtype==ESP_PARTITION_SUBTYPE_APP_OTA_0) return 0;
  if(run->subtype==ESP_PARTITION_SUBTYPE_APP_OTA_1) return 1;
  return -1;
}
static String ota3SlotSnapshot(int slot) {
  const esp_partition_t *p=ota3Slot(slot), *run=esp_ota_get_running_partition(),
      *boot=esp_ota_get_boot_partition();
  if(!p) return String("APP")+slot+": MISSING\n";
  esp_app_desc_t desc={}; 
  esp_err_t d=esp_ota_get_partition_description(p,&desc);
  esp_ota_img_states_t st=ESP_OTA_IMG_UNDEFINED;
  esp_err_t sr=esp_ota_get_state_partition(p,&st);
  String version=d==ESP_OK?String(desc.version):"INVALID_IMAGE";
  // app description is not proof of full bootability; selection verifies image.
  String state=sr==ESP_OK?String((int)st):"UNDEFINED";
  return String("APP")+slot+"_VERSION: "+version+"\nAPP"+slot+"_STATE_CODE: "+state+
    "\nAPP"+slot+"_RUNNING: "+(run && p->address==run->address?"YES":"NO")+
    "\nAPP"+slot+"_NEXT_BOOT: "+(boot && p->address==boot->address?"YES":"NO")+"\n";
}
static String ota3LastResult() {
  Preferences p; String result="NONE";
  if(p.begin("ota3-last",true)){result=p.getString("outcome","NONE");p.end();}
  return result;
}
static bool ota3ShaValid(const char *sha) {
  if(!sha || strlen(sha)!=64)return false;
  for(int i=0;i<64;i++)if(!isxdigit((unsigned char)sha[i]))return false;
  return true;
}
struct Ota3Entry {
  char id[42]={0}, url[256]={0}, sha[65]={0};
  size_t size=0;
};
static bool ota3ParseEntry(cJSON *o, Ota3Entry &out) {
  if(!cJSON_IsObject(o))return false;
  cJSON *id=cJSON_GetObjectItemCaseSensitive(o,"id");
  cJSON *url=cJSON_GetObjectItemCaseSensitive(o,"url");
  cJSON *sha=cJSON_GetObjectItemCaseSensitive(o,"sha256");
  cJSON *size=cJSON_GetObjectItemCaseSensitive(o,"size");
  cJSON *board=cJSON_GetObjectItemCaseSensitive(o,"board");
  if(!cJSON_IsString(id)||!cJSON_IsString(url)||!cJSON_IsString(sha)||
     !cJSON_IsNumber(size)||!cJSON_IsString(board))return false;
  if(strlen(id->valuestring)<3 || strlen(id->valuestring)>=sizeof(out.id) ||
     strlen(url->valuestring)>=sizeof(out.url) ||
     strcmp(board->valuestring,"esp32s3-n16r8")!=0 ||
     size->valuedouble<4096 || size->valuedouble>OTA3_BIN_MAX ||
     size->valuedouble!=(double)size->valueint ||
     strncmp(url->valuestring, OTA3_BIN_ORIGIN, strlen(OTA3_BIN_ORIGIN))!=0 ||
     !ota3ShaValid(sha->valuestring))return false;
  for(const char *c=id->valuestring;*c;c++)if(!(isalnum((unsigned char)*c)||*c=='.'||*c=='-'||*c=='_'))return false;
  snprintf(out.id,sizeof(out.id),"%s",id->valuestring);
  snprintf(out.url,sizeof(out.url),"%s",url->valuestring);
  snprintf(out.sha,sizeof(out.sha),"%s",sha->valuestring);
  out.size=(size_t)size->valueint;
  return true;
}
static bool ota3ValidateCatalog(const String &body, uint8_t &count) {
  count=0;
  cJSON *root=cJSON_Parse(body.c_str());
  if(!root)return false;
  cJSON *schema=cJSON_GetObjectItemCaseSensitive(root,"schema");
  cJSON *board=cJSON_GetObjectItemCaseSensitive(root,"board");
  cJSON *entries=cJSON_GetObjectItemCaseSensitive(root,"entries");
  bool ok=cJSON_IsNumber(schema)&&schema->valueint==1&&
    cJSON_IsString(board)&&strcmp(board->valuestring,"esp32s3-n16r8")==0&&
    cJSON_IsArray(entries);
  int n=ok?cJSON_GetArraySize(entries):0;
  if(n<1||n>12)ok=false;
  for(int i=0;ok&&i<n;i++) {
    Ota3Entry a;
    ok=ota3ParseEntry(cJSON_GetArrayItem(entries,i),a);
    if(ok)for(int j=0;j<i;j++) {
      cJSON *b=cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(entries,j),"id");
      if(cJSON_IsString(b)&&strcmp(b->valuestring,a.id)==0){ok=false;break;}
    }
  }
  if(ok)count=(uint8_t)n;
  cJSON_Delete(root);
  return ok;
}
static bool ota3OpenHttp(HTTPClient &h, NetworkClientSecure &tls,const char *url) {
  tls.useBuiltinCACertBundle();
  tls.setHandshakeTimeout(15);
  h.setConnectTimeout(15000);
  h.setTimeout(15000);
  h.useHTTP10(true);
  return h.begin(tls,url);
}
static bool ota3DownloadCatalog() {
  ota3Progress("CONSULTANDO","Baixando catalogo do GitHub");
  if(WiFi.status()!=WL_CONNECTED || time(nullptr)<1700000000) {
    ota3Finish(false,"SEM_WIFI_OU_RELOGIO_NTP");return false;
  }
  NetworkClientSecure tls; HTTPClient h;
  if(!ota3OpenHttp(h,tls,OTA3_CATALOG_URL)) {
    ota3Finish(false,"CATALOG_TLS_BEGIN");return false;
  }
  int code=h.GET(); int len=h.getSize();
  if(code!=200||len<=0||len>=(int)OTA3_CATALOG_MAX) {
    h.end();ota3Finish(false,"CATALOG_HTTP_OU_TAMANHO");return false;
  }
  String content=h.getString();h.end();
  uint8_t count=0;
  if(content.length()!=(size_t)len || !ota3ValidateCatalog(content,count)) {
    ota3Finish(false,"CATALOGO_INVALIDO");return false;
  }
  portENTER_CRITICAL(&ota3Mux);
  memcpy(ota3Catalog,content.c_str(),content.length()+1);
  ota3CatalogCount=count;
  portEXIT_CRITICAL(&ota3Mux);
  ota3Finish(true,"CATALOGO_VALIDADO");return true;
}
static bool ota3ReadEntry(const char *id,Ota3Entry &out) {
  if(!id||!*id)return false;
  // Catalog cache is immutable while a worker is running.
  cJSON *root=cJSON_Parse(ota3Catalog);
  if(!root)return false;
  cJSON *entries=cJSON_GetObjectItemCaseSensitive(root,"entries");
  bool found=false;
  if(cJSON_IsArray(entries))for(int i=0;i<cJSON_GetArraySize(entries);i++) {
    Ota3Entry next;
    if(ota3ParseEntry(cJSON_GetArrayItem(entries,i),next) &&
       strcmp(next.id,id)==0){out=next;found=true;break;}
  }
  cJSON_Delete(root);
  return found;
}
static bool ota3StageImage(const char *selected) {
  Ota3Entry entry;
  if(!ota3ReadEntry(selected,entry)){ota3Finish(false,"VERSAO_FORA_DO_CATALOGO");return false;}
  int running=ota3RunningSlot();
  const esp_partition_t *dest=ota3Slot(running==0?1:0);
  const esp_partition_t *bootBefore=esp_ota_get_boot_partition();
  if(!otaReady||running<0||!dest||dest==esp_ota_get_running_partition()||
     !bootBefore||entry.size>dest->size||WiFi.status()!=WL_CONNECTED||
     time(nullptr)<1700000000) {
    ota3Finish(false,"DESTINO_OU_REDE_INVALIDOS");return false;
  }
  ota3Progress("BAIXANDO","Abrindo firmware",0,entry.size);
  NetworkClientSecure tls; HTTPClient h;
  if(!ota3OpenHttp(h,tls,entry.url)){ota3Finish(false,"FIRMWARE_TLS_BEGIN");return false;}
  int code=h.GET();
  if(code!=200 || h.getSize()!=(int)entry.size) {
    h.end();ota3Finish(false,"FIRMWARE_HTTP_OU_TAMANHO");return false;
  }
  esp_ota_handle_t handle=0;
  esp_err_t err=esp_ota_begin(dest,entry.size,&handle);
  if(err!=ESP_OK){h.end();ota3Finish(false,"ESP_OTA_BEGIN_FALHOU");return false;}
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  bool success=mbedtls_sha256_starts(&sha,0)==0;
  unsigned char buffer[4096]; // task-owned, no full BIN in RAM
  uint32_t last=millis(),started=millis();
  size_t total=0;
  NetworkClient *stream=h.getStreamPtr();
  while(success && total<entry.size) {
    if(WiFi.status()!=WL_CONNECTED || millis()-last>15000 ||
       millis()-started>180000) {success=false;break;}
    int ready=stream->available();
    if(ready>0) {
      size_t chunk=min((size_t)ready,min(sizeof(buffer),entry.size-total));
      int got=stream->read(buffer,chunk);
      if(got>0) {
        success=mbedtls_sha256_update(&sha,buffer,got)==0 &&
                esp_ota_write(handle,buffer,got)==ESP_OK;
        if(success) {total+=got;last=millis();
          ota3Progress("GRAVANDO","Download e flash em andamento",total,entry.size);}
      }
    } else if(!h.connected()) {success=false;break;}
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  h.end();
  unsigned char digest[32]={0};
  if(success)success=mbedtls_sha256_finish(&sha,digest)==0;
  mbedtls_sha256_free(&sha);
  char actual[65]={0};
  if(success)for(int i=0;i<32;i++)snprintf(actual+i*2,3,"%02x",digest[i]);
  success=success && total==entry.size && strcasecmp(actual,entry.sha)==0;
  if(!success){
    esp_ota_abort(handle);ota3Finish(false,"TRANSFERENCIA_OU_SHA_INVALIDO");return false;
  }
  ota3Progress("VERIFICANDO","Finalizando e verificando flash",total,entry.size);
  if(esp_ota_end(handle)!=ESP_OK){ota3Finish(false,"IMAGEM_INICIALIZAVEL_INVALIDA");return false;}
  String flashSha;
  esp_app_desc_t desc={};
  success=hashPartition(dest,entry.size,flashSha) && 
    flashSha.equalsIgnoreCase(entry.sha) &&
    esp_ota_get_partition_description(dest,&desc)==ESP_OK;
  // Native esp_ota_end stages only. Boot selection is a separate user action.
  const esp_partition_t *bootAfter=esp_ota_get_boot_partition();
  success=success && bootAfter && bootAfter->address==bootBefore->address;
  if(!success){ota3Finish(false,"FLASH_OU_BOOT_NAO_CONFEREM");return false;}
  ota3Finish(true,running==0?"APP1_GRAVADA_SEM_TROCAR_BOOT":"APP0_GRAVADA_SEM_TROCAR_BOOT");
  return true;
}
static void ota3Worker(void *param) {
  const int kind=(int)(uintptr_t)param;
  char selected[42];
  portENTER_CRITICAL(&ota3Mux);
  memcpy(selected,ota3Selected,sizeof(selected));
  portEXIT_CRITICAL(&ota3Mux);
  if(kind==1)ota3DownloadCatalog();
  else if(kind==2)ota3StageImage(selected);
  else ota3Finish(false,"OPERACAO_INVALIDA");
  vTaskDelete(nullptr);
}
static bool ota3Start(uint8_t kind,const String &id="") {
  if(!otaReady || WiFi.status()!=WL_CONNECTED || 
     rebootScheduled || wifiTrialActive() || peerOtaBusy || uploadActive || uploadOK ||
     otaInstallQueued || otaCheckQueued || (kind!=1 && kind!=2))return false;
  if(kind==2&&(id.length()<3||id.length()>=sizeof(ota3Selected) ||
     !ota3CatalogCount))return false;
  portENTER_CRITICAL(&ota3Mux);
  if(ota3Busy){portEXIT_CRITICAL(&ota3Mux);return false;}
  ota3Busy=true;
  snprintf(ota3Selected,sizeof(ota3Selected),"%s",id.c_str());
  portEXIT_CRITICAL(&ota3Mux);
  ota3Progress("AGUARDANDO","Preparando tarefa");
  BaseType_t result=xTaskCreatePinnedToCore(ota3Worker,"ota3-worker",12288,
                 (void*)(uintptr_t)kind,1,nullptr,0);
  if(result!=pdPASS) {
    ota3Finish(false,"SEM_RAM_PARA_TAREFA");return false;
  }
  return true;
}
static bool ota3IsBusy() {
  portENTER_CRITICAL(&ota3Mux);bool result=ota3Busy;portEXIT_CRITICAL(&ota3Mux);
  return result;
}
static String ota3CatalogResponse() {
  portENTER_CRITICAL(&ota3Mux);
  String result=ota3Catalog[0]?String(ota3Catalog):String("{\"schema\":1,\"entries\":[]}");
  portEXIT_CRITICAL(&ota3Mux);
  return result;
}
static bool ota3SelectBoot(int slot,String &reason) {
  if(!otaReady || ota3IsBusy() || rebootScheduled ||
     peerOtaBusy || uploadActive || uploadOK ||
     wifiTrialActive() || otaCheckQueued || otaInstallQueued) {
    reason="OTA_OCUPADO";return false;
  }
  const esp_partition_t *chosen=ota3Slot(slot);
  if(!chosen) {reason="SLOT_INVALIDO";return false;}
  esp_app_desc_t desc={};
  if(esp_ota_get_partition_description(chosen,&desc)!=ESP_OK) {
    reason="IMAGEM_AUSENTE_OU_INVALIDA";return false;
  }
  esp_err_t e=esp_ota_set_boot_partition(chosen);
  if(e!=ESP_OK) {reason=String("ERRO_BOOT_")+esp_err_to_name(e);return false;}
  const esp_partition_t *boot=esp_ota_get_boot_partition();
  if(!boot || boot->address!=chosen->address) {reason="BOOT_NAO_CONFIRMADO";return false;}
  reason=slot==0?"APP0_SELECIONADA":"APP1_SELECIONADA";
  return true;
}

#pragma once
static constexpr uint8_t catalogLimit=8;
struct Environment {String id,title,version,url,sha;size_t bytes=0;};
static Environment environments[catalogLimit];
static uint8_t environmentCount=3,catalogChoice=0;
static bool catalogRefreshQueued=false;
static String catalogStatus="CACHE_ONLY",otaEnvironment;
static constexpr const char *catalogURL="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/catalog.json";
static int environmentIndex(const String &id){for(uint8_t i=0;i<environmentCount;++i)if(environments[i].id==id)return i;return -1;}
static bool catalogBusy(){return uploadActive||uploadOK||rebootScheduled||otaCheckQueued||otaInstallQueued||catalogRefreshQueued;}
static void checkCatalog();
static bool prepareEnvironment(const String &id);
static cJSON *catalogJSON();
static void loadCatalog();
static bool saveInstalledEnvironment(const String &id,const String &version,const String &sha,size_t bytes,const esp_partition_t *p);
static bool moduleStartQueued=false, moduleAvailable=false;
static String moduleId,moduleVersion;
static String moduleSHA; static size_t moduleBytes=0;
static const esp_partition_t *modulePartition=nullptr;

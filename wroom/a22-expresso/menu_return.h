#pragma once
// Same verified menu anchor and partition checks as the existing environments.
static String digestHex(const unsigned char *bytes) {
 char text[65];for(unsigned i=0;i<32;++i)snprintf(text+i*2,3,"%02x",bytes[i]);text[64]=0;return String(text);
}
static bool armMenuReturn() {
 const esp_partition_t *run=esp_ota_get_running_partition(),*menu=esp_ota_get_next_update_partition(nullptr);
 if(!run||!menu||run->size!=0x140000||menu->size!=0x140000 ||
   !((run->address==0x10000&&menu->address==0x150000)||(run->address==0x150000&&menu->address==0x10000))) {returnError="PARTITIONS";return false;}
 Preferences p;if(!p.begin("module-return",true)){returnError="NO_MENU_RECORD";return false;}
 String record=p.getString("menu","");p.end();int first=record.indexOf('|'),second=record.indexOf('|',first+1);
 if(first<1||second!=first+65||(record.substring(second+1).isEmpty()||record.substring(second+1).length()>16)||record.substring(0,first).toInt()!=(long)menu->address){returnError="WRONG_MENU_RECORD";return false;}
 unsigned char digest[32];
 if(esp_partition_get_sha256(menu,digest)!=ESP_OK||digestHex(digest)!=record.substring(first+1,second)){returnError="MENU_HASH";return false;}
 if(esp_ota_set_boot_partition(menu)!=ESP_OK){returnError="BOOT_SELECTION";return false;}
 const esp_partition_t *boot=esp_ota_get_boot_partition();
 if(!boot||boot->address!=menu->address){returnError="BOOT_READBACK";return false;}
 returnError="READY";return true;
}

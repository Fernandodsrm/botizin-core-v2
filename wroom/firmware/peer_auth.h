#pragma once
#include <mbedtls/md.h>
// Pair key arrives only through authenticated ThingsBoard device RPC. Never compiled/public.
static String peerKey;
static bool peerHex(const String &v, size_t n) {
  if (v.length() != n) return false;
  for (size_t i=0; i<n; ++i) if (!((v[i]>='0'&&v[i]<='9')||(v[i]>='a'&&v[i]<='f'))) return false;
  return true;
}
static void loadPeerKey() {
  Preferences p; if (p.begin("botizin-peer", true)) { peerKey=p.getString("key", ""); p.end(); }
  if (!peerHex(peerKey,64)) peerKey="";
}
static bool savePeerKey(const String &key) {
  if (!peerHex(key,64)) return false;
  if (key==peerKey) return true;
  Preferences p; if (!p.begin("botizin-peer", false)) return false;
  bool ok=p.putString("key",key)==64; p.end();
  if (ok) peerKey=key;
  return ok;
}
static String peerMAC(const String &data) {
  if (!peerHex(peerKey,64)) return "";
  uint8_t key[32],hash[32]; char out[65];
  for(size_t i=0;i<32;++i) key[i]=strtoul(peerKey.substring(i*2,i*2+2).c_str(),nullptr,16);
  if (mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),key,32,
      (const uint8_t*)data.c_str(),data.length(),hash)!=0) return "";
  for(size_t i=0;i<32;++i) snprintf(out+i*2,3,"%02x",hash[i]);
  return String(out);
}
static bool peerMACMatches(const String &expected,const String &actual) {
  if (!peerHex(expected,64)||!peerHex(actual,64)) return false;
  uint8_t diff=0;for(size_t i=0;i<64;++i)diff|=expected[i]^actual[i];return diff==0;
}

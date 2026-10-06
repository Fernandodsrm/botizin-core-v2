#include <string>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstdint>
struct String:std::string {
 using std::string::string;String(const std::string&s):std::string(s){}
 bool isEmpty()const{return empty();}size_t length()const{return size();}
 String substring(size_t a,size_t b)const{return substr(a,b-a);}
};
static String saved;static int writes=0;
struct Preferences {
 bool begin(const char*,bool){return true;}void end(){}
 String getString(const char*,const char*){return saved;}
 size_t putString(const char*,const String&s){saved=s;++writes;return s.length();}
};
#include "../firmware/peer_auth.h"
int main(){
 loadPeerKey();assert(peerKey.empty()&&peerMAC("x").empty());
 assert(!savePeerKey("bad")&&writes==0);
 String k="000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
 assert(savePeerKey(k)&&writes==1);assert(savePeerKey(k)&&writes==1);
 auto mac=peerMAC("request\nnonce\n{\"action\":\"check\"}");
 assert(mac=="6f64d2ac2b089c97a32d4bc2d43ef260168de1d69d5dcc21c64d06bdee81e33e");
 assert(peerMACMatches(mac,mac));assert(!peerMACMatches(mac,peerMAC("request\nnonce2\n{\"action\":\"check\"}")));
 assert(!peerMACMatches(mac,peerMAC("request\nnonce\n{\"action\":\"confirm\"}")));
 assert(!peerMACMatches(mac,peerMAC("response\nnonce\n{\"action\":\"check\"}")));
 assert(!peerMACMatches("",mac));peerKey="";loadPeerKey();assert(peerKey==k);
 puts("PEER_AUTH_TESTS_OK: known HMAC vector, nonce/body/direction binding, malformed keys, NVS persistence");
}

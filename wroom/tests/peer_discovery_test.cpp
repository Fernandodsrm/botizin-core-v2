#include <cassert>
#include "../firmware/peer_discovery_policy.h"
int main(){
 PeerDiscoveryPolicy p;assert(p.due(0));p.attempted(0,false);
 assert(!p.due(29999));assert(p.due(30000));
 p.attempted(30000,true);assert(!p.due(90000));
 p.invalidate();assert(p.due(90000));p.attempted(90000,false);
 p.invalidate();assert(!p.due(119999));assert(p.due(120000));
 p.reset(0xfffffff0);assert(p.due(0xfffffff0));p.attempted(0xfffffff0,false);
 assert(!p.due(0xfffffff1));assert(!p.due(29982));assert(p.due(29984));
}

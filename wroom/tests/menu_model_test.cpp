#include <cassert>
#include "../menu-trial/menu_model.h"
int main() {
  MenuModel m;
  assert(!m.handle(2) && m.selected == 4);
  assert(!m.handle(1) && m.confirmingReturn);
  assert(!m.handle(4 | 1) && !m.detail);
  assert(!m.handle(1));
  assert(m.handle(1));
  MenuModel n;
  for (int i=0;i<5;++i) assert(!n.handle(8));
  assert(n.selected==0 && !n.handle(1) && !n.confirmingReturn);
  assert(!n.handle(1));
  assert(n.ledOn);
  assert(!n.handle(1) && !n.ledOn);
  assert(!n.handle(1) && n.ledOn);
  assert(!n.handle(4 | 1) && !n.ledOn && !n.detail);
}

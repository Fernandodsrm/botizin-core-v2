#pragma once
// Existing V5 buttons: OK25, UP27, BACK32, DOWN33, active LOW.
// Interrupt only records a press; no network, I2C, allocation or flash in ISR.
struct NavButton { uint8_t pin, flag; volatile uint32_t lastPress; };
static NavButton navButtons[] = {{25, 1, 0}, {27, 2, 0}, {32, 4, 0}, {33, 8, 0}};
static volatile uint8_t navPending = 0;
static portMUX_TYPE navMux = portMUX_INITIALIZER_UNLOCKED;

static void ARDUINO_ISR_ATTR navPress(void *argument) {
  NavButton *button = static_cast<NavButton *>(argument);
  uint32_t now = millis();
  portENTER_CRITICAL_ISR(&navMux);
  if ((uint32_t)(now - button->lastPress) >= 150) {
    button->lastPress = now; navPending |= button->flag;
  }
  portEXIT_CRITICAL_ISR(&navMux);
}

static void beginNavigation() {
  for (auto &button : navButtons) {
    pinMode(button.pin, INPUT_PULLUP);
    attachInterruptArg(button.pin, navPress, &button, FALLING);
  }
  Serial.println("BUTTONS: OK25 UP27 BACK32 DOWN33; navigation only");
}

static void pollNavigation() {
  portENTER_CRITICAL(&navMux);
  uint8_t pending = navPending; navPending = 0;
  portEXIT_CRITICAL(&navMux);
  // Discard presses during firmware writes; buttons cannot start any OTA in 0.0.5.
  if (!pending || uploadActive || uploadOK || rebootScheduled) return;
  if (pending & 4) { oledPage = 0; oledDetail = false; }
  else if (pending & (2 | 8)) { oledPage = oledPage == 0 ? 1 : 0; oledDetail = false; }
  else if (pending & 1) oledDetail = !oledDetail;
  if (oledPage == 1) nextPeerPoll = millis();
  nextOledRefresh = 0;
  Serial.printf("NAV: %s / %s\n", oledPage == 0 ? "WROOM" : "S3", oledDetail ? "DETAIL" : "STATUS");
}

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
  Serial.println("BUTTONS: OK25 UP27 BACK32 DOWN33; navigation + explicit OTA query/confirmation");
}

static void discardNavigation() {
  portENTER_CRITICAL(&navMux); navPending = 0; portEXIT_CRITICAL(&navMux);
}

static void pollNavigation() {
  portENTER_CRITICAL(&navMux);
  uint8_t pending = navPending; navPending = 0;
  portEXIT_CRITICAL(&navMux);
  // Discard presses during firmware writes and pending actions.
  if (!pending || uploadActive || uploadOK || rebootScheduled || otaCheckQueued || otaInstallQueued || !s3QueuedAction.isEmpty()) return;
  if (pending & 4) {
    if ((oledPage == 2 || oledPage == 5) && oledDetail && !oledMenu) oledDetail = false;
    else { oledMenu = true; oledDetail = false; }
  } else if (oledMenu) {
    if (pending & (2 | 8)) oledMenuChoice = (oledMenuChoice + ((pending & 8) ? 1 : 5)) % 6;
    else if (pending & 1) {
      const uint8_t pages[] = {3, 0, 1, 2, 5, 4};
      oledPage = pages[oledMenuChoice]; oledMenu = false; oledDetail = false;
    }
  } else if (oledPage == 2) {
    if (pending & (2 | 8)) {
      oledDetail = false;
      oledOtaChoice = (oledOtaChoice + ((pending & 8) ? 1 : 2)) % 3;
    } else if (pending & 1) {
      if (oledOtaChoice == 0) { oledDetail = false; queueOtaCheck(); }
      else if (oledOtaChoice == 1 && candidateReady()) {
        if (!oledDetail) oledDetail = true;
        else { confirmOta(otaCandidateId, otaTargetVersion, otaTargetSHA, telemetryBootId); oledDetail = false; }
      } else if (oledOtaChoice == 2 && manualWindowActive()) { cancelOtaCheck(); oledDetail = false; }
    }
  } else if (oledPage == 5) {
    if (pending & (2 | 8)) { oledDetail = false; oledS3Choice = (oledS3Choice + ((pending & 8) ? 1 : 2)) % 3; }
    else if (pending & 1) {
      if (oledS3Choice == 0) { oledDetail=false; queueS3Action("check"); }
      else if (oledS3Choice == 1 && s3CandidateReady()) {
        if (!oledDetail) { oledDetail=true; stageS3Confirmation(); }
        else { queueS3Action("confirm"); oledDetail=false; }
      } else if (oledS3Choice == 2) { queueS3Action("cancel"); oledDetail=false; }
    }
  } else if (pending & (2 | 8)) {
    const uint8_t pages[] = {3, 0, 1, 2, 5, 4};
    uint8_t index = 0; while (index < 5 && pages[index] != oledPage) ++index;
    index = (index + ((pending & 8) ? 1 : 5)) % 6;
    oledPage = pages[index]; oledMenuChoice = index; oledDetail = false;
  } else if ((pending & 1) && oledPage != 4) oledDetail = !oledDetail;
  if (oledPage == 1 && !oledMenu) nextPeerPoll = millis();
  nextOledRefresh = 0;
  Serial.printf("NAV: page=%u menu=%u choice=%u detail=%u\n", oledPage, oledMenu, oledOtaChoice, oledDetail);
}

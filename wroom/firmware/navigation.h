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
  portENTER_CRITICAL(&navMux);uint8_t pending=navPending;navPending=0;portEXIT_CRITICAL(&navMux);
  if(!pending)return;
  faceLastActivity=millis();
  if(uploadActive||uploadOK||rebootScheduled||otaCheckQueued||otaInstallQueued||catalogRefreshQueued)return;
  if(faceActive.exchange(false)){
    navLevel=0;navGroup=0;oledPage=0;oledMenu=true;oledDetail=false;nextOledRefresh=0;return;
  }
  if(pending&4){
    if(oledDetail){oledDetail=false;if(oledPage==11)clearOtaCandidate();}
    else if(oledPage==11){oledPage=10;oledDetail=false;}
    else if(oledPage==10){navLevel=0;}
    else if(navLevel==3){navLevel=navGroup==1?2:0;}
    else if(navLevel){navLevel=0;}
  }else if(navLevel==0){
    if(pending&(2|8))oledMenuChoice=(oledMenuChoice+((pending&8)?1:3))%4;
    else if(pending&1){navGroup=oledMenuChoice;navLevel=navGroup==1?2:3;oledPage=navGroup==0?8:navGroup==3?10:2;}
  }else if(navLevel==2){
    if(pending&(2|8))navDiagnostic=(navDiagnostic+((pending&8)?1:3))%4;
    else if(pending&1){navLevel=3;oledPage=7;}
  }else if(oledPage==10){
    if(pending&(2|8))catalogChoice=(catalogChoice+((pending&8)?1:environmentCount))%(environmentCount+1);
    else if(pending&1){
      if(catalogChoice==0){if(!catalogBusy()&&!moduleStartQueued)catalogRefreshQueued=true;}
      else{oledPage=11;oledDetail=false;}
    }
  }else if(oledPage==11 && catalogChoice>0 && catalogChoice<=environmentCount){
    auto &e=environments[catalogChoice-1];
    if(pending&1){
      if(moduleAvailable&&moduleId==e.id){if(!oledDetail)oledDetail=true;else{queueModuleStart(telemetryBootId,moduleSHA);oledDetail=false;}}
      else if(oledDetail&&candidateReady()&&otaEnvironment==e.id){confirmOta(otaCandidateId,otaTargetVersion,otaTargetSHA,telemetryBootId);oledDetail=false;}
      else{oledDetail=prepareEnvironment(e.id);}
    }
  }else if(oledPage==2){
    uint8_t &choice=oledOtaChoice;
    if(pending&(2|8)){oledDetail=false;choice=(choice+((pending&8)?1:3))%4;}
    else if(pending&1){

        if(choice==3){oledDetail=false;oledPage=10;}
        else if(choice==0){oledDetail=false;queueOtaCheck();}
        else if(choice==1&&candidateReady()){
          if(!oledDetail)oledDetail=true;
          else{confirmOta(otaCandidateId,otaTargetVersion,otaTargetSHA,telemetryBootId);oledDetail=false;}
        }else if(choice==2&&manualWindowActive()){cancelOtaCheck();oledDetail=false;}

    }
  }
  oledMenu=navLevel==0;nextOledRefresh=0;
}

static void pollFaceIdle(){
  bool busy=uploadActive||uploadOK||rebootScheduled||otaCheckQueued||otaInstallQueued||moduleStartQueued||catalogRefreshQueued||candidateReady()||oledDetail;
  if(busy){faceLastActivity=millis();return;}
  if(faceIdleDue(millis(),faceLastActivity,false))faceActive.store(true);
}

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
  if(!pending||uploadActive||uploadOK||rebootScheduled||otaCheckQueued||otaInstallQueued||!s3QueuedAction.isEmpty())return;
  if(pending&4){
    if(oledDetail){oledDetail=false;}
    else if(navLevel==3){navLevel=navGroup==4?0:navGroup==1?2:1;}
    else if(navLevel){--navLevel;}
  }else if(navLevel==0){
    if(pending&(2|8))oledMenuChoice=(oledMenuChoice+((pending&8)?1:4))%5;
    else if(pending&1){navGroup=oledMenuChoice;navLevel=navGroup==4?3:1;oledPage=4;}
  }else if(navLevel==1){
    if(pending&(2|8))navBoard=1-navBoard;
    else if(pending&1){
      if(navGroup==1)navLevel=2;
      else{navLevel=3;oledPage=navGroup==0?8:navGroup==2?(navBoard?5:2):9;}
      if(navBoard)nextPeerPoll=millis();
    }
  }else if(navLevel==2){
    if(pending&(2|8))navDiagnostic=(navDiagnostic+((pending&8)?1:3))%4;
    else if(pending&1){navLevel=3;oledPage=7;if(navBoard)nextPeerPoll=millis();}
  }else if(oledPage==2||oledPage==5){
    uint8_t &choice=oledPage==2?oledOtaChoice:oledS3Choice;
    if(pending&(2|8)){oledDetail=false;choice=(choice+((pending&8)?1:2))%3;}
    else if(pending&1){
      if(oledPage==2){
        if(choice==0){oledDetail=false;queueOtaCheck();}
        else if(choice==1&&candidateReady()){
          if(!oledDetail)oledDetail=true;
          else{confirmOta(otaCandidateId,otaTargetVersion,otaTargetSHA,telemetryBootId);oledDetail=false;}
        }else if(choice==2&&manualWindowActive()){cancelOtaCheck();oledDetail=false;}
      }else{
        if(choice==0){oledDetail=false;queueS3Action("check");}
        else if(choice==1&&s3CandidateReady()){
          if(!oledDetail){oledDetail=true;stageS3Confirmation();}
          else{queueS3Action("confirm");oledDetail=false;}
        }else if(choice==2&&s3OtaRecent()&&s3WindowUntil&&(int32_t)(s3WindowUntil-millis())>0){queueS3Action("cancel");oledDetail=false;}
      }
    }
  }
  oledMenu=navLevel==0;nextOledRefresh=0;
}

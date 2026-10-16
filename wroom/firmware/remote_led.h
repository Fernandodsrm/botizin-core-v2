#pragma once
// Existing external LED on GPIO26. Idempotent command; no NVS writes.
static bool remoteLedOn=false;
static uint32_t remoteLedOffAt=0;
static void setRemoteLed(bool enabled) {
  remoteLedOn=enabled;
  digitalWrite(26,enabled?HIGH:LOW);
  remoteLedOffAt=enabled?millis()+30000:0;
}
static void beginRemoteLed() {
  digitalWrite(26,LOW);
  pinMode(26,OUTPUT);
  setRemoteLed(false);
}
static void pollRemoteLed() {
  if(remoteLedOn && (int32_t)(millis()-remoteLedOffAt)>=0) setRemoteLed(false);
}

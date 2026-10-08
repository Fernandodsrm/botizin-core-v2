#pragma once
// Explicit OTA commands acknowledge first; queued work runs after TLS is released.
uint32_t nextPingPoll = 0;
uint8_t pingFailures = 0;
bool pingStopped = false;
String pingStatus = "WAITING";

void __attribute__((noinline)) pollPing() {
  if (!provisioned || pingStopped || rebootScheduled || uploadActive || uploadOK || !otaReady ||
      WiFi.status() != WL_CONNECTED || time(nullptr) < 1700000000) return;
  if (ESP.getFreeHeap() < 80000) { pingStatus = "SKIPPED_LOW_HEAP"; return; }
  DiagnosticScope diagnosticScope(DIAG_RPC);
  String body;
  int code = -1000;
  String base = "https://thingsboard.cloud/api/v1/";
  base += BOTIZIN_TB_DEVICE_TOKEN;
  {
    NetworkClientSecure tls;
    tls.useBuiltinCACertBundle(); tls.setHandshakeTimeout(5);
    HTTPClient http;
    http.setConnectTimeout(5000); http.setTimeout(5000);
    http.useHTTP10(true); http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (http.begin(tls, base + "/rpc?timeout=1000")) {
      code = http.GET();
      int size = http.getSize();
      if (code == 200 && size > 0 && size <= 1024) {
        body = http.getString();
        if (body.length() != (size_t)size) body = "";
      }
    }
    http.end(); tls.stop();
  }
  // No command is a normal outcome; it must not pause the receiver.
  if (code == 408 || code == 204) { pingFailures = 0; pingStatus = "READY"; return; }
  if (code != 200 || body.isEmpty()) {
    diagnosticScope.failed=true;
    pingStatus = "POLL_HTTP_" + String(code);
    if (++pingFailures >= 3) { pingFailures = 3; nextPingPoll = millis()+60000; pingStatus = "RETRY_AFTER_60_SECONDS"; }
    Serial.println("PING_RPC: " + pingStatus); return;
  }
  cJSON *root = cJSON_Parse(body.c_str());
  if (!root) { diagnosticScope.failed=true;pingStatus = "INVALID_JSON"; return; }
  cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
  cJSON *method = cJSON_GetObjectItemCaseSensitive(root, "method");
  cJSON *params = cJSON_GetObjectItemCaseSensitive(root, "params");
  cJSON *command = cJSON_GetObjectItemCaseSensitive(params, "command_id");
  cJSON *issued = cJSON_GetObjectItemCaseSensitive(params, "issued_at_ms");
  double now = (double)time(nullptr) * 1000.0;
  bool valid = cJSON_IsNumber(id) && id->valuedouble >= 0 &&
    id->valuedouble <= 2147483647.0 && id->valuedouble == (double)id->valueint &&
    cJSON_IsString(method) && (strcmp(method->valuestring, "ping") == 0 ||
      strcmp(method->valuestring, "ota_check") == 0 || strcmp(method->valuestring, "ota_status") == 0 ||
      strcmp(method->valuestring, "ota_confirm") == 0 || strcmp(method->valuestring, "ota_cancel") == 0 || strcmp(method->valuestring, "peer_pair") == 0 || strcmp(method->valuestring, "peer_address") == 0 || strcmp(method->valuestring, "ota_automatic") == 0 || strcmp(method->valuestring, "ps4_config") == 0 || strcmp(method->valuestring, "module_start") == 0) &&
    cJSON_IsObject(params) && cJSON_IsString(command) &&
    strlen(command->valuestring) > 0 && strlen(command->valuestring) <= 64 &&
    cJSON_IsNumber(issued) && issued->valuedouble <= now + 30000.0 &&
    issued->valuedouble >= now - 90000.0;
  if (!valid) { cJSON_Delete(root); pingStatus = "REJECTED_COMMAND"; Serial.println("PING_RPC: " + pingStatus); return; }
  int requestId = id->valueint;
  String commandId(command->valuestring);
  String action(method->valuestring), result = action == "ping" ? "PONG" : "OTA_STATUS";
  bool accepted = false;
  if(action=="module_start") {
    cJSON *boot=cJSON_GetObjectItemCaseSensitive(params,"boot_id"), *sha=cJSON_GetObjectItemCaseSensitive(params,"sha256");
    accepted=moduleAvailable && !otaInstallQueued && !otaCheckQueued && !moduleStartQueued && cJSON_IsString(boot) && cJSON_IsString(sha) && String(boot->valuestring)==telemetryBootId && String(sha->valuestring)==moduleSHA;
    result=accepted?"MODULE_START_ACCEPTED":"MODULE_START_REJECTED";
  } else if(action=="ps4_config") {
    cJSON *button=cJSON_GetObjectItemCaseSensitive(params,"led_button");
    accepted=cJSON_IsNumber(button) && button->valuedouble==button->valueint && savePS4Button(button->valueint);
    result=accepted?"PS4_CONFIG_SAVED":"PS4_CONFIG_REJECTED";
  } else if(action=="ota_automatic") {
    cJSON *enabled=cJSON_GetObjectItemCaseSensitive(params,"enabled");
    accepted=cJSON_IsBool(enabled) && !otaInstallQueued && !otaCheckQueued && saveAutomatic(cJSON_IsTrue(enabled));
    result=accepted?"AUTOMATIC_SAVED":"AUTOMATIC_REJECTED";
  } else if(action=="peer_address") {
    cJSON *ip=cJSON_GetObjectItemCaseSensitive(params,"ip");
    accepted=cJSON_IsString(ip) && savePeerAddress(String(ip->valuestring));
    result=accepted?"PEER_ADDRESS_SAVED":"PEER_ADDRESS_REJECTED";
  } else if (action == "peer_pair") {
    cJSON *k = cJSON_GetObjectItemCaseSensitive(params, "key");
    accepted = cJSON_IsString(k) && savePeerKey(String(k->valuestring));
    result = accepted ? "PAIR_READY" : "PAIR_REJECTED";
  } else if (action == "ota_check") {
    accepted = otaReady && !internetStopped && !otaCheckQueued && !otaInstallQueued && !uploadActive && !uploadOK && !rebootScheduled;
    result = accepted ? "CHECK_ACCEPTED" : "BUSY_OR_BLOCKED";
  } else if (action == "ota_confirm") {
    cJSON *cid = cJSON_GetObjectItemCaseSensitive(params, "candidate_id");
    cJSON *v = cJSON_GetObjectItemCaseSensitive(params, "target_version");
    cJSON *sha = cJSON_GetObjectItemCaseSensitive(params, "sha256");
    cJSON *bid = cJSON_GetObjectItemCaseSensitive(params, "boot_id");
    accepted = candidateReady() && cJSON_IsString(cid) && cJSON_IsString(v) && cJSON_IsString(sha) && cJSON_IsString(bid) &&
      otaConfirmationMatches(millis(), otaManualUntil, cid->valuestring, otaCandidateId.c_str(), v->valuestring, otaTargetVersion.c_str(), sha->valuestring, otaTargetSHA.c_str(), bid->valuestring, telemetryBootId.c_str());
    result = accepted ? "INSTALL_ACCEPTED" : "CONFIRMATION_REJECTED";
  } else if (action == "ota_cancel") {
    accepted = !otaInstallQueued && !uploadActive && !uploadOK && !rebootScheduled;
    result = accepted ? "CANCEL_ACCEPTED" : "BUSY_OR_BLOCKED";
  }
  cJSON_Delete(root);
  cJSON *reply = cJSON_CreateObject();
  if (!reply) { pingStatus = "NO_REPLY_MEMORY"; return; }
  const esp_partition_t *run = esp_ota_get_running_partition();
  String running = partitionInfo(run), boot = partitionInfo(esp_ota_get_boot_partition());
  String state = stateInfo(run), reset = resetInfo();
  bool ok = cJSON_AddStringToObject(reply, "result", result.c_str()) &&
    cJSON_AddStringToObject(reply, "origin", "ESP32_REAL") &&
    cJSON_AddStringToObject(reply, "command_id", commandId.c_str()) &&
    cJSON_AddNumberToObject(reply, "request_id", requestId) &&
    cJSON_AddStringToObject(reply, "firmware_version", BOTIZIN_VERSION) &&
    cJSON_AddNumberToObject(reply,"ps4_led_button",ps4LedButton) &&
    cJSON_AddBoolToObject(reply,"module_available",moduleAvailable) &&
    cJSON_AddStringToObject(reply,"slot_other_environment",moduleAvailable?"PS4 0.0.21":"Sem modulo confirmado") &&
    cJSON_AddStringToObject(reply,"module_sha256",moduleSHA.c_str()) &&
    cJSON_AddStringToObject(reply,"wifi_ip",WiFi.localIP().toString().c_str()) &&
    cJSON_AddStringToObject(reply,"local_name",BOTIZIN_LOCAL_NAME ".local") &&
    cJSON_AddBoolToObject(reply,"mdns_ready",localNameReady) &&
    cJSON_AddStringToObject(reply,"s3_resolved_ip",peerAddress.toString().c_str()) &&
    cJSON_AddStringToObject(reply,"s3_address_source",peerAddressSource.c_str()) &&
    cJSON_AddStringToObject(reply,"s3_manual_ip",peerManualIP.c_str()) &&
    cJSON_AddStringToObject(reply, "boot_id", telemetryBootId.c_str()) &&
    cJSON_AddNumberToObject(reply, "uptime_seconds", millis() / 1000) &&
    cJSON_AddStringToObject(reply, "running_partition", running.c_str()) &&
    cJSON_AddStringToObject(reply, "boot_partition", boot.c_str()) &&
    cJSON_AddStringToObject(reply, "ota_state", state.c_str()) &&
    cJSON_AddStringToObject(reply, "reset_reason", reset.c_str());
  if (action != "ping") {
    ok = ok && cJSON_AddBoolToObject(reply, "automatic_enabled", otaAutomaticEnabled) &&
      cJSON_AddStringToObject(reply, "manual_status", manualOtaStatus.c_str()) &&
      cJSON_AddStringToObject(reply, "internet_status", internetStatus.c_str()) &&
      cJSON_AddBoolToObject(reply, "candidate_ready", candidateReady()) &&
      cJSON_AddStringToObject(reply, "candidate_id", otaCandidateId.c_str()) &&
      cJSON_AddStringToObject(reply, "target_version", otaTargetVersion.c_str()) &&
      cJSON_AddStringToObject(reply, "sha256", otaTargetSHA.c_str()) &&
      cJSON_AddNumberToObject(reply, "size", otaTargetBytes) &&
      cJSON_AddNumberToObject(reply, "expires_in_seconds", manualWindowActive() ? (otaManualUntil - millis()) / 1000 : 0);
  }
  char *payload = ok ? cJSON_PrintUnformatted(reply) : nullptr;
  cJSON_Delete(reply);
  if (!payload) { pingStatus = "NO_REPLY_MEMORY"; return; }
  int posted = -1000;
  {
    NetworkClientSecure tls;
    tls.useBuiltinCACertBundle(); tls.setHandshakeTimeout(5);
    HTTPClient http;
    http.setConnectTimeout(5000); http.setTimeout(5000);
    http.useHTTP10(true); http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (http.begin(tls, base + "/rpc/" + String(requestId))) {
      http.addHeader("Content-Type", "application/json");
      posted = http.POST((uint8_t *)payload, strlen(payload));
    }
    http.end(); tls.stop();
  }
  diagnosticScope.failed = posted < 200 || posted >= 300;
  cJSON_free(payload);
  pingStatus = "REPLY_HTTP_" + String(posted);
  Serial.println("PING_RPC: " + commandId + " " + pingStatus);
  if (posted >= 200 && posted < 300) {
    pingFailures = 0;
    // A failed acknowledgement never starts an installation.
    if(accepted && action=="module_start") {
      queueModuleStart(telemetryBootId,moduleSHA);
    } else if (accepted && action == "ota_check") queueOtaCheck();
    else if (accepted && action == "ota_confirm" && candidateReady()) {
      otaInstallQueued = true; manualOtaStatus = "INSTALL_QUEUED";
    } else if (accepted && action == "ota_cancel") cancelOtaCheck();
  }
  else if (++pingFailures >= 3) { pingFailures = 3; nextPingPoll = millis()+60000; pingStatus = "RETRY_AFTER_60_SECONDS"; }
}

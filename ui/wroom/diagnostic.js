self.onInit = function () {
  self.root = self.ctx.$container[0];
  self.values = {}; self.lastSeen = 0;
  self.timer = setInterval(function () { renderPresence(); }, 1000);
  self.onDataUpdated();
};
self.onDataUpdated = function () {
  if (!self.root) return;
  self.values = {};
  (self.ctx.data || []).forEach(function (row) {
    if (!row.data || !row.data.length) return;
    var point = row.data[row.data.length - 1];
    if (point[1] === null || point[1] === undefined) return;
    self.values[row.dataKey.name] = { value: point[1], ts: Number(point[0]) };
  });
  var v = self.values;
  function value(key) { return v[key] ? String(v[key].value) : 'Aguardando dados'; }
  var real = value('origin') === 'ESP32_REAL' && value('simulated') === 'false';
  self.lastSeen = real && v.firmware_version ? v.firmware_version.ts : 0;
  put('version', value('firmware_version'));
  put('origin', real ? 'Relato real da ESP32' : 'Origem ainda não confirmada');
  put('uptime', v.uptime_seconds ? duration(Number(v.uptime_seconds.value)) : 'Aguardando dados');
  put('reset', value('reset_reason'));
  put('running', value('running_partition'));
  put('boot', value('boot_partition'));
  put('ota', value('ota_state'));
  put('boot-id', value('boot_id'));
  put('sequence', value('sequence'));
  var journal = value('ota_journal');
  put('journal', journal);
  var target = journal.match(/^TARGET_VERSION:\s*(.+)$/m);
  var bytes = journal.match(/^BYTES_WRITTEN:\s*(.+)$/m);
  var hash = journal.match(/^SHA_CALCULATED_FLASH:\s*(.+)$/m);
  put('last-ota', target ? 'Versão ' + target[1] : 'Aguardando registro');
  put('bytes', bytes ? bytes[1] + ' bytes' : 'Aguardando registro');
  put('sha', hash ? hash[1] : 'Aguardando registro');
  renderPresence();
};
function put(key, text) {
  var node = self.root.querySelector('[data-field="' + key + '"]');
  if (node) node.textContent = text;
}
function duration(seconds) {
  var s = Math.max(0, Math.floor(seconds));
  return Math.floor(s / 3600) + 'h ' + Math.floor((s % 3600) / 60) + 'min ' + (s % 60) + 's';
}
function renderPresence() {
  if (!self.root) return;
  var badge = self.root.querySelector('[data-field="presence"]');
  var age = self.lastSeen ? Math.max(0, Math.floor((Date.now() - self.lastSeen) / 1000)) : null;
  var recent = age !== null && age < 180;
  put('presence', age === null ? 'AGUARDANDO' : recent ? 'CONTATO RECENTE' : 'SEM CONTATO RECENTE');
  if (badge) badge.className = 'status ' + (recent ? 'recent' : 'stale');
  put('age', age === null ? 'Nenhum relato real recebido' : 'Último contato há ' + duration(age));
  put('timestamp', self.lastSeen ? new Date(self.lastSeen).toLocaleString('pt-BR') : '—');
}
self.onDestroy = function () { clearInterval(self.timer); self.root = null; };
self.typeParameters = function () { return { maxDatasources: 1, singleEntity: true }; };

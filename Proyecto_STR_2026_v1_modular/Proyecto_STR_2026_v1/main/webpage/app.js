const $ = (id) => document.getElementById(id);

function clampPercent(value) {
  const n = Number(value);
  if (Number.isNaN(n)) return 0;
  return Math.max(0, Math.min(100, Math.round(n)));
}

async function postJson(url, data) {
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  });
  if (!res.ok) throw new Error(await res.text());
  return res.json().catch(() => ({ ok: true }));
}

function updateRangeOutputs() {
  $('fanPercentOut').textContent = `${$('fanPercent').value}%`;
  $('rgbBrightnessOut').textContent = `${$('rgbBrightness').value}%`;
  $('curtainPercentOut').textContent = `${$('curtainPercent').value}%`;
}

function buildScheduleUi() {
  const root = $('scheduleList');
  root.innerHTML = '';
  for (let i = 0; i < 8; i++) {
    const div = document.createElement('div');
    div.className = 'schedule-item';
    div.innerHTML = `
      <h3>Registro ${i + 1}</h3>
      <label class="checkline"><input id="schEnabled${i}" type="checkbox"> Activo</label>
      <div class="schedule-row">
        <div><label>Hora</label><input id="schHour${i}" type="number" min="0" max="23" value="7"></div>
        <div><label>Min</label><input id="schMinute${i}" type="number" min="0" max="59" value="0"></div>
      </div>
      <label>Apertura (%)</label>
      <input id="schPercent${i}" type="number" min="0" max="100" value="0">
    `;
    root.appendChild(div);
  }
}

async function refreshStatus() {
  try {
    const data = await fetch('/api/status').then(r => r.json());
    $('connectionState').textContent = 'ESP conectado';
    $('temperature').textContent = `${Number(data.temperature).toFixed(1)} °C`;
    $('fanStatus').textContent = `${data.fan_percent}% (${data.fan_mode})`;
    $('curtainStatus').textContent = `${data.curtain_percent}% (${data.curtain_mode})`;
    $('alarmStatus').textContent = data.alarm ? 'Activa' : 'Normal';
    $('adcStatus').textContent = `ADC: ${data.adc_raw} · Voltaje: ${Number(data.adc_voltage).toFixed(3)} V`;

    $('fanMode').value = data.fan_mode;
    $('tempDesired').value = data.temp_desired;
    $('tempMax').value = data.temp_max;
    $('fanPercent').value = data.fan_percent;

    $('curtainMode').value = data.curtain_mode;
    $('curtainPercent').value = data.curtain_percent;

    if (data.rgb) {
      $('rgbR').value = data.rgb.r;
      $('rgbG').value = data.rgb.g;
      $('rgbB').value = data.rgb.b;
      $('rgbBrightness').value = data.rgb.brightness;
    }
    updateRangeOutputs();
  } catch (e) {
    $('connectionState').textContent = 'Sin respuesta del ESP';
  }
}

async function loadSchedule() {
  try {
    const data = await fetch('/api/schedule').then(r => r.json());
    (data.items || []).forEach(item => {
      const i = item.index;
      if (i >= 0 && i < 8) {
        $(`schEnabled${i}`).checked = !!item.enabled;
        $(`schHour${i}`).value = item.hour > 23 ? 7 : item.hour;
        $(`schMinute${i}`).value = item.minute > 59 ? 0 : item.minute;
        $(`schPercent${i}`).value = item.percent;
      }
    });
  } catch (e) {
    console.warn('No se pudo cargar agenda', e);
  }
}

async function saveFan() {
  await postJson('/api/fan', {
    mode: $('fanMode').value,
    desired: Number($('tempDesired').value),
    max: Number($('tempMax').value),
    percent: clampPercent($('fanPercent').value)
  });
  refreshStatus();
}

async function saveRgb() {
  await postJson('/api/rgb', {
    r: clampPercent($('rgbR').value),
    g: clampPercent($('rgbG').value),
    b: clampPercent($('rgbB').value),
    brightness: clampPercent($('rgbBrightness').value)
  });
  refreshStatus();
}

async function moveCurtain() {
  await postJson('/api/curtain', {
    mode: $('curtainMode').value,
    percent: clampPercent($('curtainPercent').value)
  });
  refreshStatus();
}

async function saveCurtainMode() {
  await postJson('/api/curtain', { mode: $('curtainMode').value });
  refreshStatus();
}

async function saveSchedule() {
  const items = [];
  for (let i = 0; i < 8; i++) {
    items.push({
      index: i,
      enabled: $(`schEnabled${i}`).checked,
      hour: Number($(`schHour${i}`).value),
      minute: Number($(`schMinute${i}`).value),
      percent: clampPercent($(`schPercent${i}`).value)
    });
  }
  await postJson('/api/schedule', { items });
  alert('Programación guardada. Recuerda poner la cortina en modo automático.');
}

async function saveStaWifi() {
  await postJson('/wifiConnect.json', {
    selectedSSID: $('staSsid').value,
    pwd: $('staPassword').value
  });
  $('wifiStatus').textContent = 'Estado WiFi: intentando conectar...';
}

async function saveApWifi() {
  await postJson('/apConfig.json', {
    ap_ssid: $('apSsid').value,
    ap_password: $('apPassword').value
  });
  alert('Soft-AP actualizado. Si estabas conectado a la red anterior, vuelve a conectarte con las nuevas credenciales.');
}

async function uploadOta() {
  const file = $('otaFile').files[0];
  if (!file) {
    alert('Selecciona primero un archivo .bin');
    return;
  }

  const form = new FormData();
  form.append('firmware', file, file.name);
  $('otaStatus').textContent = 'OTA: subiendo firmware...';

  const res = await fetch('/OTAupdate', { method: 'POST', body: form });
  if (!res.ok) {
    $('otaStatus').textContent = 'OTA: error al subir firmware';
    return;
  }

  $('otaStatus').textContent = 'OTA: firmware recibido, verificando estado...';
  setTimeout(checkOtaStatus, 1500);
}

async function checkOtaStatus() {
  try {
    const data = await fetch('/OTAstatus', { method: 'POST' }).then(r => r.json());
    $('otaStatus').textContent = `OTA: estado ${data.ota_update_status} · Compilado ${data.compile_date} ${data.compile_time}`;
  } catch (e) {
    $('otaStatus').textContent = 'OTA: no se pudo consultar estado';
  }
}

['fanPercent', 'rgbBrightness', 'curtainPercent'].forEach(id => {
  window.addEventListener('load', () => $(id).addEventListener('input', updateRangeOutputs));
});

window.addEventListener('load', () => {
  buildScheduleUi();
  updateRangeOutputs();
  refreshStatus();
  loadSchedule();
  setInterval(refreshStatus, 2000);
});

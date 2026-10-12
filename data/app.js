let BASE_URL = window.location.origin;
let intervalo = null;

const MAX_HIST = 30;
const histTemp = [];
const histHum  = [];
const histCo2  = [];

const cvs = document.getElementById('sparkline');
const ctx  = cvs.getContext('2d');

function resizeCanvas() {
  const wrap = cvs.parentElement;
  cvs.width  = wrap.clientWidth * devicePixelRatio;
  cvs.height = 120 * devicePixelRatio;
  ctx.scale(devicePixelRatio, devicePixelRatio);
  drawChart();
}

function drawChart() {
  const W = cvs.clientWidth;
  const H = 120;
  ctx.clearRect(0, 0, W, H);

  if (histTemp.length < 2) {
    ctx.fillStyle = 'rgba(106,137,106,0.3)';
    ctx.font = '12px DM Mono, monospace';
    ctx.textAlign = 'center';
    ctx.fillText('Esperando datos…', W / 2, H / 2);
    return;
  }

  const series = [
    { data: histTemp, color: '#c45c5c' },
    { data: histHum,  color: '#5fb3d4' },
    { data: histCo2,  color: '#d4a843' },
  ];

  series.forEach(({ data, color }) => {
    if (data.length < 2) return;
    const n = data.length;
    const min = Math.min(...data);
    const max = Math.max(...data);
    const range = (max - min) || 1;

    const px = (i) => (i / (MAX_HIST - 1)) * (W - 20) + 10;
    const py = (v)  => H - 10 - ((v - min) / range) * (H - 24);

    ctx.beginPath();
    ctx.moveTo(px(0), py(data[0]));
    data.forEach((v, i) => { if (i > 0) ctx.lineTo(px(i * (MAX_HIST / n)), py(v)); });
    ctx.lineTo(px((n - 1) * (MAX_HIST / n)), H);
    ctx.lineTo(px(0), H);
    ctx.closePath();
    ctx.fillStyle = color + '18';
    ctx.fill();

    ctx.beginPath();
    ctx.moveTo(px(0), py(data[0]));
    data.forEach((v, i) => { if (i > 0) ctx.lineTo(px(i * (MAX_HIST / n)), py(v)); });
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.8;
    ctx.lineJoin = 'round';
    ctx.stroke();
  });
}

window.addEventListener('resize', resizeCanvas);
resizeCanvas();

function showToast(msg, ms = 2500) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), ms);
}

function setText(id, val) {
  const el = document.getElementById(id);
  if (el) el.textContent = val;
}

function formatUptime(s) {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${h}h ${m}m ${sec}s`;
}

function tempClass(t) { if (t > 35) return 'alert'; if (t > 28) return 'warn'; return ''; }
function humClass(h)  { if (h < 30 || h > 85) return 'alert'; if (h < 40 || h > 70) return 'warn'; return ''; }
function co2Class(ppm){ if (ppm > 2000) return 'alert'; if (ppm > 1000) return 'warn'; return ''; }

function tempLabel(t) {
  if (t < 15) return '❄️ Frío';
  if (t < 22) return '😊 Fresco';
  if (t < 28) return '🌿 Óptimo';
  if (t < 35) return '🌡️ Cálido';
  return '🔥 Muy caliente';
}

function humLabel(h) {
  if (h < 30) return '🏜️ Muy seco';
  if (h < 40) return '💨 Seco';
  if (h < 70) return '🌿 Óptimo';
  if (h < 85) return '💦 Húmedo';
  return '🌊 Muy húmedo';
}

function co2Label(ppm) {
  if (ppm < 600)  return '🌿 Excelente';
  if (ppm < 1000) return '🙂 Bueno';
  if (ppm < 2000) return '😐 Regular';
  return '⚠️ Malo';
}

function audioLabel(valida) {
  return valida ? '🎙️ Mic OK' : '⚠️ Revisa cableado';
}

function iniciarLive() {
  document.getElementById('cam-stream').src = BASE_URL + '/resources/cam/live?t=' + Date.now();
}

function detenerLive() {
  document.getElementById('cam-stream').src = 'img/live.webp';
}

async function fetchSensores() {
  try {
    const r = await fetch(`${BASE_URL}/api/sensores`, { signal: AbortSignal.timeout(4000) });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const d = await r.json();

    const tEl = document.getElementById('temp-val');
    tEl.innerHTML = `${d.temperatura.toFixed(1)}<span class="card-unit">°C</span>`;
    tEl.className = 'card-value ' + tempClass(d.temperatura);
    setText('temp-sub', tempLabel(d.temperatura));

    const hEl = document.getElementById('hum-val');
    hEl.innerHTML = `${d.humedad.toFixed(1)}<span class="card-unit">%</span>`;
    hEl.className = 'card-value ' + humClass(d.humedad);
    setText('hum-sub', humLabel(d.humedad));

    const co2El    = document.getElementById('co2-val');
    const co2SubEl = document.getElementById('co2-sub');
    if (d.mq135.estado === 'listo') {
      co2El.innerHTML = `${d.mq135.co2.toFixed(0)}<span class="card-unit">ppm</span>`;
      co2El.className = 'card-value ' + co2Class(d.mq135.co2);
      co2SubEl.textContent = co2Label(d.mq135.co2);
    } else {
      co2El.innerHTML = `--<span class="card-unit">ppm</span>`;
      co2El.className = 'card-value';
      co2SubEl.textContent = d.mq135.estado === 'calentando' ? '🔥 Calentando sensor…' : '⚙️ Calibrando R0…';
    }

    const audioEl = document.getElementById('audio-val');
    audioEl.innerHTML = `${d.audio.nivelDb.toFixed(1)}<span class="card-unit">dB</span>`;
    audioEl.className = 'card-value';
    setText('audio-sub', audioLabel(d.audio.valida));

    const push = (arr, v) => { arr.push(v); if (arr.length > MAX_HIST) arr.shift(); };
    push(histTemp, d.temperatura);
    push(histHum,  d.humedad);
    if (d.mq135.estado === 'listo') push(histCo2, d.mq135.co2);
    drawChart();

    setOnline(true);
  } catch (e) {
    setOnline(false);
    console.warn('[sensores]', e.message);
  }
}

async function fetchEstado() {
  try {
    const r = await fetch(`${BASE_URL}/api/estado`, { signal: AbortSignal.timeout(4000) });
    if (!r.ok) return;
    const d = await r.json();
    setText('d-ip',    d.ip);
    setText('d-rssi',  d.rssi);
    setText('d-uptime', formatUptime(d.uptime));
    setText('d-ver',   d.version);
    setText('d-ssid',  d.ssid);
  } catch {}
}

async function mover(dir) {
  const pasos = document.getElementById('pasos-sel').value;
  setText('last-action', dir);

  const isVertical = dir === 'ARRIBA' || dir === 'ABAJO';
  if (isVertical) setText('tilt-state', dir);
  else            setText('pan-state',  dir);

  try {
    const r = await fetch(`${BASE_URL}/api/mover?dir=${dir}&pasos=${pasos}`, { signal: AbortSignal.timeout(5000) });
    const d = await r.json();
    if (!d.ok) showToast('⚠️ ' + d.error);
    else       showToast(`✓ ${dir} — ${pasos} pasos`);
  } catch (e) {
    showToast('❌ Sin conexión con el ESP32');
  }
}

function conectar() {
  let ip = document.getElementById('ip-input').value.trim();
  if (!ip) { showToast('Ingresa la IP del ESP32'); return; }
  if (!ip.startsWith('http')) ip = 'http://' + ip;
  BASE_URL = ip;

  if (intervalo) clearInterval(intervalo);

  fetchSensores();
  fetchEstado();

  intervalo = setInterval(() => {
    fetchSensores();
    fetchEstado();
  }, 5000);

  showToast('🔌 Conectando a ' + BASE_URL);
}

function setOnline(online) {
  const pill = document.getElementById('status-pill');
  setText('status-text', online ? 'En línea' : 'Desconectado');
  pill.className = 'status-pill ' + (online ? 'online' : '');
}

document.addEventListener('keydown', (e) => {
  const map = { ArrowUp: 'ARRIBA', ArrowDown: 'ABAJO', ArrowLeft: 'IZQ', ArrowRight: 'DER' };
  if (map[e.key]) { e.preventDefault(); mover(map[e.key]); }
});

// Autocompleta la IP con la de este mismo dispositivo (el que sirvió la página)
// y conecta de inmediato, sin esperar a que el usuario presione "Conectar".
// Si se abre como archivo local (file://) o desde otro host, location.hostname
// no sera la del ESP32: el campo queda editable para conectar manualmente.
document.getElementById('ip-input').value = location.hostname || '192.168.1.1';
conectar();

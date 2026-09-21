#pragma once
// Mobile control page served at http://192.168.4.1/ (or http://untitled.local/)

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Untitled</title>
<style>
:root{--bg:#111;--card:#1c1c1e;--fg:#eee;--dim:#888;--acc:#b04cff;--red:#ff453a;--grn:#30d158}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:16px -apple-system,system-ui,sans-serif;padding:16px;max-width:520px;margin:auto}
h1{font-size:22px;margin:4px 0 2px;letter-spacing:.08em}h2{font-size:13px;color:var(--dim);text-transform:uppercase;letter-spacing:.1em;margin:0 0 10px}
.sub{color:var(--dim);font-size:13px;margin-bottom:14px}
.card{background:var(--card);border-radius:14px;padding:14px;margin-bottom:14px}
.row{display:flex;justify-content:space-between;align-items:center;padding:6px 0;border-bottom:1px solid #2a2a2c}.row:last-child{border:0}
.row span:first-child{color:var(--dim)}
.phase{font-size:26px;font-weight:600;text-transform:capitalize}
.dots{display:flex;gap:16px;margin:10px 0 4px}.dot{display:flex;align-items:center;gap:6px;color:var(--dim);font-size:14px}
.dot i{width:12px;height:12px;border-radius:50%;background:#333;display:inline-block}
.dot.on.b i{background:var(--red);box-shadow:0 0 8px var(--red)}.dot.on.m i{background:var(--grn);box-shadow:0 0 8px var(--grn)}
.btns{display:flex;gap:8px;flex-wrap:wrap}
button{flex:1;min-width:30%;border:0;border-radius:10px;padding:14px 8px;font-size:16px;font-weight:600;background:#2c2c2e;color:var(--fg)}
button.pri{background:var(--acc)}button.stop{background:var(--red)}button:active{opacity:.6}
label{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:7px 0}
label small{color:var(--dim);display:block;font-size:12px}
input[type=number]{width:110px;padding:10px;border-radius:8px;border:1px solid #333;background:#000;color:var(--fg);font-size:16px;text-align:right}
input[type=file]{width:100%;margin:6px 0 10px;color:var(--dim)}
progress{width:100%;height:10px}
#msg{position:fixed;left:16px;right:16px;bottom:16px;background:#333;padding:12px;border-radius:10px;text-align:center;display:none}
</style></head><body>
<h1>UNTITLED</h1><div class="sub">Kitty Kraus &middot; controller <span id="fw"></span></div>

<div class="card">
 <div class="phase" id="phase">&hellip;</div>
 <div class="dots"><div class="dot b" id="dB"><i></i>Brake released</div><div class="dot m" id="dM"><i></i>Move</div></div>
 <div class="row"><span>Next kick in</span><span id="nk">&ndash;</span></div>
 <div class="row"><span>Next pause in</span><span id="np">&ndash;</span></div>
 <div class="row"><span>Last kick / rest</span><span id="lk">&ndash;</span></div>
 <div class="btns" style="margin-top:12px">
  <button class="pri" onclick="cmd('start')">Start</button>
  <button class="stop" onclick="cmd('stop')">Stop</button>
  <button onclick="cmd('kick')">Kick now</button>
 </div>
</div>

<div class="card"><h2>Settings</h2><form id="f"></form>
 <div class="btns" style="margin-top:10px">
  <button class="pri" onclick="save()">Save</button>
  <button onclick="defaults()">Defaults</button>
 </div>
</div>

<div class="card"><h2>Stats</h2>
 <div class="row"><span>Uptime</span><span id="up">&ndash;</span></div>
 <div class="row"><span>Kicks / pauses (since boot)</span><span id="kc">&ndash;</span></div>
 <div class="row"><span>Boots</span><span id="bt">&ndash;</span></div>
 <div class="row"><span>Wi-Fi turns off in</span><span id="ap">&ndash;</span></div>
 <div class="row"><span>Free memory</span><span id="hp">&ndash;</span></div>
</div>

<div class="card"><h2>Firmware update</h2>
 <div class="sub">Upload <b>Untitled.ino.bin</b> (not merged.bin). The piece stops during update and restarts running.</div>
 <input type="file" id="bin" accept=".bin">
 <progress id="pr" value="0" max="100"></progress>
 <div class="btns" style="margin-top:10px"><button onclick="ota()">Upload</button><button onclick="reboot()">Reboot</button></div>
</div>
<div id="msg"></div>

<script>
const F=[
 ['spinMin','Kick min','ms',1,'motor run pulse'],['spinMax','Kick max','ms',1,''],
 ['dropMin','Rest min','s',1000,'kick start to next kick'],['dropMax','Rest max','s',1000,''],
 ['hangEvery','Pause every','min',60000,'0 = never'],['hangTime','Pause length','s',1000,'rest after a pause starts'],
 ['brakeRelease','Brake release wait','ms',1,'before first kick'],['brakeEngage','Brake engage wait','ms',1,'after stop'],
 ['ledPct','LED brightness','%',1,''],['apMinutes','Wi-Fi on after boot','min',1,'0 = always on']];
const $=id=>document.getElementById(id);
function msg(t){const m=$('msg');m.textContent=t;m.style.display='block';clearTimeout(m.t);m.t=setTimeout(()=>m.style.display='none',2500)}
function fmt(ms){if(ms<0)return '–';let s=Math.round(ms/1000);if(s<60)return s+' s';let m=Math.floor(s/60);s%=60;if(m<60)return m+' min '+s+' s';return Math.floor(m/60)+' h '+m%60+' min'}
$('f').innerHTML=F.map(([k,l,u,sc,h])=>`<label><div>${l} <small>${u}${h?' · '+h:''}</small></div><input type="number" inputmode="decimal" step="any" min="0" id="${k}"></label>`).join('');
function fill(c){F.forEach(([k,,,sc])=>$(k).value=+(c[k]/sc).toFixed(3))}
async function load(){fill(await (await fetch('/api/config')).json())}
async function save(){const p=new URLSearchParams();F.forEach(([k,,,sc])=>p.append(k,Math.round(parseFloat($(k).value||0)*sc)));
 const r=await fetch('/api/config',{method:'POST',body:p});fill(await r.json());msg('Saved')}
async function defaults(){if(!confirm('Reset all settings to defaults?'))return;fill(await (await fetch('/api/defaults',{method:'POST'})).json());msg('Defaults restored')}
async function cmd(c){show(await (await fetch('/api/'+c,{method:'POST'})).json())}
async function reboot(){if(!confirm('Reboot controller?'))return;await fetch('/api/reboot',{method:'POST'});msg('Rebooting…');setTimeout(()=>location.reload(),6000)}
function show(s){
 $('fw').textContent='v'+s.fw;$('phase').textContent=s.phase;
 $('dB').classList.toggle('on',s.brake=='released');$('dM').classList.toggle('on',s.move);
 $('nk').textContent=fmt(s.nextKickIn);$('np').textContent=fmt(s.pauseIn);
 $('lk').textContent=s.kicks?s.lastSpin+' ms / '+fmt(s.lastDrop):'–';
 $('up').textContent=fmt(s.uptime*1000);$('kc').textContent=s.kicks+' / '+s.pauses;$('bt').textContent=s.boots;
 $('ap').textContent=s.apOffIn<0?'never':(s.apOffIn==0?'when phone disconnects':fmt(s.apOffIn));
 $('hp').textContent=Math.round(s.heap/1024)+' kB';
}
async function poll(){try{show(await (await fetch('/api/status')).json())}catch(e){$('phase').textContent='offline'}setTimeout(poll,1000)}
function ota(){const f=$('bin').files[0];if(!f)return msg('Choose a .bin file');if(f.name.includes('merged'))return msg('Use Untitled.ino.bin, not merged.bin');
 const x=new XMLHttpRequest(),d=new FormData();d.append('firmware',f,f.name);
 x.upload.onprogress=e=>$('pr').value=e.loaded/e.total*100;
 x.onload=()=>{if(x.status==200){msg('Updated – rebooting');setTimeout(()=>location.reload(),8000)}else msg('Update failed: '+x.responseText)};
 x.onerror=()=>msg('Upload error');x.open('POST','/update');x.send(d)}
load();poll();
</script></body></html>)HTML";

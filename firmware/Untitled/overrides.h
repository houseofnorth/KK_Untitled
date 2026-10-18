#pragma once
// Hidden page served at http://192.168.4.1/overrides (linked only from /tech and /setup)
// Per-LED levels, LED test, VersiDrive check, V2 options.

const char OVERRIDES_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Untitled · Overrides</title>
<style>
*{box-sizing:border-box;border-radius:0!important}
body{margin:0 auto;background:#111;color:#eee;font:16px -apple-system,system-ui,sans-serif;padding:16px;max-width:520px}
h1{font-size:17px;margin:4px 0 2px;letter-spacing:.14em}h2{font-size:13px;color:#888;text-transform:uppercase;letter-spacing:.1em;margin:0 0 10px}
.nav{display:flex;gap:18px;margin:2px 0 14px;font-size:12px;letter-spacing:.12em;text-transform:uppercase}
.nav a{color:#777;text-decoration:none;padding:4px 0}.nav a.on{color:#eee;border-bottom:1px solid #b04cff}
.sub{color:#888;font-size:13px;margin-bottom:14px}
.card{background:#1c1c1e;padding:14px;margin-bottom:14px}
.led{display:grid;grid-template-columns:14px 1fr 64px;gap:10px;align-items:center;padding:8px 0}
.led i{width:14px;height:14px;display:block}
.led input[type=range]{width:100%;accent-color:#b04cff}
input[type=number]{width:90px;padding:8px;border:1px solid #333;background:#000;color:#eee;font-size:16px;text-align:right}
.led input[type=number]{width:64px}
label{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:8px 0}
label small{color:#888;display:block;font-size:12px}
input[type=checkbox]{width:26px;height:26px;accent-color:#b04cff;flex:none}
.btns{display:flex;gap:8px;margin-top:10px}
button{flex:1;border:0;padding:14px 8px;font-size:16px;font-weight:600;background:#2c2c2e;color:#eee}
button.pri{background:#b04cff}button:active{opacity:.6}
#msg{position:fixed;left:16px;right:16px;bottom:16px;background:#333;padding:12px;text-align:center;display:none}
</style></head><body>
<h1>OVERRIDES</h1>
<nav class="nav"><a href="/">Main</a><a href="/tech">Advanced</a><a class="on" href="/overrides">Overrides</a><a href="/setup">Setup</a></nav>
<div class="sub">Untitled &middot; <span id="fw"></span> &middot; changes work at once, Save keeps them</div>

<div class="card"><h2>LED levels</h2>
 <div id="leds"></div>
 <label><div>LED test <small>all four solid at these levels &middot; ends when you leave the page</small></div><input type="checkbox" id="test"></label>
</div>
<div class="card"><h2>VersiDrive</h2><div id="vfd"></div></div>
<div class="card"><h2>V2 options</h2><div id="opts"></div></div>

<div class="btns"><button class="pri" onclick="save()">Save</button><button onclick="undo()">Undo</button></div>
<div id="msg"></div>

<script>
const L=[['ledRed','Red · RUN / STOP','#e5342b'],['ledGreen','Green · BRAKE RELEASE','#2fb64a'],
 ['ledAmber','Amber · VERSI DRIVE','#f5a623'],['ledBlue','Blue · WIFI ENABLE','#3b82f6']];
const V=[
 ['useVfdOk','Stop on VFD fault','on',1,'untick to run without the drive · tick again before going live'],
 ['okResume','Resume after drive OK for','s',1000,''],
 ['motorMa','Motor rated current (P-08)','A',1000,'same value as P-08 on Setup · for the A readout, needs P-25 = 9']];
const O=[
 ['autostart','Autostart after power-up','on',1,'mains on starts the piece · untick for installation / maintenance'],
 ['softKicks','Soft start kicks','kicks',1,'first pushes ramp 50→100 %, 0 = off']];
const F=V.concat(O);
const $=id=>document.getElementById(id);
function msg(t){const m=$('msg');m.textContent=t;m.style.display='block';clearTimeout(m.t);m.t=setTimeout(()=>m.style.display='none',2000)}
const fld=([k,l,u,sc,h])=>u=='on'
 ?`<label><div>${l}${h?' <small>'+h+'</small>':''}</div><input type="checkbox" id="${k}"></label>`
 :`<label><div>${l} <small>${u}${h?' · '+h:''}</small></div><input type="number" inputmode="decimal" step="any" min="0" id="${k}"></label>`;
$('vfd').innerHTML=V.map(fld).join('');$('opts').innerHTML=O.map(fld).join('');
$('leds').innerHTML=L.map(([k,n,c])=>`<div class="led"><i style="background:${c}"></i><div><small style="color:#888">${n}</small><br><input type="range" min="0" max="100" id="${k}_r"></div><input type="number" inputmode="numeric" min="0" max="100" id="${k}"></div>`).join('');
let rev=-1,busy=0;
function fill(c){rev=c.rev;L.forEach(([k])=>{$(k).value=c[k];$(k+'_r').value=c[k]});
 F.forEach(([k,,u,sc])=>{if(u=='on')$(k).checked=c[k]==1;else $(k).value=+(c[k]/sc).toFixed(3)})}
function params(){const p=new URLSearchParams();L.forEach(([k])=>p.append(k,Math.max(0,Math.min(100,parseInt($(k).value||0)))));
 F.forEach(([k,,u,sc])=>p.append(k,u=='on'?($(k).checked?1:0):Math.round(parseFloat($(k).value||0)*sc)));return p}
let tm;function live(){clearTimeout(tm);tm=setTimeout(async()=>{busy=1;fill(await (await fetch('/api/live',{method:'POST',body:params()})).json());busy=0},120)}
L.forEach(([k])=>{$(k+'_r').addEventListener('input',()=>{$(k).value=$(k+'_r').value;live()});$(k).addEventListener('input',()=>{$(k+'_r').value=$(k).value;live()})});
F.forEach(([k])=>$(k).addEventListener('change',live));
async function save(){fill(await (await fetch('/api/config',{method:'POST',body:params()})).json());msg('Saved')}
async function undo(){fill(await (await fetch('/api/revert',{method:'POST'})).json());msg('Back to saved')}
async function test(){const on=$('test').checked;await fetch('/api/ledtest',{method:'POST',body:new URLSearchParams({on:on?1:0})})}
$('test').addEventListener('change',test);setInterval(()=>{if($('test').checked)test()},3000);
async function poll(){try{const s=await (await fetch('/api/status')).json();$('fw').textContent='v'+s.fw;
 if(!busy&&s.rev!=rev)fill(await (await fetch('/api/config')).json())}catch(e){$('fw').textContent='offline'}setTimeout(poll,1500)}
fetch('/api/config').then(r=>r.json()).then(fill);poll();
</script></body></html>)HTML";

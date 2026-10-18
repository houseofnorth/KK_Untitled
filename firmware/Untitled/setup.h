#pragma once
// Setup record served at http://192.168.4.1/setup (linked from /tech, /overrides)
// What was read off the motor and the VersiDrive on site. Stored as one JSON string in flash
// (/api/setup). P-08 is not stored here: it is the config value motorMa, shared with /overrides.
// The controller cannot change drive parameters - this page is a record only.

const char SETUP_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Untitled · Setup</title>
<style>
*{box-sizing:border-box;border-radius:0!important}
body{margin:0 auto;background:#111;color:#eee;font:16px -apple-system,system-ui,sans-serif;padding:16px;max-width:520px}
h1{font-size:17px;margin:4px 0 2px;letter-spacing:.14em}h2{font-size:13px;color:#888;text-transform:uppercase;letter-spacing:.1em;margin:0 0 6px}
.nav{display:flex;gap:18px;margin:2px 0 14px;font-size:12px;letter-spacing:.12em;text-transform:uppercase}
.nav a{color:#777;text-decoration:none;padding:4px 0}.nav a.on{color:#eee;border-bottom:1px solid #b04cff}
.sub{color:#888;font-size:13px;margin-bottom:14px}
.card{background:#1c1c1e;padding:14px;margin-bottom:14px}
.card.key{border-left:3px solid #b04cff}.card.key h2{color:#d9a6ff}
.card.rec{background:#161618}.card.rec label div{color:#aaa}
label small.why{color:#b9a3cc}
label{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:7px 0;border-bottom:1px solid #2a2a2c}
label:last-child{border:0}
label small{color:#888;display:block;font-size:12px}
input{width:120px;padding:8px;border:1px solid #333;background:#000;color:#eee;font-size:16px;text-align:right;flex:none}
textarea{width:100%;min-height:90px;padding:8px;border:1px solid #333;background:#000;color:#eee;font:15px -apple-system,system-ui,sans-serif}
.btns{display:flex;gap:8px;margin-top:10px}
button{flex:1;border:0;padding:14px 8px;font-size:16px;font-weight:600;background:#2c2c2e;color:#eee}
button.pri{background:#b04cff}button:active{opacity:.6}
#msg{position:fixed;left:16px;right:16px;bottom:16px;background:#333;padding:12px;text-align:center;display:none}
</style></head><body>
<h1>SETUP</h1>
<nav class="nav"><a href="/">Main</a><a href="/tech">Advanced</a><a href="/overrides">Overrides</a><a class="on" href="/setup">Setup</a></nav>
<div class="sub">What the motor and VersiDrive are set to. A record only &ndash; the controller can't change the drive. <b>Matters</b> = affects the readout, the drive-OK light or how the push feels; the rest is for the record.</div>
<div id="g"></div>
<div class="card"><h2>Notes</h2><textarea id="notes" placeholder="anything else, date, who"></textarea></div>
<div class="btns"><button class="pri" onclick="save()">Save</button><button onclick="load()">Undo</button></div>
<div id="msg"></div>

<script>
// group title, key group?, fields: key, label, unit / expected value, why it matters (key groups only)
const G=[
 ['Matters',1,[
  ['p08','P-08 Motor current','A','sets the amps readout on /tech · same as Motor rated current on Overrides'],
  ['p25','P-25 Analog output function','must be 9','otherwise the amps readout is wrong'],
  ['p18','P-18 Relay function','must be 1','drive-OK input and the amber light depend on it'],
  ['p03','P-03 Acceleration ramp','s','pushes are 0.2–0.9 s: a longer ramp means they never reach speed'],
  ['p01','P-01 Max speed','Hz','the top of every push'],
  ['brRel','Brake release time','s','compare with Brake release wait on /tech (now <b id="wRel">–</b>)'],
  ['brEng','Brake engage time','s','compare with Brake engage wait on /tech (now <b id="wEng">–</b>)']]],
 ['Record only',0,[
  ['nV','Nameplate voltage','V'],['nA','Nameplate current','A'],['nKw','Nameplate power','kW'],
  ['nRpm','Nameplate speed','rpm'],['nHz','Nameplate frequency','Hz'],['nCos','Power factor','cos φ'],['nConn','Connection','star / delta'],
  ['p02','P-02 Min speed','Hz'],['p04','P-04 Deceleration ramp','s'],['p05','P-05 Stop mode','ramp / coast'],
  ['p07','P-07 Motor voltage','V'],['p09','P-09 Motor frequency','Hz'],['p10','P-10 Motor speed','rpm'],
  ['p12','P-12 Control mode',''],['p14','P-14 Access code','101 to change parameters'],['p15','P-15 Digital input function',''],
  ['peakA','Peak push current','A · baseline for later'],['drvFw','Drive model / firmware','']]]];
const $=id=>document.getElementById(id);
function msg(t){const m=$('msg');m.textContent=t;m.style.display='block';clearTimeout(m.t);m.t=setTimeout(()=>m.style.display='none',2000)}
$('g').innerHTML=G.map(([h,key,fs])=>`<div class="card ${key?'key':'rec'}"><h2>${h}</h2>`+fs.map(([k,l,u,w])=>
 `<label><div>${l}${u?' <small>'+u+'</small>':''}${w?'<small class="why">'+w+'</small>':''}</div><input id="${k}" ${k=='p08'?'type="number" inputmode="decimal" step="any" min="0"':'type="text"'}></label>`).join('')+'</div>').join('');
const K=G.flatMap(([,,fs])=>fs.map(f=>f[0])).filter(k=>k!='p08');
let ma=0;
async function load(){
 let d={};try{d=await (await fetch('/api/setup')).json()}catch(e){}
 K.forEach(k=>$(k).value=d[k]||'');$('notes').value=d.notes||'';
 const c=await (await fetch('/api/config')).json();ma=c.motorMa;$('wRel').textContent=c.brakeRelease/1000+' s';$('wEng').textContent=c.brakeEngage/1000+' s';$('p08').value=ma?+(ma/1000).toFixed(3):''}
async function save(){
 const d={};K.forEach(k=>{const v=$(k).value.trim();if(v)d[k]=v});const n=$('notes').value.trim();if(n)d.notes=n;
 const s=JSON.stringify(d);if(s.length>2500)return msg('Too long – shorten the notes');
 const r=await fetch('/api/setup',{method:'POST',body:new URLSearchParams({d:s})});
 if(!r.ok)return msg('Not saved');
 const nm=Math.round(parseFloat($('p08').value||0)*1000);
 if(nm!=ma){await fetch('/api/config',{method:'POST',body:new URLSearchParams({motorMa:nm})});ma=nm}
 msg('Saved')}
load();
</script></body></html>)HTML";

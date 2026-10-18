#pragma once
// VersiDrive page served at http://192.168.4.1/versidrive (/setup redirects here)
// Documents how the VersiDrive and motor are set on site. Stored as one JSON string in flash
// (/api/setup). P-08 is not stored in that record: it is the config value motorMa (shared with
// /overrides) and turns the T8 voltage into amps. The controller cannot change drive parameters.

const char VERSIDRIVE_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Untitled · VersiDrive</title>
<style>
*{box-sizing:border-box;border-radius:0!important}
body{margin:0 auto;background:#111;color:#eee;font:16px -apple-system,system-ui,sans-serif;padding:16px;max-width:520px}
h1{font-size:17px;margin:4px 0 2px;letter-spacing:.14em}h2{font-size:13px;color:#888;text-transform:uppercase;letter-spacing:.1em;margin:0 0 4px}
.nav{display:flex;gap:18px;margin:2px 0 14px;font-size:12px;letter-spacing:.12em;text-transform:uppercase}
.nav a{color:#777;text-decoration:none;padding:4px 0}.nav a.on{color:#eee;border-bottom:1px solid #b04cff}
.sub{color:#999;font-size:14px;margin-bottom:14px;line-height:1.45}
.card{background:#1c1c1e;padding:14px;margin-bottom:14px}
.card .lead{color:#999;font-size:13px;margin:0 0 8px}
.card.key{border-left:3px solid #b04cff}.card.key h2{color:#d9a6ff}
.card.rec{background:#161618}.card.rec label div{color:#aaa}
.row{display:flex;justify-content:space-between;padding:6px 0;border-bottom:1px solid #2a2a2c}.row:last-child{border:0}
.row span:first-child{color:#999}
label{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:7px 0;border-bottom:1px solid #2a2a2c}
label:last-child{border:0}
label small{color:#888;display:block;font-size:12px}
label small.why{color:#b9a3cc}
input{width:120px;padding:8px;border:1px solid #333;background:#000;color:#eee;font-size:16px;text-align:right;flex:none}
textarea{width:100%;min-height:90px;padding:8px;border:1px solid #333;background:#000;color:#eee;font:15px -apple-system,system-ui,sans-serif}
.btns{display:flex;gap:8px;margin-top:10px}
button{flex:1;border:0;padding:14px 8px;font-size:16px;font-weight:600;background:#2c2c2e;color:#eee}
button.pri{background:#b04cff}button:active{opacity:.6}
#msg{position:fixed;left:16px;right:16px;bottom:16px;background:#333;padding:12px;text-align:center;display:none}
</style></head><body>
<h1>VERSIDRIVE</h1>
<nav class="nav"><a href="/">Main</a><a href="/tech">Advanced</a><a href="/overrides">Overrides</a><a class="on" href="/versidrive">VersiDrive</a></nav>
<div class="sub">Write down how the VersiDrive and the motor are set: read the values off the drive's display and the motor nameplate. This page only documents the drive &ndash; the controller can't change it. But the values under <b>Used by the readouts</b> decide whether the readings at the top are right.</div>

<div class="card"><h2>Live readouts</h2>
 <div class="row"><span>Drive OK</span><span id="lOk">&ndash;</span></div>
 <div class="row"><span>Motor current now</span><span id="lA">&ndash;</span></div>
 <div class="row"><span>Push peak last / average</span><span id="lPk">&ndash;</span></div>
 <div class="row"><span>Drive faults since boot</span><span id="lF">&ndash;</span></div>
</div>
<div id="g"></div>
<div class="card"><h2>Notes</h2><textarea id="notes" placeholder="anything else, date, who"></textarea></div>
<div class="btns"><button class="pri" onclick="save()">Save</button><button onclick="load()">Undo</button></div>
<div id="msg"></div>

<script>
// title, style (key / plain / rec), lead text, fields: key, label, unit or expected value, effect
const G=[
 ['Used by the readouts','key','Changing these changes what the live readouts show.',[
  ['p08','P-08 Motor current','A','turns the drive\'s output voltage into amps · same value as Motor rated current on Overrides'],
  ['p25','P-25 Analog output function','must be 9','set to anything else, the current reading is wrong'],
  ['p18','P-18 Relay function','must be 1','set to anything else, Drive OK and the amber light are wrong']]],
 ['Drive settings that affect the push','plain','Change these on the VersiDrive keypad, not here. Note what they are before and after.',[
  ['p01','P-01 Max speed','Hz','the top speed any push can reach'],
  ['p03','P-03 Acceleration ramp','s','pushes are 0.2–0.9 s: shorter ramp = harder push, longer = softer'],
  ['brRel','Brake release time','s','measured · compare with Brake release wait on Advanced (now <b id="wRel">–</b>)'],
  ['brEng','Brake engage time','s','measured · compare with Brake engage wait on Advanced (now <b id="wEng">–</b>)']]],
 ['For the record','rec','',[
  ['nV','Nameplate voltage','V'],['nA','Nameplate current','A'],['nKw','Nameplate power','kW'],
  ['nRpm','Nameplate speed','rpm'],['nHz','Nameplate frequency','Hz'],['nCos','Power factor','cos φ'],['nConn','Connection','star / delta'],
  ['p02','P-02 Min speed','Hz'],['p04','P-04 Deceleration ramp','s'],['p05','P-05 Stop mode','ramp / coast'],
  ['p07','P-07 Motor voltage','V'],['p09','P-09 Motor frequency','Hz'],['p10','P-10 Motor speed','rpm'],
  ['p12','P-12 Control mode',''],['p14','P-14 Access code','101 to change parameters'],['p15','P-15 Digital input function',''],
  ['peakA','Push peak on day one','A · baseline for later'],['drvFw','Drive model / firmware','']]]];
const $=id=>document.getElementById(id);
function msg(t){const m=$('msg');m.textContent=t;m.style.display='block';clearTimeout(m.t);m.t=setTimeout(()=>m.style.display='none',2000)}
$('g').innerHTML=G.map(([h,cls,lead,fs])=>`<div class="card ${cls}"><h2>${h}</h2>${lead?'<p class="lead">'+lead+'</p>':''}`+fs.map(([k,l,u,w])=>
 `<label><div>${l}${u?' <small>'+u+'</small>':''}${w?'<small class="why">'+w+'</small>':''}</div><input id="${k}" ${k=='p08'?'type="number" inputmode="decimal" step="any" min="0"':'type="text"'}></label>`).join('')+'</div>').join('');
const K=G.flatMap(([,,,fs])=>fs.map(f=>f[0])).filter(k=>k!='p08');
let ma=0;
const amp=v=>v.toFixed(2)+' V'+(ma?' · '+(v*0.2*ma/1000).toFixed(2)+' A':' (enter P-08 for amps)');
async function load(){
 let d={};try{d=await (await fetch('/api/setup')).json()}catch(e){}
 K.forEach(k=>$(k).value=d[k]||'');$('notes').value=d.notes||'';
 const c=await (await fetch('/api/config')).json();ma=c.motorMa;$('p08').value=ma?+(ma/1000).toFixed(3):'';
 $('wRel').textContent=c.brakeRelease/1000+' s';$('wEng').textContent=c.brakeEngage/1000+' s'}
async function save(){
 const d={};K.forEach(k=>{const v=$(k).value.trim();if(v)d[k]=v});const n=$('notes').value.trim();if(n)d.notes=n;
 const s=JSON.stringify(d);if(s.length>2500)return msg('Too long – shorten the notes');
 const r=await fetch('/api/setup',{method:'POST',body:new URLSearchParams({d:s})});
 if(!r.ok)return msg('Not saved');
 const nm=Math.round(parseFloat($('p08').value||0)*1000);
 if(nm!=ma){await fetch('/api/config',{method:'POST',body:new URLSearchParams({motorMa:nm})});ma=nm}
 msg('Saved')}
async function poll(){try{const s=await (await fetch('/api/status')).json();
 $('lOk').textContent=s.vfdOk?'OK':'not OK';$('lA').textContent=amp(s.ao);
 $('lPk').textContent=s.aoPeak<0?'–':amp(s.aoPeak)+' / '+amp(s.aoAvg);$('lF').textContent=s.faults}catch(e){}setTimeout(poll,1000)}
load();poll();
</script></body></html>)HTML";

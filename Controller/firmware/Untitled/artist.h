#pragma once
// Artist page served at http://192.168.4.1/  (advanced settings: /tech)
// Sliders apply live (/api/live). Save stores to flash (/api/config), Undo returns to saved (/api/revert).

const char ARTIST_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>Kitty Kraus — Untitled</title>
<style>
:root{--bg:#0d0d0f;--card:#18181b;--line:#2a2a2e;--fg:#f2f2f2;--dim:#8a8a90;--acc:#b04cff;--acc2:#d9a6ff;--red:#ff5a4f}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;border-radius:0!important}
body{margin:0;background:var(--bg);color:var(--fg);font:17px/1.4 -apple-system,system-ui,sans-serif;padding:20px 16px 120px;max-width:540px;margin:auto}
header{display:flex;justify-content:space-between;align-items:center;margin-bottom:18px}
h1{font-size:15px;letter-spacing:.16em;margin:0;font-weight:600;line-height:1.3}
.lang{display:flex;border:1px solid var(--line);border-radius:20px;overflow:hidden}
.lang button{background:none;border:0;color:var(--dim);padding:6px 12px;font-size:14px;font-weight:600}
.lang button.on{background:var(--line);color:var(--fg)}
.card{background:var(--card);border-radius:18px;padding:18px;margin-bottom:14px}
.now{font-size:34px;font-weight:600;line-height:1.1}
.next{color:var(--dim);margin:6px 0 14px;min-height:24px}
.bar{height:6px;background:var(--line);border-radius:3px;overflow:hidden}
.bar i{display:block;height:100%;width:0;background:var(--acc);transition:width .9s linear}
.bar.push i{background:var(--acc2);width:100%!important;transition:none;animation:pulse .5s ease-in-out infinite alternate}
@keyframes pulse{from{opacity:.5}to{opacity:1}}
.btns{display:flex;gap:10px;margin-top:16px}
.btns button{flex:1;border:0;border-radius:14px;padding:16px;font-size:17px;font-weight:600;color:var(--fg);background:#2c2c30}
.btns button.go{background:var(--acc)}.btns button.stop{background:var(--red)}
button:disabled{opacity:.35}button:active{opacity:.6}
.set h2{font-size:19px;margin:0;font-weight:600}
.set p{color:var(--dim);font-size:14px;margin:4px 0 0}
.val{display:flex;gap:32px;font-size:26px;font-weight:600;margin:12px 0 2px;font-variant-numeric:tabular-nums}
.mm small{display:block;font-size:11px;letter-spacing:.14em;color:var(--dim);font-weight:600;margin-bottom:2px}
.hd{display:flex;justify-content:space-between;align-items:center}
.sw{position:relative;width:52px;height:32px;flex:none}
.sw input{position:absolute;opacity:0;width:100%;height:100%;margin:0}
.sw i{position:absolute;inset:0;border-radius:16px;background:var(--line);transition:background .2s;pointer-events:none}
.sw i:after{content:'';position:absolute;left:3px;top:3px;width:26px;height:26px;border-radius:0;background:#fff;transition:transform .2s}
.sw input:checked+i{background:var(--acc)}.sw input:checked+i:after{transform:translateX(20px)}
.set.off .val{color:var(--dim)}.set.off .sl,.set.off .ends{opacity:.35}
.sl{position:relative;height:48px}
.sl .tr{position:absolute;left:15px;right:15px;top:21px;height:6px;border-radius:3px;background:var(--line)}
.sl .tr i{position:absolute;top:0;bottom:0;border-radius:3px;background:var(--acc)}
.sl input{position:absolute;left:0;top:0;width:100%;height:48px;margin:0;background:none;-webkit-appearance:none;appearance:none;pointer-events:none}
.sl input::-webkit-slider-runnable-track{height:48px;background:none}
.sl input::-moz-range-track{background:none}
.sl input::-webkit-slider-thumb{-webkit-appearance:none;pointer-events:auto;width:30px;height:30px;margin-top:9px;border-radius:0;background:#fff;box-shadow:0 1px 6px #000a;border:0}
.sl input::-moz-range-thumb{pointer-events:auto;width:30px;height:30px;border-radius:0;background:#fff;border:0;box-shadow:0 1px 6px #000a}
.ends{display:flex;justify-content:space-between;color:var(--dim);font-size:12px;margin-top:-4px}
#save{position:fixed;left:0;right:0;bottom:0;background:#222226ee;backdrop-filter:blur(10px);-webkit-backdrop-filter:blur(10px);padding:14px 16px calc(14px + env(safe-area-inset-bottom));transform:translateY(110%);transition:transform .25s}
#save.show{transform:none}
#save div{max-width:540px;margin:auto}
#save p{margin:0 0 10px;font-size:14px;color:var(--acc2);text-align:center}
#save .btns{margin:0}
foot{display:block;text-align:center;margin-top:26px}
.wifi{display:flex;justify-content:space-between;align-items:center;gap:12px;color:var(--dim);font-size:15px}
.wifi button{border:1px solid var(--line);background:none;color:var(--fg);padding:10px 14px;font-size:15px;font-weight:600;flex:none}
foot a{color:var(--dim);font-size:14px}
#msg{position:fixed;left:16px;right:16px;top:16px;background:#333;padding:12px;border-radius:12px;text-align:center;opacity:0;transition:opacity .3s;pointer-events:none}
#msg.show{opacity:1}
</style></head><body>
<header><h1>KITTY KRAUS — UNTITLED</h1><div class="lang"><button id="lEN" onclick="setLang('en')">EN</button><button id="lDE" onclick="setLang('de')">DE</button></div></header>

<div class="card">
 <div class="now" id="now">&hellip;</div>
 <div class="next" id="next"></div>
 <div class="bar" id="bar"><i id="barI"></i></div>
 <div class="btns">
  <button id="run" class="go" onclick="toggleRun()"></button>
  <button id="kick" onclick="post('/api/kick')" data-t="pushNow"></button>
 </div>
</div>

<div id="sets"></div>

<div class="card wifi"><span id="wf"></span><button id="wfb" onclick="wifiStay()" data-t="wifiStay"></button></div>
<foot><a href="/tech" data-t="tech"></a></foot>

<div id="save"><div><p data-t="unsaved"></p><div class="btns">
 <button onclick="undo()" data-t="undo"></button><button class="go" onclick="save()" data-t="save"></button>
</div></div></div>
<div id="msg"></div>

<script>
const T={
en:{pushing:'Pushing',warn:'About to push',resting:'Resting',pause:'Long pause',starting:'Starting',stopping:'Stopping',stopped:'Stopped',
 waitVfd:'Waiting · drive fault',offline:'Not connected',
 nextIn:'Next push in ',start:'Start',stop:'Stop',pushNow:'Push now',
 push:'Push strength',pushD:'Each push is random between min and max. A longer push is a harder push and swings higher.',
 pushL:'Push length',pushLD:'How long the motor pushes each time, random between min and max.',
 str:'Push strength',strD:'How hard the motor drives each push, random between min and max.',soft:'soft',hard:'hard',
 rest:'Rest',restD:'Time from one push to the next. Each time a random value between min and max.',
 every:'Long pause every',everyD:'Now and then the piece rests for longer. Switch off for no long pauses.',never:'Never',
 plen:'Long pause length',plenD:'How long a long pause lasts, random between min and max.',
 unsaved:'Changes are live, but not saved yet',undo:'Undo',save:'Save',saved:'Saved · kept after power-off',undone:'Back to saved settings',
 wifiOff:'Wi-Fi turns off in ',wifiConn:'Wi-Fi turns off when you disconnect',wifiAlways:'Wi-Fi always on',wifiStay:'Reset to 15 min',wifiReset:'Wi-Fi timer reset',
 tech:'Advanced settings'},
de:{pushing:'Schub',warn:'Gleich geht’s los',resting:'Ruhe',pause:'Lange Pause',starting:'Startet',stopping:'Stoppt',stopped:'Gestoppt',
 waitVfd:'Wartet · Störung Antrieb',offline:'Keine Verbindung',
 nextIn:'Nächster Schub in ',start:'Start',stop:'Stopp',pushNow:'Jetzt schieben',
 push:'Schubstärke',pushD:'Jeder Schub zufällig zwischen Min und Max. Ein längerer Schub ist ein stärkerer Schub und schwingt höher.',
 pushL:'Schubdauer',pushLD:'Wie lange der Motor jeweils schiebt, zufällig zwischen Min und Max.',
 str:'Schubstärke',strD:'Wie stark der Motor jeden Schub antreibt, zufällig zwischen Min und Max.',soft:'sanft',hard:'stark',
 rest:'Ruhe',restD:'Zeit von einem Schub zum nächsten. Jedes Mal zufällig zwischen Min und Max.',
 every:'Lange Pause alle',everyD:'Ab und zu ruht die Arbeit länger. Ausschalten für keine langen Pausen.',never:'Nie',
 plen:'Länge der langen Pause',plenD:'Wie lange eine lange Pause dauert, zufällig zwischen Min und Max.',
 unsaved:'Änderungen sind aktiv, aber noch nicht gespeichert',undo:'Rückgängig',save:'Speichern',saved:'Gespeichert · bleibt nach dem Ausschalten',undone:'Zurück zu den gespeicherten Werten',
 wifiOff:'WLAN aus in ',wifiConn:'WLAN geht aus, wenn du die Verbindung trennst',wifiAlways:'WLAN immer an',wifiStay:'Auf 15 Min. zurücksetzen',wifiReset:'WLAN-Timer zurückgesetzt',
 tech:'Erweiterte Einstellungen'}};
// id, config keys, low, high (ms, log scale), can be off, pct = linear 0..100 %, sh = soft/hard ends
const S=[
 {id:'push', k:['spinMin','spinMax'],lo:50,hi:5000,sh:1},
 {id:'str',  k:['strMin','strMax'],lo:0,hi:100,pct:1,sh:1},
 {id:'rest', k:['dropMin','dropMax'],lo:2000,hi:600000},
 {id:'every',k:['hangEvery','hangEveryMax'],lo:60000,hi:10800000,sw:1},
 {id:'plen', k:['hangTime','hangTimeMax'],lo:10000,hi:3600000}];
const $=id=>document.getElementById(id);
let L='en',cfg={},st={},dirty=false,busy=0,tSend=0,loaded=false,rev=-1,lastIn=0,pauseOn=true;
try{L=localStorage.getItem('lang')||(navigator.language||'').slice(0,2)}catch(e){}
if(!T[L])L='en';
const t=k=>T[L][k]||k;

function fmt(ms){
 if(ms<1000)return (ms/1000).toFixed(2)+' s';
 if(ms<10000)return (ms/1000).toFixed(1)+' s';
 let s=Math.round(ms/1000);if(s<60)return s+' s';
 let m=Math.floor(s/60);s%=60;if(m<10)return m+' min'+(s?' '+s+' s':'');
 if(m<60)return m+' min';let h=Math.floor(m/60);m%=60;return h+' h'+(m?' '+m+' min':'');}
function snap(v){const q=v<1000?10:v<10000?100:v<60000?1000:v<600000?5000:60000;return Math.round(v/q)*q}
function toMs(d,p){return d.pct?Math.round(p/10):snap(d.lo*Math.pow(d.hi/d.lo,p/1000))}
function toPos(d,v){if(d.pct)return Math.round(Math.min(100,Math.max(0,v))*10);
 const p=1000*Math.log(Math.max(v,d.lo)/d.lo)/Math.log(d.hi/d.lo);return Math.round(Math.min(1000,Math.max(0,p)))}
const val=(d,v)=>d.pct?v+' %':fmt(v);

$('sets').innerHTML=S.map(d=>`<div class="card set" id="c_${d.id}">
 <div class="hd"><h2 id="h_${d.id}" data-t="${d.id}"></h2>${d.sw?'<label class="sw"><input type="checkbox" id="sw_'+d.id+'"><i></i></label>':''}</div>
 <p id="p_${d.id}" data-t="${d.id}D"></p>
 <div class="val" id="v_${d.id}"></div>
 <div class="sl"><div class="tr"><i id="f_${d.id}"></i></div>
 ${d.k.map((k,i)=>`<input type="range" min="0" max="1000" id="${d.id}${i}">`).join('')}</div>
 <div class="ends">${d.sh?'<span data-t="soft"></span><span data-t="hard"></span>':`<span>${fmt(d.lo)}</span><span>${fmt(d.hi)}</span>`}</div></div>`).join('');

// With the strength output on, "push" is the push length and "str" the strength; off = push length is the strength
function labels(){const on=cfg.useStr==1;
 $('c_str').style.display=on?'':'none';
 $('h_push').dataset.t=on?'pushL':'push';$('p_push').dataset.t=on?'pushLD':'pushD';
 ['h_push','p_push'].forEach(i=>$(i).textContent=t($(i).dataset.t))}

function draw(d){
 const a=$(d.id+'0'),b=$(d.id+'1'),f=$('f_'+d.id);
 const pa=+a.value,pb=+b.value,va=toMs(d,pa),vb=toMs(d,pb);
 f.style.left=pa/10+'%';f.style.right=(100-pb/10)+'%';
 const off=d.sw&&!pauseOn;
 $('v_'+d.id).innerHTML=off?'<span class="mm"><small>&nbsp;</small>'+t('never')+'</span>'
  :`<span class="mm"><small>MIN</small>${val(d,va)}</span><span class="mm"><small>MAX</small>${val(d,vb)}</span>`;
 cfg[d.k[0]]=off?0:va;cfg[d.k[1]]=vb;
 if(d.sw){$('c_every').classList.toggle('off',off);$('c_plen').classList.toggle('off',off)}
}
const changed=d=>{lastIn=Date.now();draw(d);setDirty(true);clearTimeout(tSend);tSend=setTimeout(live,120)};
S.forEach(d=>{
 const a=$(d.id+'0'),b=$(d.id+'1');
 const on=e=>{ // keep handles in order; push the other one along
  if(+a.value>+b.value){if(e.target==a)b.value=a.value;else a.value=b.value}
  a.style.zIndex=e.target==a?2:1;b.style.zIndex=e.target==b?2:1;
  if(d.sw&&!pauseOn){pauseOn=true;$('sw_'+d.id).checked=true}   // touching the slider turns pauses on
  changed(d)};
 a.oninput=b.oninput=on;
 if(d.sw)$('sw_'+d.id).onchange=e=>{pauseOn=e.target.checked;changed(d)};});

function setSliders(c){cfg=c;rev=c.rev;loaded=true;
 pauseOn=c.hangEvery>0;$('sw_every').checked=pauseOn;
 S.forEach(d=>{$(d.id+'0').value=toPos(d,d.pct?c[d.k[0]]:(c[d.k[0]]||c[d.k[1]]));$(d.id+'1').value=toPos(d,c[d.k[1]]);draw(d)});
 labels()}
function body(){const p=new URLSearchParams();S.forEach(d=>d.k.forEach(k=>p.append(k,cfg[k])));return p}
async function live(){tSend=0;busy++;try{const c=await (await fetch('/api/live',{method:'POST',body:body()})).json();rev=c.rev}catch(e){}busy--}
async function post(u,b){const r=await fetch(u,{method:'POST',body:b});return r.json()}
async function save(){clearTimeout(tSend);setSliders(await post('/api/config',body()));setDirty(false);msg(t('saved'))}
async function undo(){clearTimeout(tSend);setSliders(await post('/api/revert'));setDirty(false);msg(t('undone'))}
async function wifiStay(){show(await post('/api/wifi'));msg(t('wifiReset'))}
async function toggleRun(){show(await post(st.userRun?'/api/stop':'/api/start'))}
function setDirty(v){dirty=v;$('save').classList.toggle('show',v)}
function msg(s){const m=$('msg');m.textContent=s;m.classList.add('show');clearTimeout(m.t);m.t=setTimeout(()=>m.classList.remove('show'),2200)}

function show(s){
 st=s;let now,next='';
 const P={'kick':'pushing','resting':'resting','pause':'pause','releasing brake':'starting','engaging brake':'stopping','stopped':'stopped'};
 now=t(s.warn?'warn':(P[s.phase]||'stopped'));
 if(!s.running&&s.userRun)now=s.hold=='VFD fault'?t('waitVfd'):now;
 const bar=$('bar'),bi=$('barI');
 bar.classList.toggle('push',s.phase=='kick');
 if(s.nextKickIn>=0){next=t('nextIn')+fmt(s.nextKickIn);
  const tot=s.phase=='pause'?s.hangTime:s.lastDrop;bi.style.width=tot?Math.min(100,100*(1-s.nextKickIn/tot))+'%':'0'}
 else if(s.phase!='kick')bi.style.width='0';
 $('now').textContent=now;$('next').textContent=next;
 const r=$('run');r.textContent=s.userRun?t('stop'):t('start');r.className=s.userRun?'stop':'go';
 $('kick').disabled=!(s.running&&(s.phase=='resting'||s.phase=='pause'));
 $('wf').textContent=s.apOffIn<0?t('wifiAlways'):s.apOffIn==0?t('wifiConn'):t('wifiOff')+fmt(s.apOffIn);
 $('wfb').style.display=s.apOffIn<0?'none':'';
 const idle=!busy&&!tSend&&Date.now()-lastIn>1500;
 if(idle&&s.dirty!==undefined&&s.dirty!=dirty)setDirty(s.dirty);
 if(idle&&loaded&&s.rev!==undefined&&s.rev!=rev)fetch('/api/config').then(r=>r.json()).then(c=>{if(Date.now()-lastIn>1500)setSliders(c)});
}
async function poll(){try{show(await (await fetch('/api/status')).json())}catch(e){$('now').textContent=t('offline');$('next').textContent=''}setTimeout(poll,1000)}

function setLang(l){L=l;try{localStorage.setItem('lang',l)}catch(e){}
 document.documentElement.lang=l;$('lEN').classList.toggle('on',l=='en');$('lDE').classList.toggle('on',l=='de');
 document.querySelectorAll('[data-t]').forEach(e=>e.textContent=t(e.dataset.t));
 if(loaded){S.forEach(draw);labels()}if(st.phase)show(st)}

setLang(L);
fetch('/api/config').then(r=>r.json()).then(setSliders);
poll();
</script></body></html>)HTML";

#include "HTTPServerController.h"
#include "EffectDispatch.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include "RuntimeLog.h"

static const char* TAG = "HTTP";

// Static instance pointer for callbacks
HTTPServerController* HTTPServerController::instance = nullptr;

// Static HTML content (pure HTML/AJAX - no server-side generation)
const char HTML_PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Kinemachina SFX Controller</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
*{box-sizing:border-box;}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#0a0a0a 0%,#1a1a2e 50%,#0f0f1e 100%);color:#e0e0e0;min-height:100vh;background-attachment:fixed;}
h1{font-size:2.5em;margin:0 0 30px 0;text-align:center;color:#00d4ff;text-shadow:0 0 20px rgba(0,212,255,0.5),0 0 40px rgba(0,212,255,0.3);letter-spacing:2px;font-weight:bold;text-transform:uppercase;}
h2{font-size:1.8em;margin-top:30px;margin-bottom:15px;color:#00ff88;text-shadow:0 0 10px rgba(0,255,136,0.4);border-top:2px solid #00ff88;padding-top:15px;letter-spacing:1px;}
h3{font-size:1.3em;color:#00d4ff;margin:15px 0 10px 0;text-shadow:0 0 8px rgba(0,212,255,0.3);}
button{padding:12px 24px;margin:5px;font-size:16px;font-weight:600;border:none;border-radius:8px;cursor:pointer;transition:all 0.3s ease;text-transform:uppercase;letter-spacing:0.5px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}
button:hover{transform:translateY(-2px);box-shadow:0 6px 12px rgba(0,0,0,0.4);}
button:active{transform:translateY(0);}
.btn-primary{background:linear-gradient(135deg,#00ff88 0%,#00cc6a 100%);color:#000;text-shadow:none;}
.btn-primary:hover{background:linear-gradient(135deg,#00ffaa 0%,#00ff88 100%);box-shadow:0 0 15px rgba(0,255,136,0.5);}
.btn-danger{background:linear-gradient(135deg,#ff4444 0%,#cc0000 100%);color:white;}
.btn-danger:hover{background:linear-gradient(135deg,#ff6666 0%,#ff4444 100%);box-shadow:0 0 15px rgba(255,68,68,0.5);}
.btn-warning{background:linear-gradient(135deg,#ffaa00 0%,#ff8800 100%);color:#000;text-shadow:none;}
.btn-warning:hover{background:linear-gradient(135deg,#ffcc00 0%,#ffaa00 100%);box-shadow:0 0 15px rgba(255,170,0,0.5);}
.btn-info{background:linear-gradient(135deg,#00d4ff 0%,#0099cc 100%);color:#000;text-shadow:none;}
.btn-info:hover{background:linear-gradient(135deg,#00e6ff 0%,#00d4ff 100%);box-shadow:0 0 15px rgba(0,212,255,0.5);}
.btn-purple{background:linear-gradient(135deg,#aa44ff 0%,#8800cc 100%);color:white;}
.btn-purple:hover{background:linear-gradient(135deg,#bb66ff 0%,#aa44ff 100%);box-shadow:0 0 15px rgba(170,68,255,0.5);}
.btn-gold{background:linear-gradient(135deg,#ffd700 0%,#ffaa00 100%);color:#000;text-shadow:none;}
.btn-gold:hover{background:linear-gradient(135deg,#ffed4e 0%,#ffd700 100%);box-shadow:0 0 15px rgba(255,215,0,0.5);}
input,select{padding:10px;margin:5px;width:300px;border-radius:6px;border:2px solid #333;background:#1a1a2e;color:#e0e0e0;font-size:14px;transition:all 0.3s ease;}
input:focus,select:focus{outline:none;border-color:#00d4ff;box-shadow:0 0 10px rgba(0,212,255,0.3);background:#1f1f3e;}
.status{padding:15px;background:linear-gradient(135deg,#1a1a2e 0%,#0f0f1e 100%);margin:15px 0;border-radius:8px;border:2px solid #00d4ff;box-shadow:0 0 20px rgba(0,212,255,0.2),inset 0 0 20px rgba(0,212,255,0.05);}
.settings-section{margin-top:15px;padding-top:15px;border-top:2px solid #00d4ff;}
.effect-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;margin:15px 0;}
.effect-grid>div{background:linear-gradient(135deg,#1a1a2e 0%,#0f0f1e 100%);border:2px solid #333;padding:15px;border-radius:8px;transition:all 0.3s ease;box-shadow:0 4px 6px rgba(0,0,0,0.3);}
.effect-grid>div:hover{border-color:#00d4ff;box-shadow:0 0 15px rgba(0,212,255,0.3),0 6px 12px rgba(0,0,0,0.4);transform:translateY(-2px);}
label{color:#00d4ff;font-weight:500;display:block;margin:10px 0 5px 0;}
p{line-height:1.6;color:#b0b0b0;}
small{color:#888;font-size:0.9em;}
strong{color:#00ff88;font-weight:600;}
#loopStatus{background:linear-gradient(135deg,#1a1a2e 0%,#0f0f1e 100%);border:2px solid #00ff88;box-shadow:0 0 15px rgba(0,255,136,0.2);}
.effect-grid .effect-card.effect-card-playing{border-color:#00ff88;box-shadow:0 0 20px rgba(0,255,136,0.4),inset 0 0 15px rgba(0,255,136,0.08);background:linear-gradient(135deg,#1a2e1a 0%,#0f1e0f 100%);}
.effect-grid .effect-card.effect-card-playing::before{content:'NOW PLAYING';position:absolute;top:8px;right:8px;font-size:9px;font-weight:bold;color:#00ff88;text-shadow:0 0 6px rgba(0,255,136,0.8);letter-spacing:1px;}
.effect-grid .effect-card{position:relative;}
#effectsTransport{margin-bottom:15px;padding:12px 15px;background:linear-gradient(135deg,#1a1a2e 0%,#0f0f1e 100%);border:2px solid #333;border-radius:8px;display:flex;flex-wrap:wrap;align-items:center;gap:12px;}
#effectsTransport .now-playing-label{color:#00ff88;font-weight:600;margin-right:4px;}
#effectsTransport #nowPlayingEffect{color:#e0e0e0;font-style:italic;}
#demoStatus{background:linear-gradient(135deg,#1a1a2e 0%,#0f0f1e 100%);border:2px solid #00d4ff;box-shadow:0 0 15px rgba(0,212,255,0.2);}
form{background:linear-gradient(135deg,#1a1a2e 0%,#0f0f1e 100%);padding:15px;border-radius:8px;border:2px solid #333;margin:10px 0;box-shadow:0 4px 6px rgba(0,0,0,0.3);}
@media (max-width:768px){h1{font-size:1.8em;}h2{font-size:1.4em;}input,select{width:100%;max-width:300px;}button{width:100%;margin:5px 0;}}
</style>
</head>
<body>
<h1>Kinemachina SFX Controller</h1>
<div class='status'>
<strong>System Status:</strong><br>
<span id='audioStatus'>Loading...</span><br>
<span id='currentFile'>Loading...</span><br>
<span id='freeHeap'>Loading...</span><br>
<div class='settings-section'><strong>Current Settings (Persisted):</strong><br>
<span id='volumeDisplay'>Loading...</span><br>
<span id='brightnessDisplay'>Loading...</span><br>
<span id='ledEffectDisplay'>Loading...</span> <button onclick='stopLED()' id='statusStopLEDBtn' class='btn-danger' style='display:none;margin-left:10px;padding:6px 12px;font-size:12px;'>Stop LED Effect</button><br>
<span id='demoDelayDisplay'>Loading...</span><br>
<span id='wifiSSIDDisplay'>Loading...</span><br>
</div></div>
<h2>Audio</h2>
<form id='dirForm'><input type='text' id='dirPath' name='path' placeholder='/' value='/'>
<button type='submit' class='btn-primary'>List audio</button></form>
<button type='button' id='playSelectedBtn' class='btn-primary' style='margin-left:8px;'>Play</button>
<button onclick='stopAudio()' class='btn-danger'>Stop Audio</button>
<button onclick='stopLED()' class='btn-danger' style='margin-left:10px;'>Stop LED Effects</button>
<div id='dirListing' style='margin-top:10px;padding:12px;background:rgba(0,0,0,0.2);border-radius:6px;min-height:40px;font-family:monospace;font-size:13px;max-height:300px;overflow-y:auto;'></div>
<h3>Volume</h3>
<form id='volumeForm'><input type='number' id='volumeInput' name='volume' min='0' max='21' value='0'>
<button type='submit' class='btn-primary'>Set Volume</button></form>
<h2>Configured Effects</h2>
<div id='loopStatus' style='padding:15px;margin-bottom:15px;'>
<strong>Loop Status:</strong> <span id='currentLoop'>None</span>
<button onclick='stopLoop()' id='stopLoopBtn' class='btn-danger' style='display:none;margin-left:10px;'>Stop Loop</button>
</div>
<div id='effectsTransport'>
<span class='now-playing-label'>Now playing:</span> <span id='nowPlayingEffect'>None</span>
<button onclick='playCurrentEffect()' class='btn-primary' title='Play selected effect'>Play</button>
<button onclick='stopCurrentEffect()' class='btn-danger' title='Stop audio and LED'>Stop</button>
<button onclick='previousEffect()' class='btn-info' title='Previous effect'>Previous</button>
<button onclick='nextEffect()' class='btn-info' title='Next effect'>Next</button>
</div>
<div id='effectsList' class='effect-grid'>Loading effects...</div>
<h2>LED Effects (Direct)</h2>
<div id='ledEffectsContainer'>Loading effects...</div>
<h3>Brightness</h3>
<form id='brightnessForm'><input type='number' id='brightnessInput' name='brightness' min='0' max='255' value='0'>
<button type='submit' class='btn-primary'>Set Brightness</button></form>
<h2>Configuration</h2>
<div class='status'>
<h3>WiFi Settings</h3>
<form id='wifiForm'><input type='text' id='wifiSSID' name='ssid' placeholder='WiFi SSID' value=''><br>
<input type='password' id='wifiPassword' name='password' placeholder='WiFi Password'><br>
<button type='submit' class='btn-primary'>Save WiFi Credentials</button>
<p style='font-size:12px;color:#888;margin-top:10px;'>Note: Changes require reboot to take effect. Credentials are saved to Preferences.</p>
</form>
<h3>MQTT Settings</h3>
<form id='mqttForm'>
<label>Enabled: <input type='checkbox' id='mqttEnabled' name='enabled'></label><br>
<label>Broker: <input type='text' id='mqttBroker' name='broker' placeholder='hostname or IP' maxlength='127' style='width:300px;'></label><br>
<label>Port: <input type='number' id='mqttPort' name='port' min='1' max='65535' value='1883' style='width:80px;'></label><br>
<label>Username: <input type='text' id='mqttUsername' name='username' placeholder='optional' maxlength='63' style='width:300px;'></label><br>
<label>Password: <input type='password' id='mqttPassword' name='password' placeholder='leave blank to keep current' maxlength='63' style='width:300px;'></label><br>
<label>Device ID: <input type='text' id='mqttDeviceId' name='device_id' placeholder='optional' maxlength='31' style='width:300px;'></label><br>
<label>Base topic: <input type='text' id='mqttBaseTopic' name='base_topic' placeholder='e.g. kinemachina/sfx' maxlength='63' style='width:300px;'></label><br>
<label>QoS commands: <input type='number' id='mqttQosCommands' name='qos_commands' min='0' max='2' value='1' style='width:50px;'></label><br>
<label>QoS status: <input type='number' id='mqttQosStatus' name='qos_status' min='0' max='2' value='0' style='width:50px;'></label><br>
<label>Keepalive (s): <input type='number' id='mqttKeepalive' name='keepalive' min='1' max='65535' value='60' style='width:80px;'></label><br>
<label>Clean session: <input type='checkbox' id='mqttCleanSession' name='clean_session'></label><br>
<button type='submit' class='btn-primary'>Save MQTT config</button>
</form>
<p style='font-size:12px;color:#888;margin-top:10px;'>MQTT config is saved to Preferences. Reboot to apply.</p>
<p style='font-size:12px;color:#aaa;margin-top:15px;'><strong>Config File:</strong> Create /config.json on SD card to load settings on boot:<br>
{<br>
&nbsp;&nbsp;"wifi": { "ssid": "YourNetwork", "password": "YourPassword" },<br>
&nbsp;&nbsp;"audio": { "volume": 21 },<br>
&nbsp;&nbsp;"led": { "brightness": 128 },<br>
&nbsp;&nbsp;"demo": { "delay": 5000 },<br>
&nbsp;&nbsp;"effects": {<br>
&nbsp;&nbsp;&nbsp;&nbsp;"godzilla_roar": { "audio": "/sounds/godzilla.mp3", "led": "atomic_breath" },<br>
&nbsp;&nbsp;&nbsp;&nbsp;"ghidorah_attack": { "audio": "/sounds/ghidorah.mp3", "led": "gravity_beam" },<br>
&nbsp;&nbsp;&nbsp;&nbsp;"fire_breath": { "led": "fire_breath" },<br>
&nbsp;&nbsp;&nbsp;&nbsp;"background_music": { "audio": "/sounds/music.mp3" }<br>
&nbsp;&nbsp;}<br>
}</p>
</div>
<h2>Demo Mode</h2>
<div class='status' id='demoStatus'>Loading...</div>
<div style='margin:15px 0;padding:15px;'>
<h3>Configuration</h3>
<label>Mode: <select id='demoMode' style='padding:8px;margin:5px;'>
<option value='audio_led'>Audio + LED</option>
<option value='audio_only'>Audio Only</option>
<option value='led_only'>LED Only</option>
<option value='effects'>Effects</option>
</select></label><br>
<label>Order: <select id='demoOrder' style='padding:8px;margin:5px;'>
<option value='random'>Random</option>
<option value='sequential'>Sequential</option>
</select></label><br>
<label>Delay (ms): <input type='number' id='demoDelay' value='5000' min='1000' max='60000' step='1000' style='padding:8px;margin:5px;width:150px;'></label>
</div>
<div style='margin:15px 0;'>
<button onclick='startDemo()' class='btn-primary'>Start Demo</button>
<button onclick='stopDemo()' class='btn-danger'>Stop Demo</button>
<button onclick='pauseDemo()' class='btn-warning'>Pause</button>
<button onclick='resumeDemo()' class='btn-info'>Resume</button>
</div>
<p>Demo mode can play audio files, LED effects, or configured effects. Choose random or sequential playback order.</p>
<script>
let currentLooping='';
let effectNames=[];
let currentEffectIndex=0;
let currentPlayingEffectName='';
function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{
if(d.status==='ok'){
document.getElementById('audioStatus').textContent='Audio: '+(d.playing?'Playing':'Stopped');
document.getElementById('currentFile').textContent='Current File: '+(d.currentFile||'None');
document.getElementById('freeHeap').textContent='Free Heap: '+d.freeHeap+' bytes';
document.getElementById('volumeDisplay').textContent='Volume: '+d.volume+' / 21';
document.getElementById('brightnessDisplay').textContent='LED Brightness: '+d.brightness+' / 255';
document.getElementById('ledEffectDisplay').textContent='LED Effect: '+(d.ledEffectName||'None');
let ledStopBtn=document.getElementById('statusStopLEDBtn');
if(d.ledEffect&&d.ledEffect!==-1&&d.ledEffectName&&d.ledEffectName!=='None'){
ledStopBtn.style.display='inline-block';
}else{
ledStopBtn.style.display='none';
}
document.getElementById('demoDelayDisplay').textContent='Demo Delay: '+(d.demoDelay/1000)+' seconds';
document.getElementById('wifiSSIDDisplay').textContent='WiFi SSID: '+(d.wifiSSID||'Not set');
document.getElementById('volumeInput').value=d.volume||0;
document.getElementById('brightnessInput').value=d.brightness||0;
document.getElementById('wifiSSID').value=d.wifiSSID||'';
document.getElementById('demoDelay').value=d.demoDelay||5000;
}}).catch(e=>console.error('Status error:',e));}
function updateEffectHighlight(){
let nowEl=document.getElementById('nowPlayingEffect');
nowEl.textContent=currentPlayingEffectName||'None';
nowEl.style.color=currentPlayingEffectName?'#00ff88':'#888';
nowEl.style.fontWeight=currentPlayingEffectName?'bold':'normal';
document.querySelectorAll('.effect-card').forEach(el=>{
if(parseInt(el.getAttribute('data-index'),10)===currentEffectIndex&&currentPlayingEffectName){el.classList.add('effect-card-playing');}
else{el.classList.remove('effect-card-playing');}
});
}
function loadEffects(){fetch('/effects/list').then(r=>r.json()).then(d=>{
if(d.status==='ok'){
if(d.currentLoop)currentLooping=d.currentLoop;
effectNames=d.effects.map(e=>e.name);
if(currentEffectIndex>=effectNames.length)currentEffectIndex=Math.max(0,effectNames.length-1);
let html='';
d.effects.forEach((eff,i)=>{
let desc='';
if(eff.hasAudio&&eff.hasLED)desc='Audio + LED';
else if(eff.hasAudio)desc='Audio only';
else if(eff.hasLED)desc='LED only';
let loopBadge=eff.loop?'<span style="background:linear-gradient(135deg,#ffaa00 0%,#ff8800 100%);color:#000;padding:3px 8px;border-radius:4px;font-size:11px;margin-left:5px;font-weight:bold;text-transform:uppercase;box-shadow:0 0 8px rgba(255,170,0,0.4);">LOOP</span>':'';
html+='<div class="effect-card" data-index="'+i+'">';
html+='<strong>'+eff.name.replace(/</g,'&lt;').replace(/>/g,'&gt;')+'</strong>'+loopBadge+'<br>';
html+='<small style="color:#888;margin-top:5px;display:block;">'+desc+'</small>';
html+='<button onclick="executeEffectByIndex('+i+')" class="btn-primary" style="margin-top:10px;width:100%;">Execute</button>';
html+='</div>';
});
if(html==='')html='<p>No effects configured. Add effects to /config.json on SD card.</p>';
document.getElementById('effectsList').innerHTML=html;
updateEffectHighlight();
updateLoopStatus();
}}).catch(e=>{console.error(e);document.getElementById('effectsList').innerHTML='Error loading effects';});}
function executeEffectByIndex(i){if(i<0||i>=effectNames.length)return;executeEffect(effectNames[i]);}
function executeEffect(name){fetch('/effects/execute',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(name)}).then(r=>r.json()).then(d=>{
if(d.looping){currentLooping=name;updateLoopStatus();}
currentPlayingEffectName=name;
let idx=effectNames.indexOf(name);
if(idx>=0)currentEffectIndex=idx;
updateEffectHighlight();
loadEffects();
updateStatus();
}).catch(e=>console.error('Error:',e));}
function playCurrentEffect(){if(effectNames.length===0)return;executeEffect(effectNames[currentEffectIndex]);}
function stopCurrentEffect(){currentLooping='';currentPlayingEffectName='';updateEffectHighlight();updateLoopStatus();fetch('/effects/stop-loop',{method:'POST'}).then(()=>fetch('/stop',{method:'POST'}).then(()=>fetch('/led/stop',{method:'POST'}).then(()=>{loadEffects();updateStatus();}).catch(()=>{loadEffects();updateStatus();})).catch(()=>{loadEffects();updateStatus();})).catch(()=>{loadEffects();updateStatus();});}
function nextEffect(){if(effectNames.length===0)return;currentEffectIndex=(currentEffectIndex+1)%effectNames.length;executeEffect(effectNames[currentEffectIndex]);}
function previousEffect(){if(effectNames.length===0)return;currentEffectIndex=(currentEffectIndex-1+effectNames.length)%effectNames.length;executeEffect(effectNames[currentEffectIndex]);}
function stopLoop(){fetch('/effects/stop-loop',{method:'POST'}).then(r=>r.json()).then(d=>{currentLooping='';updateLoopStatus();loadEffects();updateStatus();}).catch(e=>console.error('Error:',e));}
function loadMQTTConfig(){fetch('/settings/mqtt').then(r=>r.json()).then(d=>{
if(d.status==='ok'){
document.getElementById('mqttEnabled').checked=!!d.enabled;
document.getElementById('mqttBroker').value=d.broker||'';
document.getElementById('mqttPort').value=d.port||1883;
document.getElementById('mqttUsername').value=d.username||'';
document.getElementById('mqttDeviceId').value=d.device_id||'';
document.getElementById('mqttBaseTopic').value=d.base_topic||'';
document.getElementById('mqttQosCommands').value=d.qos_commands!==undefined?d.qos_commands:1;
document.getElementById('mqttQosStatus').value=d.qos_status!==undefined?d.qos_status:0;
document.getElementById('mqttKeepalive').value=d.keepalive||60;
document.getElementById('mqttCleanSession').checked=!!d.clean_session;
}
}).catch(e=>console.error('Error loading MQTT config:',e));}
function updateLoopStatus(){
let statusEl=document.getElementById('currentLoop');
let btnEl=document.getElementById('stopLoopBtn');
if(currentLooping){
statusEl.textContent=currentLooping;
statusEl.style.color='#00ff88';
statusEl.style.textShadow='0 0 10px rgba(0,255,136,0.5)';
statusEl.style.fontWeight='bold';
btnEl.style.display='inline-block';
}else{
statusEl.textContent='None';
statusEl.style.color='#888';
statusEl.style.textShadow='none';
statusEl.style.fontWeight='normal';
btnEl.style.display='none';
}
}
function stopAudio(){fetch('/stop',{method:'POST'}).then(r=>r.json()).then(d=>{updateStatus();}).catch(e=>console.error('Error:',e));}
function setLEDEffect(effect){fetch('/led/effect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'effect='+encodeURIComponent(effect)}).then(r=>r.json()).then(d=>{updateStatus();}).catch(e=>console.error('Error:',e));}
function stopLED(){fetch('/led/stop',{method:'POST'}).then(r=>r.json()).then(d=>{updateStatus();}).catch(e=>console.error('Error:',e));}
function loadLEDEffects(){fetch('/led/effects/list').then(r=>{
if(!r.ok){throw new Error('HTTP error: '+r.status);}
return r.json();
}).then(d=>{
if(d.status==='ok'&&d.effects){
let effectsByCategory={};
d.effects.forEach(eff=>{
if(!effectsByCategory[eff.category])effectsByCategory[eff.category]=[];
effectsByCategory[eff.category].push(eff);
});
let html='';
for(let category in effectsByCategory){
let gpioPin=category==='Strip Effects'?'GPIO 6':'GPIO 5';
let sectionClass=category==='Strip Effects'?'strip-effects-section':'matrix-effects-section';
html+='<div class="'+sectionClass+'" style="margin-bottom:30px;padding:15px;border-radius:8px;border:2px solid '+(category==='Strip Effects'?'#4a90e2':'#e24a4a')+';background:'+(category==='Strip Effects'?'rgba(74,144,226,0.05)':'rgba(226,74,74,0.05)')+';">';
html+='<h3 style="margin-top:0;color:'+(category==='Strip Effects'?'#4a90e2':'#e24a4a')+';">'+category+' <span style="font-size:0.7em;font-weight:normal;color:#666;">('+gpioPin+')</span></h3>';
html+='<div class="effect-grid">';
effectsByCategory[category].forEach(eff=>{
html+='<button onclick="setLEDEffect(\''+eff.id.replace(/'/g,"\\'")+'\')" class="'+eff.buttonClass+'">'+eff.name.replace(/</g,'&lt;').replace(/>/g,'&gt;')+'</button>';
});
html+='</div>';
html+='</div>';
}
html+='<div style="margin-top:15px;"><button onclick="stopLED()" class="btn-danger" style="font-size:18px;padding:14px 28px;">Stop All LED Effects</button></div>';
document.getElementById('ledEffectsContainer').innerHTML=html;
}else{
console.error('Invalid response:',d);
document.getElementById('ledEffectsContainer').innerHTML='<p style="color:#ff4444;">Invalid response from server.</p>';
}
}).catch(e=>{console.error('Error loading LED effects:',e);document.getElementById('ledEffectsContainer').innerHTML='<p style="color:#ff4444;">Error loading effects: '+e.message+'. Please refresh the page.</p>';});}
function updateDemoStatus(){fetch('/demo/status').then(r=>r.json()).then(d=>{
let statusText='<strong>Status:</strong> '+(d.running?(d.paused?'Paused':'Running'):'Stopped');
statusText+='<br><strong>Audio Files:</strong> '+d.fileCount;
let modeNames=['Audio+LED','Audio Only','LED Only','Effects'];
let orderNames=['Random','Sequential'];
statusText+='<br><strong>Mode:</strong> '+(modeNames[d.mode]||'Unknown');
statusText+='<br><strong>Order:</strong> '+(orderNames[d.order]||'Unknown');
document.getElementById('demoStatus').innerHTML=statusText;
if(d.mode!==undefined)document.getElementById('demoMode').value=['audio_led','audio_only','led_only','effects'][d.mode]||'audio_led';
if(d.order!==undefined)document.getElementById('demoOrder').value=['random','sequential'][d.order]||'random';
}).catch(e=>console.error(e));}
function startDemo(){
let mode=document.getElementById('demoMode').value;
let order=document.getElementById('demoOrder').value;
let delay=document.getElementById('demoDelay').value;
fetch('/demo/start',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'delay='+encodeURIComponent(delay)+'&mode='+encodeURIComponent(mode)+'&order='+encodeURIComponent(order)}).then(r=>r.json()).then(d=>{updateDemoStatus();}).catch(e=>console.error('Error:',e));}
function stopDemo(){fetch('/demo/stop',{method:'POST'}).then(r=>r.json()).then(d=>{updateDemoStatus();}).catch(e=>console.error(e));}
function pauseDemo(){fetch('/demo/pause',{method:'POST'}).then(r=>r.json()).then(d=>{updateDemoStatus();}).catch(e=>console.error(e));}
function resumeDemo(){fetch('/demo/resume',{method:'POST'}).then(r=>r.json()).then(d=>{updateDemoStatus();}).catch(e=>console.error(e));}
let selectedAudioPath='';
document.getElementById('playSelectedBtn').addEventListener('click',function(){if(!selectedAudioPath)return;fetch('/play',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'file='+encodeURIComponent(selectedAudioPath)}).then(r=>r.json()).then(d=>{updateStatus();}).catch(e=>console.error('Error:',e));});
document.getElementById('volumeForm').addEventListener('submit',function(e){e.preventDefault();fetch('/volume',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'volume='+encodeURIComponent(document.getElementById('volumeInput').value)}).then(r=>r.json()).then(d=>{updateStatus();}).catch(e=>console.error('Error:',e));});
document.getElementById('brightnessForm').addEventListener('submit',function(e){e.preventDefault();fetch('/led/brightness',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'brightness='+encodeURIComponent(document.getElementById('brightnessInput').value)}).then(r=>r.json()).then(d=>{updateStatus();}).catch(e=>console.error('Error:',e));});
document.getElementById('wifiForm').addEventListener('submit',function(e){e.preventDefault();fetch('/settings/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(document.getElementById('wifiSSID').value)+'&password='+encodeURIComponent(document.getElementById('wifiPassword').value)}).then(r=>r.json()).then(d=>{updateStatus();}).catch(e=>console.error('Error:',e));});
document.getElementById('mqttForm').addEventListener('submit',function(e){e.preventDefault();var b='enabled='+(document.getElementById('mqttEnabled').checked?1:0)+'&broker='+encodeURIComponent(document.getElementById('mqttBroker').value)+'&port='+encodeURIComponent(document.getElementById('mqttPort').value)+'&username='+encodeURIComponent(document.getElementById('mqttUsername').value)+'&device_id='+encodeURIComponent(document.getElementById('mqttDeviceId').value)+'&base_topic='+encodeURIComponent(document.getElementById('mqttBaseTopic').value)+'&qos_commands='+encodeURIComponent(document.getElementById('mqttQosCommands').value)+'&qos_status='+encodeURIComponent(document.getElementById('mqttQosStatus').value)+'&keepalive='+encodeURIComponent(document.getElementById('mqttKeepalive').value)+'&clean_session='+(document.getElementById('mqttCleanSession').checked?1:0);if(document.getElementById('mqttPassword').value.length>0)b+='&password='+encodeURIComponent(document.getElementById('mqttPassword').value);fetch('/settings/mqtt',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b}).then(r=>r.json()).then(d=>{if(d.status==='ok'){alert(d.message||'MQTT config saved.');loadMQTTConfig();}else{alert(d.message||'Error saving MQTT config.');}}).catch(function(err){alert('Error: '+err.message);});});
document.getElementById('dirForm').addEventListener('submit',function(e){e.preventDefault();let path=document.getElementById('dirPath').value;let el=document.getElementById('dirListing');el.textContent='Loading...';el.style.color='';fetch('/dir?path='+encodeURIComponent(path)+'&audio_only=1').then(r=>r.json()).then(d=>{if(d.status==='ok'){if(!d.files||d.files.length===0){el.textContent='(empty)';el.style.color='#888';}else{let base=d.path==='/'?'':d.path;if(base&&!base.endsWith('/'))base+='/';let html='<strong>'+d.path+'</strong><br><span style="font-size:11px;color:#888;">Click a file to select, then click Play.</span><br>';d.files.forEach(f=>{if(f.type==='dir'){let dirPath=(base?(base+'/'):'/')+f.name;if(dirPath.startsWith('//'))dirPath=dirPath.slice(1);html+='<span class="dir-list-dir" data-path="'+dirPath.replace(/"/g,'&quot;')+'" style="cursor:pointer;display:block;padding:2px 0;">[DIR] '+f.name.replace(/</g,'&lt;').replace(/>/g,'&gt;')+'</span>';}else{let fullPath=(base?base+'/':'/')+f.name;if(fullPath.startsWith('//'))fullPath=fullPath.slice(1);html+='<span class="dir-list-file" data-path="'+fullPath.replace(/"/g,'&quot;')+'" style="cursor:pointer;display:block;padding:2px 0;" title="Click to select, then click Play">     '+f.name.replace(/</g,'&lt;').replace(/>/g,'&gt;')+(f.size!==undefined?' ('+f.size+' bytes)':'')+'</span>';}});el.innerHTML=html;el.style.color='';el.querySelectorAll('.dir-list-file,.dir-list-dir').forEach(node=>{node.addEventListener('click',function(){el.querySelectorAll('.dir-list-file,.dir-list-dir').forEach(n=>n.style.background='');this.style.background='rgba(0,200,100,0.25)';if(this.classList.contains('dir-list-file')){selectedAudioPath=this.getAttribute('data-path');}else{document.getElementById('dirPath').value=this.getAttribute('data-path');}});});}}else{el.textContent='Error: '+(d.message||'Unknown error');el.style.color='#ff6666';}}).catch(err=>{document.getElementById('dirListing').textContent='Error: '+err.message;document.getElementById('dirListing').style.color='#ff6666';});});
updateStatus();
loadEffects();
loadLEDEffects();
loadMQTTConfig();
updateDemoStatus();
setInterval(updateStatus,5000);
setInterval(loadEffects,5000);
setInterval(loadLEDEffects,10000);
setInterval(updateDemoStatus,3000);
</script>
</body>
</html>
)rawliteral";

HTTPServerController::HTTPServerController(char* ssid, char* password, int port)
    : wifiSSID(ssid), wifiPassword(password), httpPort(port),
      server(nullptr), audioController(nullptr), ledStripController(nullptr), ledMatrixController(nullptr), demoController(nullptr), settingsController(nullptr), bassMonoProcessor(nullptr),
      currentLoopingEffect(""), isLoopingAudio(false), isLoopingLED(false), loopingAudioFile(""), loopingAvSync(false), audioPlaybackFailed(false), audioLoopStartTime(0) {
    instance = this;
    loopingLEDEffectName[0] = '\0';
}

HTTPServerController::~HTTPServerController() {
    if (server) {
        delete server;
    }
    if (instance == this) {
        instance = nullptr;
    }
}

bool HTTPServerController::initWiFi() {
    ESP_LOGI(TAG, "Initializing WiFi...");
    ESP_LOGI(TAG, "Connecting to: %s", wifiSSID);
    
    WiFi.mode(WIFI_STA);
    
    // Set WiFi to persistent mode and configure for better reliability
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    
    // Set hostname for easier identification
    WiFi.setHostname("esp32-audio");
    
    WiFi.begin(wifiSSID, wifiPassword);
    
    // Wait for connection with timeout
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        ESP_LOGI(TAG, ".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi connected!");
        ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "Signal strength (RSSI): %d dBm", WiFi.RSSI());

        // Initialize mDNS for .local hostname resolution
        if (!MDNS.begin("esp32-audio")) {
            ESP_LOGE(TAG, "Error setting up mDNS responder!");
        } else {
            ESP_LOGI(TAG, "mDNS responder started");
            ESP_LOGI(TAG, "Hostname: esp32-audio.local");
        }
        
        return true;
    } else {
        ESP_LOGE(TAG, "WiFi connection failed!");
        return false;
    }
}

void HTTPServerController::handleRoot(AsyncWebServerRequest *request) {
    // Serve static HTML - all data is fetched via AJAX
    request->send(200, "text/html", HTML_PAGE);
}

void HTTPServerController::handlePlay(AsyncWebServerRequest *request) {
    if (request->hasParam("file", true)) {
        String newFile = request->getParam("file", true)->value();
        newFile.trim();
        
        if (newFile.length() > 0) {
            if (audioController->playFile(newFile)) {
                JsonDocument doc;
                doc["status"] = "ok";
                doc["message"] = "File queued: " + newFile;
                String json;
                serializeJson(doc, json);
                request->send(200, "application/json", json);
            } else {
                JsonDocument errorDoc;
                errorDoc["status"] = "error";
                errorDoc["message"] = "Invalid file path";
                String errorJson;
                serializeJson(errorDoc, errorJson);
                request->send(400, "application/json", errorJson);
            }
        } else {
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "No file specified";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(400, "application/json", errorJson);
        }
    } else {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Missing file parameter";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
    }
}

void HTTPServerController::handleStop(AsyncWebServerRequest *request) {
    audioController->stop();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Playback stopped";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleVolume(AsyncWebServerRequest *request) {
    // Read volume parameter from POST body (REST best practice)
    if (!request->hasParam("volume", true)) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Missing volume parameter in request body";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    int vol = request->getParam("volume", true)->value().toInt();
    if (vol >= 0 && vol <= 21) {
        audioController->setVolume(vol);
        if (settingsController) {
            settingsController->saveVolume(vol);  // Save to preferences
        }
        JsonDocument doc;
        doc["status"] = "ok";
        doc["message"] = "Volume set to " + String(vol);
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    } else {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Volume must be 0-21";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
    }
}

// Helper function to determine if an effect ID is a matrix effect based on original enum order
String HTTPServerController::getStripLEDEffectName(StripLEDController::EffectType effect) const {
    switch (effect) {
        case StripLEDController::EFFECT_ATOMIC_BREATH: return "Atomic Breath";
        case StripLEDController::EFFECT_GRAVITY_BEAM: return "Gravity Beam";
        case StripLEDController::EFFECT_FIRE_BREATH: return "Fire Breath";
        case StripLEDController::EFFECT_ELECTRIC_ATTACK: return "Electric Attack";
        case StripLEDController::EFFECT_BATTLE_DAMAGE: return "Battle Damage";
        case StripLEDController::EFFECT_VICTORY: return "Victory";
        case StripLEDController::EFFECT_IDLE: return "Idle";
        // Popular general effects
        case StripLEDController::EFFECT_RAINBOW: return "Rainbow";
        case StripLEDController::EFFECT_RAINBOW_CHASE: return "Rainbow Chase";
        case StripLEDController::EFFECT_COLOR_WIPE: return "Color Wipe";
        case StripLEDController::EFFECT_THEATER_CHASE: return "Theater Chase";
        case StripLEDController::EFFECT_PULSE: return "Pulse";
        case StripLEDController::EFFECT_BREATHING: return "Breathing";
        case StripLEDController::EFFECT_METEOR: return "Meteor";
        case StripLEDController::EFFECT_TWINKLE: return "Twinkle";
        case StripLEDController::EFFECT_WATER: return "Water";
        case StripLEDController::EFFECT_STROBE: return "Strobe";
        // Edge-lit acrylic disc effects
        case StripLEDController::EFFECT_RADIAL_OUT: return "Radial Out";
        case StripLEDController::EFFECT_RADIAL_IN: return "Radial In";
        case StripLEDController::EFFECT_SPIRAL: return "Spiral";
        case StripLEDController::EFFECT_ROTATING_RAINBOW: return "Rotating Rainbow";
        case StripLEDController::EFFECT_CIRCULAR_CHASE: return "Circular Chase";
        case StripLEDController::EFFECT_RADIAL_GRADIENT: return "Radial Gradient";
        default: return "None";
    }
}

String HTTPServerController::getMatrixLEDEffectName(MatrixLEDController::EffectType effect) const {
    switch (effect) {
        // Battle Arena - Fight Behaviors
        case MatrixLEDController::EFFECT_BEAM_ATTACK: return "Beam Attack";
        case MatrixLEDController::EFFECT_EXPLOSION: return "Explosion";
        case MatrixLEDController::EFFECT_IMPACT_WAVE: return "Impact Wave";
        case MatrixLEDController::EFFECT_DAMAGE_FLASH: return "Damage Flash";
        case MatrixLEDController::EFFECT_BLOCK_SHIELD: return "Block Shield";
        case MatrixLEDController::EFFECT_DODGE_TRAIL: return "Dodge Trail";
        case MatrixLEDController::EFFECT_CHARGE_UP: return "Charge Up";
        case MatrixLEDController::EFFECT_FINISHER_BEAM: return "Finisher Beam";
        case MatrixLEDController::EFFECT_GRAVITY_BEAM: return "Gravity Beam";
        case MatrixLEDController::EFFECT_ELECTRIC_ATTACK_MATRIX: return "Electric Attack (Matrix)";
        // Battle Arena - Dance Behaviors
        case MatrixLEDController::EFFECT_VICTORY_DANCE: return "Victory Dance";
        case MatrixLEDController::EFFECT_TAUNT_PATTERN: return "Taunt Pattern";
        case MatrixLEDController::EFFECT_POSE_STRIKE: return "Pose Strike";
        case MatrixLEDController::EFFECT_CELEBRATION_WAVE: return "Celebration Wave";
        case MatrixLEDController::EFFECT_CONFETTI: return "Confetti";
        case MatrixLEDController::EFFECT_HEART_EYES: return "Heart Eyes";
        case MatrixLEDController::EFFECT_POWER_UP_AURA: return "Power Up Aura";
        case MatrixLEDController::EFFECT_TRANSITION_FADE: return "Transition Fade";
        // Panel Effects
        case MatrixLEDController::EFFECT_GAME_OVER_CHIRON: return "Game Over Chiron";
        case MatrixLEDController::EFFECT_CYLON_EYE: return "Cylon Eye";
        case MatrixLEDController::EFFECT_PERLIN_INFERNO: return "Perlin Inferno";
        case MatrixLEDController::EFFECT_EMP_LIGHTNING: return "EMP Lightning";
        case MatrixLEDController::EFFECT_GAME_OF_LIFE: return "Game of Life";
        case MatrixLEDController::EFFECT_PLASMA_CLOUDS: return "Plasma Clouds";
        case MatrixLEDController::EFFECT_DIGITAL_FIREFLIES: return "Digital Fireflies";
        case MatrixLEDController::EFFECT_MATRIX_RAIN: return "Matrix Rain";
        case MatrixLEDController::EFFECT_DANCE_FLOOR: return "Dance Floor";
        case MatrixLEDController::EFFECT_ALIEN_CONTROL_PANEL: return "Alien Control Panel";
        case MatrixLEDController::EFFECT_ATOMIC_BREATH_MINUS_ONE: return "Atomic Breath (Godzilla Minus One)";
        case MatrixLEDController::EFFECT_ATOMIC_BREATH_MUSHROOM: return "Atomic Breath Mushroom Cloud";
        case MatrixLEDController::EFFECT_TRANSPORTER_TOS: return "Transporter (Star Trek TOS)";
        default: return "None";
    }
}

void HTTPServerController::handleStatus(AsyncWebServerRequest *request) {
    bool playing = audioController->isPlaying();
    String currentFile = audioController->getCurrentFile();
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    int volume = audioController->getVolume();
    // Get brightness from strip controller (or matrix if strip not available)
    uint8_t brightness = ledStripController ? ledStripController->getBrightness() : (ledMatrixController ? ledMatrixController->getBrightness() : 0);
    // Get current effect from both controllers
    int stripEffect = ledStripController ? (int)ledStripController->getCurrentEffect() : -1;
    int matrixEffect = ledMatrixController ? (int)ledMatrixController->getCurrentEffect() : -1;
    String effectName = "None";
    int currentEffectId = -1;
    if (stripEffect >= 0) {
        effectName = getStripLEDEffectName((StripLEDController::EffectType)stripEffect);
        currentEffectId = stripEffect;
    } else if (matrixEffect >= 0) {
        effectName = getMatrixLEDEffectName((MatrixLEDController::EffectType)matrixEffect);
        currentEffectId = matrixEffect;
    }
    unsigned long demoDelay = settingsController ? settingsController->loadDemoDelay(5000) : 5000;
    
    // Build JSON response with all status information using ArduinoJson
    JsonDocument doc;
    doc["status"] = "ok";
    doc["playing"] = playing;
    doc["currentFile"] = currentFile;
    doc["volume"] = volume;
    doc["brightness"] = brightness;
    doc["ledEffect"] = currentEffectId;
    doc["ledEffectName"] = effectName;
    doc["demoDelay"] = demoDelay;
    doc["wifiSSID"] = wifiSSID;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["wifiStatus"] = wifiConnected ? "connected" : "disconnected";
    if (wifiConnected) {
        doc["ipAddress"] = WiFi.localIP().toString();
    }
    
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

// Returns true if filename has a supported audio extension (case-insensitive).
static bool isSupportedAudioFile(const String& name) {
    if (name.length() < 4) return false;
    String lower = name;
    lower.toLowerCase();
    return lower.endsWith(".mp3") || lower.endsWith(".wav") || lower.endsWith(".aac") ||
           lower.endsWith(".m4a") || lower.endsWith(".flac") || lower.endsWith(".ogg");
}

void HTTPServerController::handleDir(AsyncWebServerRequest *request) {
    String dirPath = "/";
    if (request->hasParam("path")) {
        dirPath = request->getParam("path")->value();
        dirPath.trim();
    }
    
    if (dirPath.length() == 0) {
        dirPath = "/";
    } else if (!dirPath.startsWith("/")) {
        dirPath = "/" + dirPath;
    }
    
    bool audioOnly = request->hasParam("audio_only") && request->getParam("audio_only")->value() == "1";
    
    // Take mutex before accessing SD card with shorter timeout
    SemaphoreHandle_t mutex = audioController->getSDMutex();
    if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        // Use ArduinoJson to build JSON response
        JsonDocument doc;
        doc["status"] = "ok";
        doc["path"] = dirPath;
        JsonArray files = doc["files"].to<JsonArray>();
        
        File dir = SD.open(dirPath);
        if (dir && dir.isDirectory()) {
            int fileCount = 0;
            const int MAX_FILES = 100; // Limit to prevent memory issues
            
            while (fileCount < MAX_FILES) {
                File entry = dir.openNextFile();
                if (!entry) break;
                
                String entryName = entry.name();
                // Skip dotfiles (names starting with ".")
                if (entryName.length() > 0 && entryName.charAt(0) == '.') {
                    entry.close();
                    continue;
                }
                // When audio_only: list only directories and supported audio files
                if (audioOnly && !entry.isDirectory() && !isSupportedAudioFile(entryName)) {
                    entry.close();
                    continue;
                }
                
                JsonObject fileObj = files.add<JsonObject>();
                fileObj["name"] = entryName;
                fileObj["type"] = entry.isDirectory() ? "dir" : "file";
                if (!entry.isDirectory()) {
                    fileObj["size"] = entry.size();
                }
                
                entry.close();
                fileCount++;
                
                // Yield to allow other tasks to run
                if (fileCount % 10 == 0) {
                    vTaskDelay(1);
                }
            }
            dir.close();
        } else {
            if (dir) dir.close();
            xSemaphoreGive(mutex);
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "Directory not found";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(404, "application/json", errorJson);
            return;
        }
        
        String json;
        serializeJson(doc, json);
        xSemaphoreGive(mutex);
        request->send(200, "application/json", json);
    } else {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "SD card busy";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
    }
}

void HTTPServerController::handleLEDEffect(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "LED effect request received");

    if (!ledStripController && !ledMatrixController) {
        ESP_LOGE(TAG, "LED controllers not available");
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "LED controllers not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    // Debug: Check all parameters
    ESP_LOGI(TAG, "Method: %s", request->method() == HTTP_POST ? "POST" : "OTHER");
    ESP_LOGI(TAG, "Content-Type: %s", request->contentType().c_str());

    // Check both POST body and query parameters for debugging
    bool hasPostParam = request->hasParam("effect", true);
    bool hasQueryParam = request->hasParam("effect", false);
    ESP_LOGI(TAG, "Has POST param: %s, Has query param: %s", hasPostParam ? "yes" : "no", hasQueryParam ? "yes" : "no");
    
    // Read effect parameter from POST body (REST best practice)
    String effectName = "";
    if (hasPostParam) {
        effectName = request->getParam("effect", true)->value();
        ESP_LOGI(TAG, "Got effect from POST body: %s", effectName.c_str());
    } else if (hasQueryParam) {
        // Fallback to query param for debugging
        effectName = request->getParam("effect", false)->value();
        ESP_LOGI(TAG, "Got effect from query string: %s", effectName.c_str());
    } else {
        ESP_LOGE(TAG, "Missing effect parameter");
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Missing effect parameter in request body";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    effectName.trim();
    if (effectName.length() == 0) {
        ESP_LOGE(TAG, "Empty effect parameter");
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Empty effect parameter";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    ESP_LOGI(TAG, "Processing effect: %s", effectName.c_str());
    
    if (!dispatchLEDEffect(settingsController, effectName.c_str(), ledStripController, ledMatrixController)) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Invalid effect name or LED controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    ESP_LOGI(TAG, "LED effect started successfully");
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Effect started: " + effectName;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleLEDBrightness(AsyncWebServerRequest *request) {
    if (!ledStripController && !ledMatrixController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "LED controllers not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    // Read brightness parameter from POST body (REST best practice)
    if (!request->hasParam("brightness", true)) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Missing brightness parameter in request body";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    int brightness = request->getParam("brightness", true)->value().toInt();
    if (brightness >= 0 && brightness <= 255) {
        // Set brightness on both controllers (shared brightness)
        if (ledStripController) {
            ledStripController->setBrightness((uint8_t)brightness);
        }
        if (ledMatrixController) {
            ledMatrixController->setBrightness((uint8_t)brightness);
        }
        if (settingsController) {
            settingsController->saveBrightness((uint8_t)brightness);  // Save to preferences
        }
        JsonDocument doc;
        doc["status"] = "ok";
        doc["message"] = "Brightness set to " + String(brightness);
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    } else {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Brightness must be 0-255";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
    }
}

void HTTPServerController::handleLEDStop(AsyncWebServerRequest *request) {
    // Stop both controllers
    if (ledStripController) {
        ledStripController->stopEffect();
    }
    if (ledMatrixController) {
        ledMatrixController->stopEffect();
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "LED effect stopped";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleLEDEffectsList(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "LED effects list request received");

    if (!ledStripController && !ledMatrixController) {
        ESP_LOGW(TAG, "LED controllers not available, but returning effects list anyway");
    }
    
    // Build JSON response with all available effects organized by category using ArduinoJson
    JsonDocument doc;
    doc["status"] = "ok";
    JsonArray effects = doc["effects"].to<JsonArray>();
    
    // Helper function to add an effect
    auto addEffect = [&effects](const char* id, const char* name, const char* category, const char* buttonClass) {
        JsonObject effect = effects.add<JsonObject>();
        effect["id"] = id;
        effect["name"] = name;
        effect["category"] = category;
        effect["buttonClass"] = buttonClass;
    };
    
    // Strip Effects
    addEffect("atomic_breath", "Atomic Breath", "Strip Effects", "btn-info");
    addEffect("gravity_beam", "Gravity Beam", "Strip Effects", "btn-gold");
    addEffect("fire_breath", "Fire Breath", "Strip Effects", "btn-danger");
    addEffect("electric", "Electric Attack", "Strip Effects", "btn-info");
    addEffect("battle_damage", "Battle Damage", "Strip Effects", "btn-danger");
    addEffect("victory", "Victory", "Strip Effects", "btn-primary");
    addEffect("idle", "Idle", "Strip Effects", "btn-info");
    
    // Strip Effects
    addEffect("rainbow", "Rainbow", "Strip Effects", "btn-purple");
    addEffect("rainbow_chase", "Rainbow Chase", "Strip Effects", "btn-purple");
    addEffect("color_wipe", "Color Wipe", "Strip Effects", "btn-info");
    addEffect("theater_chase", "Theater Chase", "Strip Effects", "btn-info");
    addEffect("pulse", "Pulse", "Strip Effects", "btn-primary");
    addEffect("breathing", "Breathing", "Strip Effects", "btn-info");
    addEffect("meteor", "Meteor", "Strip Effects", "btn-warning");
    addEffect("twinkle", "Twinkle", "Strip Effects", "btn-warning");
    addEffect("water", "Water", "Strip Effects", "btn-info");
    addEffect("strobe", "Strobe", "Strip Effects", "btn-danger");
    
    // Strip Effects
    addEffect("radial_out", "Radial Out", "Strip Effects", "btn-purple");
    addEffect("radial_in", "Radial In", "Strip Effects", "btn-purple");
    addEffect("spiral", "Spiral", "Strip Effects", "btn-info");
    addEffect("rotating_rainbow", "Rotating Rainbow", "Strip Effects", "btn-primary");
    addEffect("circular_chase", "Circular Chase", "Strip Effects", "btn-info");
    addEffect("radial_gradient", "Radial Gradient", "Strip Effects", "btn-info");
    
    // Matrix Effects
    addEffect("beam_attack", "Beam Attack", "Matrix Effects", "btn-info");
    addEffect("explosion", "Explosion", "Matrix Effects", "btn-danger");
    addEffect("impact_wave", "Impact Wave", "Matrix Effects", "btn-info");
    addEffect("damage_flash", "Damage Flash", "Matrix Effects", "btn-danger");
    addEffect("block_shield", "Block Shield", "Matrix Effects", "btn-info");
    addEffect("dodge_trail", "Dodge Trail", "Matrix Effects", "btn-purple");
    addEffect("charge_up", "Charge Up", "Matrix Effects", "btn-warning");
    addEffect("finisher_beam", "Finisher Beam", "Matrix Effects", "btn-danger");
    addEffect("gravity_beam_attack", "Gravity Beam (Ghidorah)", "Matrix Effects", "btn-warning");
    addEffect("electric_attack_matrix", "Electric Attack (Matrix)", "Matrix Effects", "btn-info");
    
    // Matrix Effects
    addEffect("victory_dance", "Victory Dance", "Matrix Effects", "btn-primary");
    addEffect("taunt_pattern", "Taunt Pattern", "Matrix Effects", "btn-danger");
    addEffect("pose_strike", "Pose Strike", "Matrix Effects", "btn-warning");
    addEffect("celebration_wave", "Celebration Wave", "Matrix Effects", "btn-purple");
    addEffect("confetti", "Confetti", "Matrix Effects", "btn-primary");
    addEffect("heart_eyes", "Heart Eyes", "Matrix Effects", "btn-primary");
    addEffect("power_up_aura", "Power Up Aura", "Matrix Effects", "btn-info");
    addEffect("transition_fade", "Transition Fade", "Matrix Effects", "btn-info");
    
    // Matrix Effects
    addEffect("game_over_chiron", "Game Over Chiron", "Matrix Effects", "btn-danger");
    addEffect("cylon_eye", "Cylon Eye", "Matrix Effects", "btn-danger");
    addEffect("perlin_inferno", "Perlin Inferno", "Matrix Effects", "btn-warning");
    addEffect("emp_lightning", "EMP Lightning", "Matrix Effects", "btn-info");
    addEffect("game_of_life", "Game of Life", "Matrix Effects", "btn-primary");
    addEffect("plasma_clouds", "Plasma Clouds", "Matrix Effects", "btn-purple");
    addEffect("digital_fireflies", "Digital Fireflies", "Matrix Effects", "btn-warning");
    addEffect("matrix_rain", "Matrix Rain", "Matrix Effects", "btn-primary");
    addEffect("dance_floor", "Dance Floor", "Matrix Effects", "btn-purple");
    addEffect("alien_control_panel", "Alien Control Panel", "Matrix Effects", "btn-info");
    addEffect("atomic_breath_minus_one", "Atomic Breath (Godzilla Minus One)", "Matrix Effects", "btn-info");
    addEffect("atomic_breath_mushroom", "Atomic Breath Mushroom Cloud", "Matrix Effects", "btn-danger");
    addEffect("transporter_tos", "Transporter (Star Trek TOS)", "Matrix Effects", "btn-info");
    
    String json;
    serializeJson(doc, json);
    
    ESP_LOGD(TAG, "Sending LED effects list (JSON length: %u)", json.length());
    
    request->send(200, "application/json", json);
}

void HTTPServerController::handleDemoStart(AsyncWebServerRequest *request) {
    if (!demoController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Demo controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    unsigned long delayMs = 5000; // Default 5 seconds
    if (request->hasParam("delay", true)) {
        delayMs = request->getParam("delay", true)->value().toInt();
        if (delayMs < 1000) delayMs = 1000; // Minimum 1 second
        if (delayMs > 60000) delayMs = 60000; // Maximum 60 seconds
    }
    
    DemoController::DemoMode mode = DemoController::DEMO_AUDIO_LED; // Default
    if (request->hasParam("mode", true)) {
        String modeStr = request->getParam("mode", true)->value();
        modeStr.toLowerCase();
        if (modeStr == "audio_only") mode = DemoController::DEMO_AUDIO_ONLY;
        else if (modeStr == "led_only") mode = DemoController::DEMO_LED_ONLY;
        else if (modeStr == "effects") mode = DemoController::DEMO_EFFECTS;
        else if (modeStr == "audio_led") mode = DemoController::DEMO_AUDIO_LED;
    }
    
    DemoController::PlaybackOrder order = DemoController::ORDER_RANDOM; // Default
    if (request->hasParam("order", true)) {
        String orderStr = request->getParam("order", true)->value();
        orderStr.toLowerCase();
        if (orderStr == "sequential" || orderStr == "order") order = DemoController::ORDER_SEQUENTIAL;
        else order = DemoController::ORDER_RANDOM;
    }
    
    ESP_LOGI(TAG, "Starting demo: mode=%d, order=%d, delay=%lu", (int)mode, (int)order, delayMs);
    
    demoController->startDemo(delayMs, mode, order);
    if (settingsController) {
        settingsController->saveDemoDelay(delayMs);  // Save to preferences
        settingsController->saveDemoMode((int)mode);
        settingsController->saveDemoOrder((int)order);
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Demo mode started";
    doc["mode"] = (int)mode;
    doc["order"] = (int)order;
    doc["delay"] = delayMs;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleDemoStop(AsyncWebServerRequest *request) {
    if (!demoController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Demo controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    demoController->stopDemo();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Demo mode stopped";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleDemoPause(AsyncWebServerRequest *request) {
    if (!demoController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Demo controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    demoController->pauseDemo();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Demo mode paused";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleDemoResume(AsyncWebServerRequest *request) {
    if (!demoController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Demo controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    demoController->resumeDemo();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Demo mode resumed";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleDemoStatus(AsyncWebServerRequest *request) {
    if (!demoController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Demo controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    bool running = demoController->isRunning();
    bool paused = demoController->isPaused();
    int fileCount = demoController->getAudioFileCount();
    int mode = (int)demoController->getDemoMode();
    int order = (int)demoController->getPlaybackOrder();
    
    JsonDocument doc;
    doc["status"] = "ok";
    doc["running"] = running;
    doc["paused"] = paused;
    doc["fileCount"] = fileCount;
    doc["mode"] = mode;
    doc["order"] = order;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleSettingsClear(AsyncWebServerRequest *request) {
    if (!settingsController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Settings controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    settingsController->clearAll();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "All settings cleared";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleWiFiConfig(AsyncWebServerRequest *request) {
    if (!settingsController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Settings controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    if (request->method() == HTTP_POST) {
        bool hasSSID = request->hasParam("ssid", true);
        bool hasPassword = request->hasParam("password", true);
        
        if (hasSSID && hasPassword) {
            String ssid = request->getParam("ssid", true)->value();
            String password = request->getParam("password", true)->value();
            
            if (ssid.length() > 0 && ssid.length() < 64) {
                settingsController->saveWiFiSSID(ssid.c_str());
                settingsController->saveWiFiPassword(password.c_str());
                
                // Update the buffers (will be used on next connection attempt)
                strncpy(wifiSSID, ssid.c_str(), 63);
                wifiSSID[63] = '\0';
                strncpy(wifiPassword, password.c_str(), 63);
                wifiPassword[63] = '\0';
                
                JsonDocument doc;
                doc["status"] = "ok";
                doc["message"] = "WiFi credentials saved. Reboot required to apply.";
                doc["ssid"] = ssid;
                String json;
                serializeJson(doc, json);
                request->send(200, "application/json", json);
            } else {
                JsonDocument errorDoc;
                errorDoc["status"] = "error";
                errorDoc["message"] = "Invalid SSID length (1-63 characters)";
                String errorJson;
                serializeJson(errorDoc, errorJson);
                request->send(400, "application/json", errorJson);
            }
        } else {
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "Missing ssid or password parameter";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(400, "application/json", errorJson);
        }
    } else if (request->method() == HTTP_GET) {
        // Return current WiFi SSID (not password for security)
        JsonDocument doc;
        doc["status"] = "ok";
        doc["ssid"] = wifiSSID;
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    } else {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Method not allowed";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(405, "application/json", errorJson);
    }
}

void HTTPServerController::handleMQTTConfig(AsyncWebServerRequest *request) {
    if (!settingsController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Settings controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    if (request->method() == HTTP_GET) {
        SettingsController::MQTTConfig config;
        settingsController->loadMQTTConfig(&config, nullptr);
        JsonDocument doc;
        doc["status"] = "ok";
        doc["enabled"] = config.enabled;
        doc["broker"] = config.broker;
        doc["port"] = config.port;
        doc["username"] = config.username;
        doc["device_id"] = config.deviceId;
        doc["base_topic"] = config.baseTopic;
        doc["qos_commands"] = config.qosCommands;
        doc["qos_status"] = config.qosStatus;
        doc["keepalive"] = config.keepalive;
        doc["clean_session"] = config.cleanSession;
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        return;
    }
    if (request->method() == HTTP_POST) {
        String broker = request->hasParam("broker", true) ? request->getParam("broker", true)->value() : "";
        String portStr = request->hasParam("port", true) ? request->getParam("port", true)->value() : "1883";
        String username = request->hasParam("username", true) ? request->getParam("username", true)->value() : "";
        String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
        String deviceId = request->hasParam("device_id", true) ? request->getParam("device_id", true)->value() : "";
        String baseTopic = request->hasParam("base_topic", true) ? request->getParam("base_topic", true)->value() : "";
        String qosCmdStr = request->hasParam("qos_commands", true) ? request->getParam("qos_commands", true)->value() : "1";
        String qosStatusStr = request->hasParam("qos_status", true) ? request->getParam("qos_status", true)->value() : "0";
        String keepaliveStr = request->hasParam("keepalive", true) ? request->getParam("keepalive", true)->value() : "60";
        bool enabled = request->hasParam("enabled", true) && (request->getParam("enabled", true)->value() == "1" || request->getParam("enabled", true)->value() == "true");
        bool cleanSession = request->hasParam("clean_session", true) && (request->getParam("clean_session", true)->value() == "1" || request->getParam("clean_session", true)->value() == "true");
        int port = portStr.toInt();
        int qosCmd = qosCmdStr.toInt();
        int qosStatus = qosStatusStr.toInt();
        int keepalive = keepaliveStr.toInt();
        if (enabled && broker.length() == 0) {
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "Broker is required when MQTT is enabled";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(400, "application/json", errorJson);
            return;
        }
        if (port < 1 || port > 65535) {
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "Port must be 1-65535";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(400, "application/json", errorJson);
            return;
        }
        if (broker.length() > 127 || username.length() > 63 || password.length() > 63 || deviceId.length() > 31 || baseTopic.length() > 63) {
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "One or more fields exceed maximum length";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(400, "application/json", errorJson);
            return;
        }
        SettingsController::MQTTConfig config;
        settingsController->loadMQTTConfig(&config, nullptr);
        config.enabled = enabled;
        strncpy(config.broker, broker.c_str(), sizeof(config.broker) - 1);
        config.broker[sizeof(config.broker) - 1] = '\0';
        config.port = port;
        strncpy(config.username, username.c_str(), sizeof(config.username) - 1);
        config.username[sizeof(config.username) - 1] = '\0';
        if (password.length() > 0) {
            strncpy(config.password, password.c_str(), sizeof(config.password) - 1);
            config.password[sizeof(config.password) - 1] = '\0';
        }
        strncpy(config.deviceId, deviceId.c_str(), sizeof(config.deviceId) - 1);
        config.deviceId[sizeof(config.deviceId) - 1] = '\0';
        strncpy(config.baseTopic, baseTopic.c_str(), sizeof(config.baseTopic) - 1);
        config.baseTopic[sizeof(config.baseTopic) - 1] = '\0';
        config.qosCommands = (qosCmd >= 0 && qosCmd <= 2) ? qosCmd : 1;
        config.qosStatus = (qosStatus >= 0 && qosStatus <= 2) ? qosStatus : 0;
        config.keepalive = (keepalive >= 1 && keepalive <= 65535) ? keepalive : 60;
        config.cleanSession = cleanSession;
        settingsController->saveMQTTConfig(&config);
        JsonDocument doc;
        doc["status"] = "ok";
        doc["message"] = "MQTT config saved. Reboot to apply.";
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        return;
    }
    JsonDocument errorDoc;
    errorDoc["status"] = "error";
    errorDoc["message"] = "Method not allowed";
    String errorJson;
    serializeJson(errorDoc, errorJson);
    request->send(405, "application/json", errorJson);
}

void HTTPServerController::handleConfigReload(AsyncWebServerRequest *request) {
    if (!settingsController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Settings controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    if (!audioController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Audio controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    if (request->method() == HTTP_POST) {
        SemaphoreHandle_t sdMutex = audioController->getSDMutex();
        bool reloaded = settingsController->loadFromConfigFile(sdMutex);
        
        if (reloaded) {
            // Reload WiFi credentials into buffers
            char newSSID[64];
            char newPassword[64];
            settingsController->loadWiFiSSID(newSSID, sizeof(newSSID), "");
            settingsController->loadWiFiPassword(newPassword, sizeof(newPassword), "");
            
            JsonDocument doc;
            doc["status"] = "ok";
            if (strlen(newSSID) > 0) {
                strncpy(wifiSSID, newSSID, 63);
                wifiSSID[63] = '\0';
                strncpy(wifiPassword, newPassword, 63);
                wifiPassword[63] = '\0';
                doc["message"] = "Config reloaded from SD card. Reboot required for WiFi changes.";
            } else {
                doc["message"] = "Config reloaded from SD card (no WiFi settings found)";
            }
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        } else {
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "config.json not found or invalid";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(404, "application/json", errorJson);
        }
    } else {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Method not allowed";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(405, "application/json", errorJson);
    }
}

void HTTPServerController::handleNotFound(AsyncWebServerRequest *request) {
    JsonDocument errorDoc;
    errorDoc["status"] = "error";
    errorDoc["message"] = "Not found";
    String errorJson;
    serializeJson(errorDoc, errorJson);
    request->send(404, "application/json", errorJson);
}

// Static wrapper functions
void HTTPServerController::handleRootWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleRoot(request);
}

void HTTPServerController::handlePlayWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handlePlay(request);
}

void HTTPServerController::handleStopWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleStop(request);
}

void HTTPServerController::handleVolumeWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleVolume(request);
}

void HTTPServerController::handleStatusWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleStatus(request);
}

void HTTPServerController::handleDirWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleDir(request);
}

void HTTPServerController::handleLEDEffectWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleLEDEffect(request);
}

void HTTPServerController::handleLEDBrightnessWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleLEDBrightness(request);
}

void HTTPServerController::handleLEDStopWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleLEDStop(request);
}

void HTTPServerController::handleLEDEffectsListWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleLEDEffectsList(request);
}

void HTTPServerController::handleDemoStartWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleDemoStart(request);
}

void HTTPServerController::handleDemoStopWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleDemoStop(request);
}

void HTTPServerController::handleDemoPauseWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleDemoPause(request);
}

void HTTPServerController::handleDemoResumeWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleDemoResume(request);
}

void HTTPServerController::handleDemoStatusWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleDemoStatus(request);
}

void HTTPServerController::handleSettingsClearWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleSettingsClear(request);
}

void HTTPServerController::handleWiFiConfigWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleWiFiConfig(request);
}

void HTTPServerController::handleMQTTConfigWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleMQTTConfig(request);
}

void HTTPServerController::handleConfigReloadWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleConfigReload(request);
}

void HTTPServerController::handleEffectsList(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Effects list request received");

    if (!settingsController) {
        ESP_LOGE(TAG, "Settings controller not available");
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Settings controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }

    int effectCount = settingsController->getEffectCount();
    ESP_LOGI(TAG, "Total effects configured: %d", effectCount);
    
    // Build JSON response using ArduinoJson
    JsonDocument doc;
    doc["status"] = "ok";
    JsonArray effects = doc["effects"].to<JsonArray>();
    
    for (int i = 0; i < effectCount; i++) {
        SettingsController::Effect effect;
        if (settingsController->getEffect(i, &effect)) {
            ESP_LOGI(TAG, "Effect %d: %s (audio: %s, LED: %s)", i, effect.name, effect.hasAudio ? "yes" : "no", effect.hasLED ? "yes" : "no");
            
            JsonObject effectObj = effects.add<JsonObject>();
            effectObj["name"] = effect.name;
            effectObj["hasAudio"] = effect.hasAudio;
            effectObj["hasLED"] = effect.hasLED;
            effectObj["loop"] = effect.loop;
            effectObj["av_sync"] = effect.av_sync;
            if (effect.hasAudio) {
                effectObj["audio"] = effect.audioFile;
            }
            if (effect.hasLED) {
                effectObj["led"] = effect.ledEffectName;
            }
        }
    }
    
    doc["currentLoop"] = currentLoopingEffect;
    
    String json;
    serializeJson(doc, json);
    ESP_LOGD(TAG, "Sending effects list (%d effects, current loop: %s)", effectCount, currentLoopingEffect.length() > 0 ? currentLoopingEffect.c_str() : "none");
    request->send(200, "application/json", json);
}

void HTTPServerController::handleEffectExecute(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Effect execute request received");

    if (!settingsController) {
        ESP_LOGE(TAG, "Settings controller not available");
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Settings controller not available";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    // Read effect name from POST body (REST best practice)
    if (!request->hasParam("name", true)) {
        ESP_LOGE(TAG, "Missing name parameter in request body");
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Missing name parameter in request body";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    String effectName = request->getParam("name", true)->value();
    ESP_LOGI(TAG, "Got name from POST body: %s", effectName.c_str());

    effectName.trim();
    ESP_LOGI(TAG, "Trimmed effect name: '%s'", effectName.c_str());
    
    if (effectName.length() == 0) {
        ESP_LOGE(TAG, "Effect name is empty after trimming");
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Invalid effect name";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    ESP_LOGI(TAG, "Looking up effect: %s", effectName.c_str());
    
    SettingsController::Effect effect;
    if (!settingsController->getEffectByName(effectName.c_str(), &effect)) {
        ESP_LOGE(TAG, "Effect not found: %s", effectName.c_str());
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Effect not found";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(404, "application/json", errorJson);
        return;
    }

    ESP_LOGI(TAG, "Effect found: %s (hasAudio: %s, hasLED: %s, loop: %s, av_sync: %s)", effect.name, effect.hasAudio ? "yes" : "no", effect.hasLED ? "yes" : "no", effect.loop ? "yes" : "no", effect.av_sync ? "yes" : "no");
    
    // Stop any currently looping effect
    if (isLoopingAudio || isLoopingLED) {
        ESP_LOGI(TAG, "Stopping current loop before starting new effect");
        stopEffectLoop();
    }

    // Execute the effect
    bool audioPlayed = false;
    bool ledStarted = false;

    if (effect.hasAudio && audioController) {
        ESP_LOGI(TAG, "Playing audio file: %s", effect.audioFile);
        
        // Check if file exists before attempting to play
        bool fileExists = audioController->fileExists(effect.audioFile);
        if (!fileExists) {
            ESP_LOGE(TAG, "Audio file not found: %s", effect.audioFile);
            audioPlaybackFailed = true;
        } else if (audioController->playFile(effect.audioFile)) {
            audioPlayed = true;
            audioPlaybackFailed = false;  // Reset failure flag on success
            audioLoopStartTime = 0;  // Reset timeout
            ESP_LOGI(TAG, "Audio file queued successfully");
        } else {
            ESP_LOGE(TAG, "Failed to queue audio file");
            audioPlaybackFailed = true;
            audioLoopStartTime = 0;
        }
    } else if (effect.hasAudio) {
        ESP_LOGW(TAG, "Effect has audio but audioController is null");
        audioPlaybackFailed = true;
    }
    
    if (effect.hasLED) {
        if (dispatchLEDEffect(settingsController, effect.ledEffectName, ledStripController, ledMatrixController)) {
            ESP_LOGI(TAG, "Starting LED effect: %s", effect.ledEffectName);
            ledStarted = true;
            ESP_LOGI(TAG, "LED effect started");
        } else {
            ESP_LOGW(TAG, "Effect has LED but appropriate controller is not available or effect name unknown");
        }
    }
    
    // Start looping if requested and playback was successful
    if (effect.loop) {
        if (effect.hasAudio && audioPlaybackFailed) {
            ESP_LOGW(TAG, "Loop requested but audio playback failed, loop not started");
        } else {
            ESP_LOGI(TAG, "Starting effect loop");
            startEffectLoop(effect);
        }
    }
    
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Effect executed";
    doc["audioPlayed"] = audioPlayed;
    doc["ledStarted"] = ledStarted;
    doc["looping"] = effect.loop;
    doc["av_sync"] = effect.av_sync;
    
    String json;
    serializeJson(doc, json);
    ESP_LOGD(TAG, "Sending response: %s", json.c_str());
    request->send(200, "application/json", json);
}

void HTTPServerController::startEffectLoop(const SettingsController::Effect& effect) {
    ESP_LOGI(TAG, "startEffectLoop() called for effect: %s", effect.name);
    
    currentLoopingEffect = effect.name;
    loopingAvSync = effect.av_sync;  // Store av_sync flag for loop
    audioPlaybackFailed = false;  // Reset failure flag when starting new loop
    
    if (effect.hasAudio) {
        // Only enable audio looping if playback was successful
        if (!audioPlaybackFailed) {
            isLoopingAudio = true;
            loopingAudioFile = effect.audioFile;
            ESP_LOGI(TAG, "Audio looping enabled for: %s", loopingAudioFile.c_str());
        } else {
            ESP_LOGW(TAG, "Audio looping disabled due to playback failure");
            isLoopingAudio = false;
        }
    }
    
    if (effect.hasLED) {
        isLoopingLED = true;
        strncpy(loopingLEDEffectName, effect.ledEffectName, sizeof(loopingLEDEffectName) - 1);
        loopingLEDEffectName[sizeof(loopingLEDEffectName) - 1] = '\0';
        ESP_LOGI(TAG, "LED looping enabled for effect: %s", loopingLEDEffectName);
        loopingAvSync = effect.av_sync;
    }
}

void HTTPServerController::stopEffectLoop() {
    ESP_LOGI(TAG, "stopEffectLoop() called, current loop: %s", currentLoopingEffect.c_str());

    if (isLoopingAudio && audioController) {
        ESP_LOGI(TAG, "Stopping audio loop");
        audioController->stop();
    }
    
    if (isLoopingLED) {
        ESP_LOGI(TAG, "Stopping LED loop");
        // Stop both controllers (effect could be on either)
        if (ledStripController) {
            ledStripController->stopEffect();
        }
        if (ledMatrixController) {
            ledMatrixController->stopEffect();
        }
    }
    
    currentLoopingEffect = "";
    isLoopingAudio = false;
    isLoopingLED = false;
    loopingAudioFile = "";
    loopingLEDEffectName[0] = '\0';
    loopingAvSync = false;  // Reset av_sync flag
    audioPlaybackFailed = false;  // Reset failure flag when stopping loop
    audioLoopStartTime = 0;  // Reset timeout when stopping loop
    
    ESP_LOGI(TAG, "Loop stopped");
}

bool HTTPServerController::executeEffectByName(const char* effectName) {
    ESP_LOGI(TAG, "Startup: Attempting to execute effect: %s", effectName);

    if (!settingsController) {
        ESP_LOGE(TAG, "Startup: Settings controller not available");
        return false;
    }

    SettingsController::Effect effect;
    if (!settingsController->getEffectByName(effectName, &effect)) {
        ESP_LOGE(TAG, "Startup: Effect not found: %s", effectName);
        return false;
    }

    ESP_LOGI(TAG, "Startup: Effect found: %s (hasAudio: %s, hasLED: %s, loop: %s)", effect.name, effect.hasAudio ? "yes" : "no", effect.hasLED ? "yes" : "no", effect.loop ? "yes" : "no");
    
    // Stop any currently looping effect
    if (isLoopingAudio || isLoopingLED) {
        ESP_LOGI(TAG, "Startup: Stopping current loop before starting startup effect");
        stopEffectLoop();
    }
    
    // Execute the effect
    bool audioPlayed = false;
    bool ledStarted = false;
    
    if (effect.hasAudio && audioController) {
        ESP_LOGI(TAG, "Startup: Playing audio file: %s", effect.audioFile);

        // Check if file exists before attempting to play
        bool fileExists = audioController->fileExists(effect.audioFile);
        if (!fileExists) {
            ESP_LOGE(TAG, "Startup: Audio file not found: %s", effect.audioFile);
            audioPlaybackFailed = true;
        } else if (audioController->playFile(effect.audioFile)) {
            audioPlayed = true;
            audioPlaybackFailed = false;
            audioLoopStartTime = 0;
            ESP_LOGI(TAG, "Startup: Audio file queued successfully");
        } else {
            ESP_LOGE(TAG, "Startup: Failed to queue audio file");
            audioPlaybackFailed = true;
            audioLoopStartTime = 0;
        }
    }
    
    if (effect.hasLED) {
        if (dispatchLEDEffect(settingsController, effect.ledEffectName, ledStripController, ledMatrixController)) {
            ESP_LOGI(TAG, "Startup: Starting LED effect: %s", effect.ledEffectName);
        }
    }
    
    // Start looping if requested and playback was successful
    if (effect.loop) {
        if (effect.hasAudio && audioPlaybackFailed) {
            ESP_LOGW(TAG, "Startup: Loop requested but audio playback failed, loop not started");
        } else {
            ESP_LOGI(TAG, "Startup: Starting effect loop");
            startEffectLoop(effect);
        }
    }
    
    ESP_LOGI(TAG, "Startup: Startup effect executed successfully");
    return true;
}

void HTTPServerController::handleEffectStopLoop(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Stop loop request received");

    JsonDocument doc;
    doc["status"] = "ok";

    if (!isLoopingAudio && !isLoopingLED) {
        ESP_LOGW(TAG, "No loop currently active");
        doc["message"] = "No loop active";
    } else {
        stopEffectLoop();
        doc["message"] = "Loop stopped";
    }
    
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

// ============================================================================
// ============================================================================
// Bass Mono DSP API Implementation
// ============================================================================

void HTTPServerController::handleBassMonoGet(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["status"] = "ok";
    if (bassMonoProcessor) {
        doc["enabled"] = bassMonoProcessor->isEnabled();
        doc["crossover_hz"] = bassMonoProcessor->getCrossoverHz();
        doc["sample_rate"] = bassMonoProcessor->getSampleRate();
    } else {
        doc["enabled"] = false;
        doc["crossover_hz"] = 80;
        doc["sample_rate"] = 0;
    }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void HTTPServerController::handleBassMonoPost(AsyncWebServerRequest *request) {
    bool changed = false;

    if (request->hasParam("enabled", true)) {
        String val = request->getParam("enabled", true)->value();
        bool enabled = (val == "true" || val == "1");
        if (bassMonoProcessor) {
            bassMonoProcessor->setEnabled(enabled);
        }
        if (settingsController) {
            settingsController->saveBassMonoEnabled(enabled);
        }
        changed = true;
    }

    if (request->hasParam("crossover_hz", true)) {
        int hz = request->getParam("crossover_hz", true)->value().toInt();
        if (hz >= 20 && hz <= 500) {
            if (bassMonoProcessor) {
                bassMonoProcessor->setCrossoverHz((uint16_t)hz);
            }
            if (settingsController) {
                settingsController->saveBassMonoCrossover((uint16_t)hz);
            }
            changed = true;
        } else {
            JsonDocument errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "crossover_hz must be 20-500";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            request->send(400, "application/json", errorJson);
            return;
        }
    }

    if (!changed) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Missing enabled or crossover_hz parameter";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }

    // Return current state
    handleBassMonoGet(request);
}

// Battle Arena API Implementation
// ============================================================================

void HTTPServerController::handleBattleTrigger(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Battle: Trigger request received");
    
    if (!audioController || (!ledStripController && !ledMatrixController) || !settingsController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Controller not available";
        errorDoc["command"] = "trigger";
        errorDoc["executed"] = false;
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    // For AsyncWebServer, we need to handle body via onBody callback
    // For now, accept parameters from query string or form data as fallback
    String body = "";
    
    // Try to get JSON from query parameter (for testing) or form data
    if (request->hasParam("body", true)) {
        body = request->getParam("body", true)->value();
    } else if (request->hasParam("plain", true)) {
        body = request->getParam("plain", true)->value();
    } else {
        // Build JSON from individual parameters (fallback for simple requests) using ArduinoJson
        JsonDocument bodyDoc;
        if (request->hasParam("command", true)) {
            bodyDoc["command"] = request->getParam("command", true)->value();
        } else {
            bodyDoc["command"] = "trigger";
        }
        if (request->hasParam("category", true)) {
            bodyDoc["category"] = request->getParam("category", true)->value();
        }
        if (request->hasParam("effect", true)) {
            bodyDoc["effect"] = request->getParam("effect", true)->value();
        }
        if (request->hasParam("priority", true)) {
            bodyDoc["priority"] = request->getParam("priority", true)->value();
        }
        if (request->hasParam("duration", true)) {
            bodyDoc["duration"] = request->getParam("duration", true)->value().toInt();
        }
        if (request->hasParam("audio_volume", true)) {
            bodyDoc["audio_volume"] = request->getParam("audio_volume", true)->value().toInt();
        }
        if (request->hasParam("led_brightness", true)) {
            bodyDoc["led_brightness"] = request->getParam("led_brightness", true)->value().toInt();
        }
        serializeJson(bodyDoc, body);
    }
    
    if (body.length() == 0) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Empty request body. Send JSON via POST body or use query parameters.";
        errorDoc["command"] = "trigger";
        errorDoc["executed"] = false;
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    ESP_LOGI(TAG, "Battle: Request body: %s", body.c_str());
    
    // Parse JSON (using JsonDocument - ArduinoJson v7 API)
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        ESP_LOGE(TAG, "Battle: JSON parse error: %s", error.c_str());
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Invalid JSON";
        errorDoc["command"] = "trigger";
        errorDoc["executed"] = false;
        errorDoc["error"] = error.c_str();
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    // Extract command parameters
    String command = doc["command"].as<String>();
    String category = doc["category"].as<String>();
    String effectName = doc["effect"].as<String>();
    String priority = doc["priority"].as<String>();
    int duration = doc["duration"].as<int>();
    int audioVolume = doc["audio_volume"].as<int>();
    int ledBrightness = doc["led_brightness"].as<int>();
    
    category.toLowerCase();
    effectName.toLowerCase();
    priority.toLowerCase();
    
    ESP_LOGI(TAG, "Battle: Command: %s, Category: %s, Effect: %s, Priority: %s", command.c_str(), category.c_str(), effectName.c_str(), priority.c_str());
    
    // Validate command
    if (command != "trigger") {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Invalid command";
        errorDoc["command"] = command;
        errorDoc["executed"] = false;
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    // Find effect by category or name
    SettingsController::Effect effect;
    bool effectFound = false;
    
    if (effectName.length() > 0) {
        // Specific effect requested
        effectFound = settingsController->getEffectByName(effectName.c_str(), &effect);
        if (!effectFound) {
            ESP_LOGE(TAG, "Battle: Effect not found: %s", effectName.c_str());
        }
    } else if (category.length() > 0) {
        // Find random effect in category
        SettingsController::Effect categoryEffects[32];
        int count = settingsController->getEffectsByCategory(category.c_str(), categoryEffects, 32);
        
        if (count > 0) {
            // Select random effect from category
            int index = random(count);
            effect = categoryEffects[index];
            effectFound = true;
            ESP_LOGI(TAG, "Battle: Selected effect from category: %s (%d available)", effect.name, count);
        } else {
            ESP_LOGW(TAG, "Battle: No effects found in category: %s", category.c_str());
        }
    } else {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Must specify category or effect";
        errorDoc["command"] = "trigger";
        errorDoc["executed"] = false;
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(400, "application/json", errorJson);
        return;
    }
    
    if (!effectFound) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Effect not found";
        errorDoc["command"] = "trigger";
        errorDoc["category"] = category;
        errorDoc["effect"] = effectName;
        errorDoc["executed"] = false;
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(404, "application/json", errorJson);
        return;
    }
    
    // Apply optional parameters
    bool audioPlayed = false;
    bool ledStarted = false;
    
    if (audioVolume >= 0 && audioVolume <= 21) {
        audioController->setVolume(audioVolume);
        settingsController->saveVolume(audioVolume);
    }
    
    if (ledBrightness >= 0 && ledBrightness <= 255) {
        // Set brightness on both controllers (shared brightness)
        if (ledStripController) {
            ledStripController->setBrightness((uint8_t)ledBrightness);
        }
        if (ledMatrixController) {
            ledMatrixController->setBrightness((uint8_t)ledBrightness);
        }
        settingsController->saveBrightness((uint8_t)ledBrightness);
    }
    
    // Stop any existing loops
    if (isLoopingAudio || isLoopingLED) {
        stopEffectLoop();
    }
    
    // Execute effect
    if (effect.hasAudio) {
        if (audioController->playFile(effect.audioFile)) {
            audioPlayed = true;
            ESP_LOGI(TAG, "Battle: Audio played: %s", effect.audioFile);
        } else {
            ESP_LOGE(TAG, "Battle: Failed to play audio: %s", effect.audioFile);
        }
    }
    
    if (effect.hasLED) {
        if (dispatchLEDEffect(settingsController, effect.ledEffectName, ledStripController, ledMatrixController)) {
            ledStarted = true;
            ESP_LOGI(TAG, "Battle: LED effect started: %s", effect.ledEffectName);
        }
    }
    
    // Build response using ArduinoJson
    JsonDocument responseDoc;
    responseDoc["status"] = "success";
    responseDoc["command"] = "trigger";
    responseDoc["effect"] = effect.name;
    responseDoc["category"] = effect.category;
    responseDoc["executed"] = true;
    responseDoc["audio_played"] = audioPlayed;
    responseDoc["led_started"] = ledStarted;
    responseDoc["message"] = "Effect triggered successfully";
    responseDoc["timestamp"] = millis();
    responseDoc["error"] = nullptr;
    
    String response;
    serializeJson(responseDoc, response);
    ESP_LOGD(TAG, "Battle: Response: %s", response.c_str());
    
    request->send(200, "application/json", response);
}

void HTTPServerController::handleBattleEffects(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Battle: Effects list request received");
    
    if (!settingsController) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "Settings controller not available";
        errorDoc["effects"].to<JsonArray>();
        String errorJson;
        serializeJson(errorDoc, errorJson);
        request->send(503, "application/json", errorJson);
        return;
    }
    
    // Check for category filter
    String category = "";
    if (request->hasParam("category")) {
        category = request->getParam("category")->value();
        category.toLowerCase();
    }
    
    // Build JSON response using ArduinoJson
    JsonDocument doc;
    doc["status"] = "success";
    JsonArray effects = doc["effects"].to<JsonArray>();
    int count = 0;
    
    if (category.length() > 0) {
        // Filter by category
        SettingsController::Effect categoryEffects[32];
        int effectCount = settingsController->getEffectsByCategory(category.c_str(), categoryEffects, 32);
        
        for (int i = 0; i < effectCount; i++) {
            JsonObject effectObj = effects.add<JsonObject>();
            effectObj["name"] = categoryEffects[i].name;
            effectObj["category"] = categoryEffects[i].category;
            effectObj["has_audio"] = categoryEffects[i].hasAudio;
            effectObj["has_led"] = categoryEffects[i].hasLED;
            effectObj["audio_file"] = categoryEffects[i].audioFile;
            effectObj["led_effect"] = categoryEffects[i].ledEffectName;
            effectObj["loop"] = categoryEffects[i].loop;
            effectObj["av_sync"] = categoryEffects[i].av_sync;
            count++;
        }
    } else {
        // Return all effects
        int effectCount = settingsController->getEffectCount();
        for (int i = 0; i < effectCount; i++) {
            SettingsController::Effect effect;
            if (settingsController->getEffect(i, &effect)) {
                JsonObject effectObj = effects.add<JsonObject>();
                effectObj["name"] = effect.name;
                effectObj["category"] = effect.category;
                effectObj["has_audio"] = effect.hasAudio;
                effectObj["has_led"] = effect.hasLED;
                effectObj["audio_file"] = effect.audioFile;
                effectObj["led_effect"] = effect.ledEffectName;
                effectObj["loop"] = effect.loop;
                effectObj["av_sync"] = effect.av_sync;
                count++;
            }
        }
    }
    
    doc["count"] = count;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void HTTPServerController::handleBattleStatus(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Battle: Status request received");
    
    // Build JSON response using ArduinoJson
    JsonDocument doc;
    doc["status"] = "success";
    
    JsonObject audio = doc["audio"].to<JsonObject>();
    audio["playing"] = audioController && audioController->isPlaying();
    audio["volume"] = audioController ? audioController->getVolume() : 0;
    
    JsonObject led = doc["led"].to<JsonObject>();
    // Report status from strip controller (or matrix if strip not available)
    bool stripRunning = ledStripController && ledStripController->isEffectRunning();
    bool matrixRunning = ledMatrixController && ledMatrixController->isEffectRunning();
    led["effect_running"] = stripRunning || matrixRunning;
    led["current_effect"] = ledStripController ? (int)ledStripController->getCurrentEffect() : (ledMatrixController ? (int)ledMatrixController->getCurrentEffect() : -1);
    led["brightness"] = ledStripController ? ledStripController->getBrightness() : (ledMatrixController ? ledMatrixController->getBrightness() : 0);
    
    JsonObject looping = doc["looping"].to<JsonObject>();
    looping["active"] = isLoopingAudio || isLoopingLED;
    looping["effect"] = currentLoopingEffect;
    
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["connected"] = isConnected();
    wifi["ip"] = getIPAddress();
    
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void HTTPServerController::handleBattleHealth(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Battle: Health check request received");
    
    bool healthy = true;
    String issues = "";
    
    if (!audioController) {
        healthy = false;
        issues += "audio_controller_missing;";
    }
    
    if (!ledStripController && !ledMatrixController) {
        healthy = false;
        issues += "led_controllers_missing;";
    }
    
    if (!settingsController) {
        healthy = false;
        issues += "settings_controller_missing;";
    }
    
    if (!isConnected()) {
        healthy = false;
        issues += "wifi_disconnected;";
    }
    
    // Build JSON response using ArduinoJson
    JsonDocument doc;
    doc["status"] = healthy ? "healthy" : "unhealthy";
    doc["healthy"] = healthy;
    doc["issues"] = issues;
    doc["uptime"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    
    int statusCode = healthy ? 200 : 503;
    request->send(statusCode, "application/json", response);
}

void HTTPServerController::handleBattleStop(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Battle: Stop request received");
    
    // Stop all audio and LED effects
    if (audioController) {
        audioController->stop();
    }
    
    // Stop both controllers
    if (ledStripController) {
        ledStripController->stopEffect();
    }
    if (ledMatrixController) {
        ledMatrixController->stopEffect();
    }
    
    // Stop any loops
    if (isLoopingAudio || isLoopingLED) {
        stopEffectLoop();
    }
    
    // Build JSON response using ArduinoJson
    JsonDocument doc;
    doc["status"] = "success";
    doc["command"] = "stop";
    doc["message"] = "All effects stopped";
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void HTTPServerController::handleEffectsListWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleEffectsList(request);
}

void HTTPServerController::handleEffectExecuteWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleEffectExecute(request);
}

void HTTPServerController::handleEffectStopLoopWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleEffectStopLoop(request);
}

// Bass Mono DSP wrapper functions
void HTTPServerController::handleBassMonoGetWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleBassMonoGet(request);
}

void HTTPServerController::handleBassMonoPostWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleBassMonoPost(request);
}

// Battle Arena API wrapper functions
void HTTPServerController::handleBattleTriggerWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleBattleTrigger(request);
}

void HTTPServerController::handleBattleEffectsWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleBattleEffects(request);
}

void HTTPServerController::handleBattleStatusWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleBattleStatus(request);
}

void HTTPServerController::handleBattleHealthWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleBattleHealth(request);
}

void HTTPServerController::handleBattleStopWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleBattleStop(request);
}

void HTTPServerController::handleNotFoundWrapper(AsyncWebServerRequest *request) {
    if (instance) instance->handleNotFound(request);
}

void HTTPServerController::setupRoutes() {
    // Audio routes
    server->on("/", HTTP_GET, handleRootWrapper);
    server->on("/play", HTTP_POST, handlePlayWrapper);
    server->on("/stop", HTTP_POST, handleStopWrapper);
    server->on("/volume", HTTP_POST, handleVolumeWrapper);
    server->on("/status", HTTP_GET, handleStatusWrapper);
    server->on("/dir", HTTP_GET, handleDirWrapper);
    
    // LED routes
    server->on("/led/effect", HTTP_POST, handleLEDEffectWrapper);
    server->on("/led/brightness", HTTP_POST, handleLEDBrightnessWrapper);
    server->on("/led/stop", HTTP_POST, handleLEDStopWrapper);
    server->on("/led/effects/list", HTTP_GET, handleLEDEffectsListWrapper);
    
    // Demo mode routes
    server->on("/demo/start", HTTP_POST, handleDemoStartWrapper);
    server->on("/demo/stop", HTTP_POST, handleDemoStopWrapper);
    server->on("/demo/pause", HTTP_POST, handleDemoPauseWrapper);
    server->on("/demo/resume", HTTP_POST, handleDemoResumeWrapper);
    server->on("/demo/status", HTTP_GET, handleDemoStatusWrapper);
    
    // Settings routes
    server->on("/settings/clear", HTTP_POST, handleSettingsClearWrapper);
    server->on("/settings/mqtt", HTTP_GET, handleMQTTConfigWrapper);
    server->on("/settings/mqtt", HTTP_POST, handleMQTTConfigWrapper);
    
    // Effects routes
    server->on("/effects/list", HTTP_GET, handleEffectsListWrapper);
    server->on("/effects/execute", HTTP_POST, handleEffectExecuteWrapper);
    server->on("/effects/stop-loop", HTTP_POST, handleEffectStopLoopWrapper);
    
    // Bass Mono DSP routes
    server->on("/audio/bass-mono", HTTP_GET, handleBassMonoGetWrapper);
    server->on("/audio/bass-mono", HTTP_POST, handleBassMonoPostWrapper);

    // Battle Arena API endpoints
    server->on("/api/battle/trigger", HTTP_POST, handleBattleTriggerWrapper);
    server->on("/api/battle/effects", HTTP_GET, handleBattleEffectsWrapper);
    server->on("/api/battle/status", HTTP_GET, handleBattleStatusWrapper);
    server->on("/api/battle/health", HTTP_GET, handleBattleHealthWrapper);
    server->on("/api/battle/stop", HTTP_POST, handleBattleStopWrapper);
    
    // Note: For proper JSON body parsing with AsyncWebServer, you may need to use
    // onBody callback. For now, the trigger endpoint accepts query parameters as fallback.
    
    server->onNotFound(handleNotFoundWrapper);
}

bool HTTPServerController::begin(AudioController* audioCtrl, StripLEDController* ledStripCtrl, DemoController* demoCtrl, SettingsController* settingsCtrl, MatrixLEDController* ledMatrixCtrl) {
    audioController = audioCtrl;
    ledStripController = ledStripCtrl;
    ledMatrixController = ledMatrixCtrl;  // Can be nullptr if not used
    demoController = demoCtrl;
    settingsController = settingsCtrl;
    
    if (!initWiFi()) {
        return false;
    }
    
    // Create AsyncWebServer instance
    server = new AsyncWebServer(httpPort);
    
    // Setup routes
    setupRoutes();
    
    // Start server (async server handles requests automatically)
    server->begin();
    ESP_LOGI(TAG, "HTTP server started (AsyncWebServer)");
    
    return true;
}

void HTTPServerController::update() {
    // Check WiFi connection and reconnect if needed
    // Note: ESPAsyncWebServer handles requests automatically, no polling needed
    if (WiFi.status() != WL_CONNECTED) {
        static unsigned long lastReconnectAttempt = 0;
        unsigned long now = millis();
        
        // Try to reconnect every 5 seconds
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            ESP_LOGD(TAG, "WiFi disconnected, attempting to reconnect...");
            WiFi.disconnect();
            WiFi.begin(wifiSSID, wifiPassword);
        }
    }
}

bool HTTPServerController::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String HTTPServerController::getIPAddress() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return String("");
}

void HTTPServerController::updateLoops() {
    // Check audio loop
    if (isLoopingAudio && audioController && !audioPlaybackFailed) {
        // Check if audio has finished playing
        if (!audioController->isPlaying() && loopingAudioFile.length() > 0) {
            // If we just attempted to start audio, check if it started within timeout
            if (audioLoopStartTime > 0) {
                unsigned long now = millis();
                if (now - audioLoopStartTime > AUDIO_START_TIMEOUT) {
                    // Audio didn't start within timeout - likely an error
                    ESP_LOGE(TAG, "Audio failed to start within timeout, stopping loop");
                    audioPlaybackFailed = true;
                    isLoopingAudio = false;
                    audioLoopStartTime = 0;
                    if (!isLoopingLED) {
                        currentLoopingEffect = "";
                    }
                    return;
                }
                // Still waiting for audio to start
                return;
            }
            
            // Verify file still exists before restarting
            bool fileExists = audioController->fileExists(loopingAudioFile);
            if (fileExists) {
                ESP_LOGD(TAG, "Audio loop restarting: %s", loopingAudioFile.c_str());
                if (audioController->playFile(loopingAudioFile)) {
                    // Successfully queued for playback - set timeout to check if it starts
                    audioLoopStartTime = millis();
                    ESP_LOGD(TAG, "Audio loop restart queued successfully, waiting for playback to start...");
                } else {
                    // Failed to queue - stop looping
                    ESP_LOGE(TAG, "Failed to restart audio loop, stopping loop");
                    audioPlaybackFailed = true;
                    isLoopingAudio = false;
                    if (!isLoopingLED) {
                        // Only clear currentLoopingEffect if LED is also not looping
                        currentLoopingEffect = "";
                    }
                }
            } else {
                // File no longer exists - stop looping
                ESP_LOGE(TAG, "Audio file no longer exists, stopping loop: %s", loopingAudioFile.c_str());
                audioPlaybackFailed = true;
                isLoopingAudio = false;
                if (!isLoopingLED) {
                    // Only clear currentLoopingEffect if LED is also not looping
                    currentLoopingEffect = "";
                }
            }
        } else if (audioController->isPlaying() && audioLoopStartTime > 0) {
            // Audio started playing successfully - clear timeout
            ESP_LOGD(TAG, "Audio loop restart confirmed - playback started");
            audioLoopStartTime = 0;
        }
    } else if (isLoopingAudio && audioPlaybackFailed) {
        // Audio loop was active but playback failed - stop it
        ESP_LOGE(TAG, "Audio loop stopped due to playback failure");
        isLoopingAudio = false;
        audioLoopStartTime = 0;
        if (!isLoopingLED) {
            currentLoopingEffect = "";
        }
    }
    
    // Check LED loop
    if (isLoopingLED && loopingLEDEffectName[0] != '\0' && settingsController) {
        if (!isLEDEffectRunning(settingsController, loopingLEDEffectName, ledStripController, ledMatrixController)) {
            ESP_LOGD(TAG, "LED loop restarting effect: %s", loopingLEDEffectName);
            dispatchLEDEffect(settingsController, loopingLEDEffectName, ledStripController, ledMatrixController);
        }
    }
}

void HTTPServerController::printEndpoints() const {
    if (!isConnected()) {
        return;
    }
    
    String ipStr = getIPAddress();
    const char* ip = ipStr.c_str();

    ESP_LOGD(TAG, "HTTP API Endpoints:");
    ESP_LOGD(TAG, "  http://%s/ - Web interface", ip);

    ESP_LOGD(TAG, "Audio Endpoints:");
    ESP_LOGD(TAG, "  POST http://%s/play?file=/path/to/file.mp3", ip);
    ESP_LOGD(TAG, "  POST http://%s/stop", ip);
    ESP_LOGD(TAG, "  POST http://%s/volume?volume=21", ip);
    ESP_LOGD(TAG, "  GET http://%s/status", ip);
    ESP_LOGD(TAG, "  GET http://%s/dir?path=/", ip);

    ESP_LOGD(TAG, "LED Effect Endpoints:");
    ESP_LOGD(TAG, "  Themed Effects:");
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=atomic_breath", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=gravity_beam", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=fire_breath", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=electric", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=battle_damage", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=victory", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=idle", ip);
    ESP_LOGD(TAG, "  Popular Effects:");
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=rainbow", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=rainbow_chase", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=color_wipe", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=theater_chase", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=pulse", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=breathing", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=meteor", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=twinkle", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=water", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=strobe", ip);
    ESP_LOGD(TAG, "  Edge-Lit Disc Effects:");
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=radial_out", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=radial_in", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=spiral", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=rotating_rainbow", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=circular_chase", ip);
    ESP_LOGD(TAG, "    POST http://%s/led/effect?effect=radial_gradient", ip);
    ESP_LOGD(TAG, "  POST http://%s/led/brightness?brightness=128", ip);

    ESP_LOGD(TAG, "Battle Arena API Endpoints:");
    ESP_LOGD(TAG, "  POST http://%s/api/battle/trigger?command=trigger&category=attack", ip);
    ESP_LOGD(TAG, "  POST http://%s/api/battle/trigger?command=trigger&effect=godzilla_roar", ip);
    ESP_LOGD(TAG, "  GET http://%s/api/battle/effects", ip);
    ESP_LOGD(TAG, "  GET http://%s/api/battle/effects?category=attack", ip);
    ESP_LOGD(TAG, "  GET http://%s/api/battle/status", ip);
    ESP_LOGD(TAG, "  GET http://%s/api/battle/health", ip);
    ESP_LOGD(TAG, "  POST http://%s/api/battle/stop", ip);
    ESP_LOGD(TAG, "  POST http://%s/led/stop", ip);

    ESP_LOGD(TAG, "Demo Mode Endpoints:");
    ESP_LOGD(TAG, "  POST http://%s/demo/start?delay=5000", ip);
    ESP_LOGD(TAG, "  POST http://%s/demo/stop", ip);
    ESP_LOGD(TAG, "  POST http://%s/demo/pause", ip);
    ESP_LOGD(TAG, "  POST http://%s/demo/resume", ip);
    ESP_LOGD(TAG, "  GET http://%s/demo/status", ip);

    ESP_LOGD(TAG, "Settings Endpoints:");
    ESP_LOGD(TAG, "  POST http://%s/settings/clear", ip);
}

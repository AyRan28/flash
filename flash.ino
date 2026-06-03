//latest code
#include <WiFi.h>
#include <WebServer.h>

const char* wifi_ssid     = "iQOO 12";
const char* wifi_password = "ekseaanth";

WebServer server(80);

#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33
#define ENA 14
#define ENB 12
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define TRIG 5
#define ECHO 18

bool autoMode = false;
long duration;
float distance;
int speedValue = 210;
int speedLevel = 80;
String robotStatus = "STOP";
String currentMode = "MANUAL";

enum AutoState { AUTO_FORWARD, AUTO_STOPPING, AUTO_TURNING };
AutoState autoState = AUTO_FORWARD;
unsigned long autoTimer = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Smart Rover</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{text-align:center;font-family:Arial;background:#111;color:white;padding:10px}
h1{font-size:22px;margin:10px 0;color:cyan}
.card{background:#1a1a2e;margin:10px auto;padding:12px;border-radius:12px;max-width:400px}
.data{font-size:18px;margin:6px;color:cyan}
.arrow{font-size:50px;color:yellow;margin:5px}
button{height:50px;font-size:16px;margin:5px;border:none;border-radius:10px;color:white;cursor:pointer;padding:0 20px}
.bf{background:#27ae60;width:110px}
.bb{background:#e67e22;width:110px}
.bl{background:#2980b9;width:90px}
.br{background:#2980b9;width:90px}
.bs{background:#e74c3c;width:90px}
.ba{background:#8e44ad}
.bm{background:#7f8c8d}
.bv{background:#16a085}
.bmap{background:#d35400}
.speedBtn{width:80px;height:45px;font-size:16px;font-weight:bold;border:3px solid transparent;border-radius:10px;color:white;cursor:pointer;margin:4px}
.s50{background:#e67e22}
.s80{background:#2980b9}
.s100{background:#e74c3c}
.activeS{border-color:lime;box-shadow:0 0 12px lime}
.sBar{width:80%;height:20px;background:#333;margin:8px auto;border-radius:10px;overflow:hidden}
.sFill{height:100%;background:lime;transition:width 0.3s}
.radar{width:150px;height:150px;border:2px solid lime;border-radius:50%;margin:10px auto;position:relative}
.dot{width:14px;height:14px;background:red;border-radius:50%;position:absolute;left:50%;transform:translateX(-50%);transition:top 0.3s}
.warn{font-size:22px;color:red;font-weight:bold}
.safe{font-size:18px;color:lime}
.mic-btn{width:70px;height:70px;border-radius:50%;border:none;font-size:30px;cursor:pointer;background:#e74c3c;color:white;margin:10px}
.mic-btn.listening{background:#27ae60;animation:pulse 1s infinite}
@keyframes pulse{0%,100%{box-shadow:0 0 0 0 rgba(39,174,96,0.7)}50%{box-shadow:0 0 0 15px rgba(39,174,96,0)}}
.log{text-align:left;background:#0d0d0d;padding:10px;border-radius:8px;margin-top:10px;font-size:13px;max-height:200px;overflow-y:auto;font-family:monospace}
.log div{margin:3px 0;padding:3px;border-bottom:1px solid #222}
.log .cmd-active{color:lime}
.log .cmd-done{color:#666}
.log .cmd-error{color:red}
#transcript{color:#aaa;font-style:italic;margin:8px;min-height:20px}
#apiKeyBox input{width:70%;padding:8px;border-radius:6px;border:1px solid #444;background:#222;color:white;font-size:13px}
#apiKeyBox button{height:35px;font-size:13px;background:#8e44ad;padding:0 12px}
#voiceReady{display:none}
#textBox{margin:10px}
#textBox input{width:70%;padding:8px;border-radius:6px;border:1px solid #444;background:#222;color:white;font-size:14px}
#textBox button{height:38px;font-size:14px;background:#16a085;padding:0 15px}
#mapPanel{display:none}
#mapCanvas{touch-action:none}
.mapInfo{font-size:12px;color:#aaa;margin:6px}
.mapBtn{height:40px;font-size:14px;margin:5px;border:none;border-radius:8px;color:white;padding:0 15px;cursor:pointer}
</style>
</head>
<body>

<h1>ESP32 SMART ROVER</h1>

<div class="card">
<div class="data">Distance: <span id="dist">--</span> cm</div>
<div class="data">Mode: <span id="mode">--</span></div>
<div class="data">Status: <span id="stat">--</span></div>
</div>

<div class="card">
<div class="arrow" id="arrow">⏹</div>
</div>

<div class="card">
<b>Speed</b><br>
<button class="speedBtn s50" onclick="setSpd(50)">50%</button>
<button class="speedBtn s80" onclick="setSpd(80)">80%</button>
<button class="speedBtn s100" onclick="setSpd(100)">100%</button>
<div class="sBar"><div class="sFill" id="sFill" style="width:80%"></div></div>
</div>

<div class="card">
<b>Radar</b>
<div class="radar"><div class="dot" id="dot" style="top:50%"></div></div>
<div id="warnBox" class="safe">SAFE</div>
</div>

<div class="card">
<button class="bf" onclick="cmd('forward')">FORWARD</button><br>
<button class="bl" onclick="cmd('left')">LEFT</button>
<button class="bs" onclick="cmd('stop')">STOP</button>
<button class="br" onclick="cmd('right')">RIGHT</button><br>
<button class="bb" onclick="cmd('backward')">BACKWARD</button>
</div>

<div class="card">
<button class="ba" onclick="setMode('auto')">AUTO</button>
<button class="bm" onclick="setMode('manual')">MANUAL</button>
<button class="bv" onclick="setMode('voice')">VOICE</button>
<button class="bmap" onclick="setMode('map');showMap()">MAP</button>
<button style="background:#e91e63" onclick="setMode('music');showMusic()">🎵 MUSIC</button>
</div>

<div class="card" id="voicePanel">
<b>Voice Control</b>
<div id="apiKeyBox">
<br>
<input id="apiKey" type="password" placeholder="Paste OpenAI API Key">
<button onclick="saveKey()">Save</button>
</div>
<div id="voiceReady">
<button class="mic-btn" id="micBtn" onclick="startVoice()">🎤</button>
<div id="transcript">Tap mic or type below</div>
<div id="textBox">
<input id="textCmd" type="text" placeholder="e.g. forward 2m then left">
<button onclick="sendText()">Send</button>
</div>
<div class="log" id="log"></div>
</div>
</div>

<div class="card" id="mapPanel">
<b>Map Trajectory Mode</b>
<p class="mapInfo">Draw your path on the grid. Tap to start, drag to draw, lift to finish.</p>
<div style="margin:8px">
<label style="font-size:13px;color:#ccc">Scale: 1 grid square = </label>
<input id="gridScale" type="number" value="50" style="width:50px;padding:4px;background:#222;color:white;border:1px solid #444;border-radius:4px"> cm
</div>
<canvas id="mapCanvas" width="300" height="300" style="border:2px solid lime;border-radius:8px;background:#0a0a0a"></canvas><br>
<button class="mapBtn" style="background:#7f8c8d" onclick="clearMap()">CLEAR</button>
<button class="mapBtn" style="background:#27ae60" onclick="processPath()">EXECUTE PATH</button>
<div class="log" id="mapLog"></div>
</div>

<div class="card" id="musicPanel" style="display:none">
  <b> Chord Control Mode</b>
  <p style="font-size:12px;color:#aaa;margin:6px">Strum and HOLD each chord ~0.5 seconds</p>
  <div style="background:#1a1a1a;border-radius:10px;padding:12px;margin:8px;font-size:14px;line-height:2.4">
    <span style="color:lime">■ C major</span> → ⬆ FORWARD<br>
    <span style="color:#e74c3c">■ A minor</span> → ⬇ BACKWARD<br>
    <span style="color:#2980b9">■ F major</span> → ⬅ LEFT<br>
    <span style="color:#f39c12">■ G major</span> → ➡ RIGHT
  </div>
  <button class="mic-btn" id="musicBtn" onclick="toggleMusic()" style="background:#e91e63;font-size:24px">🎵</button>
  <div id="musicStatus" style="color:#aaa;margin:8px">Tap to start then strum and hold a chord</div>
  <div style="margin:10px">
    <div id="chordDisplay" style="font-size:64px;color:lime;font-weight:bold;min-height:80px">-</div>
    <div id="freqDisplay" style="font-size:12px;color:#555;margin:4px">waiting...</div>
    <div style="margin:8px 0 4px;font-size:12px;color:#888">Lock-in progress:</div>
    <div style="width:80%;height:14px;background:#333;border-radius:7px;margin:0 auto 10px">
      <div id="stableBar" style="height:100%;width:0%;border-radius:7px;transition:width 0.08s;background:lime"></div>
    </div>
    <div id="cmdDisplay" style="font-size:24px;color:cyan;font-weight:bold">WAITING</div>
  </div>
  <div class="log" id="musicLog" style="max-height:130px"></div>
</div>

<script>
var MS_PER_METER = 5000;
var MS_PER_90_TURN = 1500;
var apiKey = localStorage.getItem('oai_key') || '';
if(apiKey){ showVoiceReady(); }

setInterval(poll, 1000);

function poll(){
  fetch('/api/status').then(function(r){return r.json()}).then(function(d){
    document.getElementById('dist').textContent = d.distance.toFixed(1);
    document.getElementById('mode').textContent = d.mode;
    document.getElementById('stat').textContent = d.status;
    document.getElementById('arrow').textContent = d.arrow;
    document.getElementById('sFill').style.width = d.speed+'%';
    var dotTop = Math.max(10, Math.min(85, 10+(d.distance/100)*75));
    document.getElementById('dot').style.top = dotTop+'%';
    var wb = document.getElementById('warnBox');
    if(d.distance < 15){wb.className='warn';wb.textContent='OBSTACLE!';}
    else{wb.className='safe';wb.textContent='SAFE';}
    document.querySelectorAll('.speedBtn').forEach(function(b){b.classList.remove('activeS')});
    var sb = document.querySelector('.s'+d.speed);
    if(sb) sb.classList.add('activeS');
  }).catch(function(){});
}

function cmd(c){ fetch('/api/'+c).catch(function(){}); }
function setSpd(s){ fetch('/api/speed'+s).catch(function(){}); }
function setMode(m){ fetch('/api/'+m).catch(function(){}); }

function saveKey(){
  apiKey = document.getElementById('apiKey').value.trim();
  if(!apiKey){alert('Enter API key');return;}
  localStorage.setItem('oai_key', apiKey);
  showVoiceReady();
}

function showVoiceReady(){
  document.getElementById('apiKeyBox').style.display='none';
  document.getElementById('voiceReady').style.display='block';
}

function startVoice(){
  var SR = window.SpeechRecognition || window.webkitSpeechRecognition;
  if(!SR){ addLog('Speech not supported. Use text input.','cmd-error'); return; }
  var rec = new SR();
  rec.lang = 'en-US'; rec.interimResults = false;
  var btn = document.getElementById('micBtn');
  btn.classList.add('listening');
  document.getElementById('transcript').textContent = 'Listening...';
  rec.start();
  rec.onresult = function(e){
    var text = e.results[0][0].transcript;
    document.getElementById('transcript').textContent = '"'+text+'"';
    btn.classList.remove('listening');
    processWithAI(text);
  };
  rec.onerror = function(e){ addLog('Mic error: '+e.error,'cmd-error'); btn.classList.remove('listening'); };
  rec.onend = function(){ btn.classList.remove('listening'); };
}

function sendText(){
  var text = document.getElementById('textCmd').value.trim();
  if(!text){alert('Type a command first');return;}
  document.getElementById('transcript').textContent = '"'+text+'"';
  document.getElementById('textCmd').value = '';
  processWithAI(text);
}

function processWithAI(text){
  addLog('Sending to AI: "'+text+'"','cmd-active');
  fetch('https://api.openai.com/v1/chat/completions',{
    method:'POST',
    headers:{'Content-Type':'application/json','Authorization':'Bearer '+apiKey},
    body:JSON.stringify({
      model:'gpt-4o-mini', temperature:0,
      messages:[
        {role:'system',content:'You are a robot command parser. Convert voice commands to a JSON array of actions. Available actions: forward, backward, left, right. Each needs time_ms. Calibration: 1 meter = '+MS_PER_METER+'ms. 90-degree turn = '+MS_PER_90_TURN+'ms. Reply ONLY with valid JSON array. No markdown, no explanation.'},
        {role:'user',content:text}
      ]
    })
  }).then(function(r){return r.json()}).then(function(data){
    if(data.error){ addLog('API Error: '+data.error.message,'cmd-error'); return; }
    var content = data.choices[0].message.content.trim();
    addLog('AI says: '+content,'cmd-active');
    executeCommands(JSON.parse(content));
  }).catch(function(e){ addLog('Error: '+e.message,'cmd-error'); });
}

function executeCommands(cmds){
  cmd('voice');
  addLog('Running '+cmds.length+' commands...','cmd-active');
  var i = 0;
  function runNext(){
    if(i >= cmds.length){ addLog('ALL DONE!','cmd-active'); cmd('stop'); return; }
    var c = cmds[i];
    addLog('['+(i+1)+'/'+cmds.length+'] '+c.action.toUpperCase()+' for '+(c.time_ms/1000).toFixed(1)+'s','cmd-active');
    cmd(c.action);
    setTimeout(function(){ cmd('stop'); setTimeout(function(){ i++; runNext(); }, 200); }, c.time_ms);
  }
  runNext();
}

function addLog(msg, cls){
  var d = document.getElementById('log');
  var el = document.createElement('div');
  el.className = cls||''; el.textContent = '> '+msg;
  d.appendChild(el); d.scrollTop = d.scrollHeight;
}

// ====== MAP MODE ======
var mapDrawing=false, pathPoints=[], mapCtx=null;

function showMap(){
  document.getElementById('mapPanel').style.display='block';
  mapCtx = document.getElementById('mapCanvas').getContext('2d');
  drawGrid();
}

function drawGrid(){
  mapCtx.clearRect(0,0,300,300);
  mapCtx.strokeStyle='#1a3a1a'; mapCtx.lineWidth=0.5;
  for(var i=0;i<=300;i+=30){
    mapCtx.beginPath();mapCtx.moveTo(i,0);mapCtx.lineTo(i,300);mapCtx.stroke();
    mapCtx.beginPath();mapCtx.moveTo(0,i);mapCtx.lineTo(300,i);mapCtx.stroke();
  }
  mapCtx.strokeStyle='#2a5a2a'; mapCtx.lineWidth=1;
  mapCtx.beginPath();mapCtx.moveTo(150,0);mapCtx.lineTo(150,300);mapCtx.stroke();
  mapCtx.beginPath();mapCtx.moveTo(0,150);mapCtx.lineTo(300,150);mapCtx.stroke();
}

function getMapPos(e){
  var rect=document.getElementById('mapCanvas').getBoundingClientRect();
  var t=e.touches?e.touches[0]:e;
  return {x:t.clientX-rect.left, y:t.clientY-rect.top};
}

function mapDown(e){
  e.preventDefault(); mapDrawing=true; pathPoints=[]; drawGrid();
  var p=getMapPos(e); pathPoints.push(p);
  mapCtx.beginPath(); mapCtx.arc(p.x,p.y,6,0,Math.PI*2);
  mapCtx.fillStyle='lime'; mapCtx.fill();
  mapCtx.beginPath(); mapCtx.moveTo(p.x,p.y);
}

function mapMove(e){
  e.preventDefault(); if(!mapDrawing)return;
  var p=getMapPos(e); pathPoints.push(p);
  mapCtx.lineTo(p.x,p.y); mapCtx.strokeStyle='cyan'; mapCtx.lineWidth=2; mapCtx.stroke();
}

function mapUp(e){
  e.preventDefault(); mapDrawing=false;
  if(pathPoints.length>1){
    var last=pathPoints[pathPoints.length-1];
    mapCtx.beginPath(); mapCtx.arc(last.x,last.y,6,0,Math.PI*2);
    mapCtx.fillStyle='red'; mapCtx.fill();
  }
}

setTimeout(function(){
  var c=document.getElementById('mapCanvas');
  c.addEventListener('mousedown',mapDown); c.addEventListener('mousemove',mapMove); c.addEventListener('mouseup',mapUp);
  c.addEventListener('touchstart',mapDown); c.addEventListener('touchmove',mapMove); c.addEventListener('touchend',mapUp);
},500);

function clearMap(){ pathPoints=[]; if(mapCtx)drawGrid(); document.getElementById('mapLog').innerHTML=''; }

function processPath(){
  if(pathPoints.length<10){ mapLog('Draw a longer path!','cmd-error'); return; }
  var sm=pathPoints;
  for(var pass=0;pass<5;pass++){
    var temp=[];
    for(var i=0;i<sm.length;i++){
      var sx=0,sy=0,n=0;
      for(var j=Math.max(0,i-10);j<=Math.min(sm.length-1,i+10);j++){ sx+=sm[j].x; sy+=sm[j].y; n++; }
      temp.push({x:sx/n, y:sy/n});
    }
    sm=temp;
  }
  var step=Math.max(1,Math.floor(sm.length/7));
  var simp=[sm[0]];
  for(var i=step;i<sm.length-step;i+=step) simp.push(sm[i]);
  simp.push(sm[sm.length-1]);
  for(var i=0;i<simp.length;i++){
    mapCtx.beginPath(); mapCtx.arc(simp[i].x,simp[i].y,4,0,Math.PI*2);
    mapCtx.fillStyle=i===0?'lime':i===simp.length-1?'red':'yellow'; mapCtx.fill();
  }
  var gridCm=parseInt(document.getElementById('gridScale').value)||50;
  var pxPerCm=30.0/gridCm, msPerCm=MS_PER_METER/100.0;
  var commands=[], heading=-90;
  for(var i=0;i<simp.length-1;i++){
    var dx=simp[i+1].x-simp[i].x, dy=simp[i+1].y-simp[i].y;
    var angle=Math.atan2(dy,dx)*(180/Math.PI);
    var len=Math.sqrt(dx*dx+dy*dy);
    var distMs=Math.round((len/pxPerCm)*msPerCm);
    var turn=angle-heading;
    while(turn>180)turn-=360; while(turn<-180)turn+=360;
    if(Math.abs(turn)>35){
      var turnMs=Math.max(800,Math.round(Math.abs(turn)/90.0*MS_PER_90_TURN));
      commands.push({action:turn>0?'pivotright':'pivotleft', time_ms:turnMs});
    }
    if(distMs>500) commands.push({action:'forward', time_ms:Math.max(1500,distMs)});
    heading=angle;
  }
  var merged=[];
  for(var i=0;i<commands.length;i++){
    var p=merged.length>0?merged[merged.length-1]:null;
    if(commands[i].action==='forward'&&p&&p.action==='forward'){ p.time_ms+=commands[i].time_ms; }
    else merged.push({action:commands[i].action, time_ms:commands[i].time_ms});
  }
  commands=merged;
  if(commands.length===0){ mapLog('Path too short!','cmd-error'); return; }
  mapLog('Generated '+commands.length+' commands:','cmd-active');
  for(var i=0;i<commands.length;i++) mapLog('  ['+(i+1)+'] '+commands[i].action.toUpperCase()+' '+(commands[i].time_ms/1000).toFixed(1)+'s','cmd-done');
  mapLog('Executing...','cmd-active');
  cmd('map');
  var idx=0;
  function runMapNext(){
    if(idx>=commands.length){ mapLog('TRAJECTORY COMPLETE!','cmd-active'); cmd('stop'); return; }
    var c=commands[idx];
    mapLog('['+(idx+1)+'/'+commands.length+'] '+c.action.toUpperCase()+' '+(c.time_ms/1000).toFixed(1)+'s','cmd-active');
    cmd(c.action);
    setTimeout(function(){ cmd('stop'); setTimeout(function(){ idx++; runMapNext(); },500); }, c.time_ms);
  }
  runMapNext();
}

function mapLog(msg,cls){
  var d=document.getElementById('mapLog');
  var el=document.createElement('div');
  el.className=cls||''; el.textContent='> '+msg;
  d.appendChild(el); d.scrollTop=d.scrollHeight;
}

// ====== CHORD / MUSIC MODE ======
var musicActive=false,audioCtx=null,analyser=null,micStream=null,detectInterval=null;
var lastChord='',chordCount=0,lastCmdChord='',lastCmdTime=0;
var STABLE=6, COOLDOWN=1800, refractoryUntil=0;

var CHORDS={
  'C' :{notes:[0,4,7],  root:0, action:'forward',  label:'⬆ FORWARD',  color:'#27ae60'},
  'Am':{notes:[9,0,4],  root:9, action:'backward', label:'⬇ BACKWARD', color:'#e74c3c'},
  'F' :{notes:[5,9,0],  root:5, action:'left',     label:'⬅ LEFT',     color:'#2980b9'},
  'G' :{notes:[7,11,2], root:7, action:'right',    label:'➡ RIGHT',    color:'#f39c12'}
};

function showMusic(){ document.getElementById('musicPanel').style.display='block'; }
function toggleMusic(){ if(musicActive){stopMusic();}else{startMusic();} }

function startMusic(){
  musicActive=true; chordCount=0; lastChord='';
  document.getElementById('musicBtn').style.background='#27ae60';
  document.getElementById('musicBtn').classList.add('listening');
  document.getElementById('musicStatus').textContent='Strum and HOLD a chord...';
  cmd('music');
  navigator.mediaDevices.getUserMedia({audio:true}).then(function(stream){
    micStream=stream;
    audioCtx=new(window.AudioContext||window.webkitAudioContext)();
    var src=audioCtx.createMediaStreamSource(stream);
    analyser=audioCtx.createAnalyser();
    analyser.fftSize=8192; analyser.smoothingTimeConstant=0.65;
    src.connect(analyser);
    detectInterval=setInterval(runDetect,80);
  }).catch(function(e){ musicLog('Mic error:'+e.message,'cmd-error'); stopMusic(); });
}

function stopMusic(){
  musicActive=false;
  if(detectInterval){clearInterval(detectInterval);detectInterval=null;}
  document.getElementById('musicBtn').style.background='#e91e63';
  document.getElementById('musicBtn').classList.remove('listening');
  document.getElementById('musicStatus').textContent='Stopped';
  document.getElementById('chordDisplay').textContent='-';
  document.getElementById('freqDisplay').textContent='waiting...';
  document.getElementById('cmdDisplay').textContent='WAITING';
  document.getElementById('stableBar').style.width='0%';
  if(micStream){micStream.getTracks().forEach(function(t){t.stop();});micStream=null;}
  if(audioCtx){audioCtx.close();audioCtx=null;}
  cmd('stop');
}

function runDetect(){
  if(!analyser)return;
  if(Date.now()<refractoryUntil)return;
  var fd=new Uint8Array(analyser.frequencyBinCount);
  analyser.getByteFrequencyData(fd);
  var bHz=audioCtx.sampleRate/analyser.fftSize;
  var lo=Math.floor(200/bHz), hi=Math.floor(1500/bHz);
  var peak=0;
  for(var i=lo;i<hi;i++) if(fd[i]>peak) peak=fd[i];
  if(peak<35){
    chordCount=Math.max(0,chordCount-1);
    if(chordCount===0){
      document.getElementById('chordDisplay').textContent='-';
      document.getElementById('stableBar').style.width='0%';
      document.getElementById('freqDisplay').textContent='Strum louder...';
    }
    return;
  }
  var thr=peak*0.38, peaks=[];
  for(var i=lo+3;i<hi-3;i++){
    if(fd[i]>thr&&fd[i]>fd[i-1]&&fd[i]>fd[i-2]&&fd[i]>fd[i-3]
                &&fd[i]>fd[i+1]&&fd[i]>fd[i+2]&&fd[i]>fd[i+3]){
      peaks.push({hz:i*bHz,amp:fd[i]});
    }
  }
  if(peaks.length<2){
    document.getElementById('freqDisplay').textContent='Peaks:'+peaks.length+' (strum louder)';
    return;
  }
  peaks.sort(function(a,b){return b.amp-a.amp;}); peaks=peaks.slice(0,6);
  document.getElementById('freqDisplay').textContent=
    peaks.slice(0,3).map(function(p){return Math.round(p.hz)+'Hz';}).join(' | ');
  var pcs=[];
  peaks.forEach(function(p){
    var pc=((Math.round(12*(Math.log2(p.hz/440))+69)%12)+12)%12;
    if(pcs.indexOf(pc)===-1) pcs.push(pc);
  });
  var DISC={'C':[7],'Am':[9],'F':[5],'G':[11,2]};
    // (inside runDetect, after DISC definition)
  var best=null, bestScore=-99;
  for(var name in CHORDS){
    var ch=CHORDS[name], score=0;
    ch.notes.forEach(function(pc){ if(pcs.indexOf(pc)!==-1) score+=3; });
    if(pcs.indexOf(ch.root)!==-1) score+=5;
    DISC[name].forEach(function(pc){ if(pcs.indexOf(pc)!==-1) score+=4; });
    pcs.forEach(function(pc){ if(ch.notes.indexOf(pc)===-1) score-=1.5; });
    for(var rival in DISC){
      if(rival===name) continue;
      DISC[rival].forEach(function(pc){ if(pcs.indexOf(pc)!==-1) score-=3; });
    }
    var norm=score/(ch.notes.length*3+9);
    if(norm>bestScore){bestScore=norm;best=name;}
  }
  if(bestScore<0.45||!best) return;
  if(best!==lastChord&&lastChord!==''){
    if(bestScore<0.62) return;
  }
  document.getElementById('chordDisplay').textContent=best;
  if(best===lastChord){ chordCount++; }
  else { lastChord=best; chordCount=1; return; }
  var pct=Math.min(100,Math.round((chordCount/STABLE)*100));
  document.getElementById('stableBar').style.width=pct+'%';
  document.getElementById('stableBar').style.background=CHORDS[best].color;
  if(chordCount===STABLE) fireChord(best);
}

function fireChord(name){
  var now=Date.now(), ch=CHORDS[name];
  if(name===lastCmdChord&&now-lastCmdTime<COOLDOWN) return;
  lastCmdChord=name; lastCmdTime=now; chordCount=0; lastChord='';
  refractoryUntil=now+900;
  document.getElementById('cmdDisplay').textContent=ch.label;
  document.getElementById('cmdDisplay').style.color=ch.color;
  musicLog('Chord: '+name+' → '+ch.label,'cmd-active');
  cmd(ch.action);
}

function musicLog(msg,cls){
  var d=document.getElementById('musicLog');
  var el=document.createElement('div');
  el.className=cls||''; el.textContent='> '+msg;
  d.appendChild(el); d.scrollTop=d.scrollHeight;
  if(d.children.length>40) d.removeChild(d.firstChild);
}
</script>
</body>
</html>
)rawliteral";

// ========================================
// SETUP
// ========================================
void setup() {
  Serial.begin(115200);
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(TRIG,OUTPUT); pinMode(ECHO,INPUT);

  ledcAttach(ENA, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(ENB, PWM_FREQ, PWM_RESOLUTION);

  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi");
  int attempts=0;
  while(WiFi.status()!=WL_CONNECTED && attempts<20){ delay(500); Serial.print("."); attempts++; }

  if(WiFi.status()==WL_CONNECTED){
    Serial.println("\nConnected!");
    Serial.print("Open this IP on your phone: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED! Falling back to AP mode");
    WiFi.softAP("ESP32_Robot","12345678");
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  }

  server.on("/", handleRoot);
  server.on("/api/status", handleApiStatus);
  server.on("/api/forward",  [](){ autoMode=false; currentMode="MANUAL"; forwardMotion();    server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/backward", [](){ autoMode=false; currentMode="MANUAL"; backwardMotion();   server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/left",     [](){ autoMode=false; currentMode="MANUAL"; leftMotion();       server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/right",    [](){ autoMode=false; currentMode="MANUAL"; rightMotion();      server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/stop",     [](){ autoMode=false; stopMotors();         server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/auto",     [](){ autoMode=true;  currentMode="AUTO";   autoState=AUTO_FORWARD; server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/manual",   [](){ autoMode=false; currentMode="MANUAL"; stopMotors(); server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/voice",    [](){ autoMode=false; currentMode="VOICE";  server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/map",      [](){ autoMode=false; currentMode="MAP";    server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/music",    [](){ autoMode=false; currentMode="MUSIC";  server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/pivotleft",  [](){ autoMode=false; currentMode="MAP"; turnLeftMotion();  server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/pivotright", [](){ autoMode=false; currentMode="MAP"; turnRightMotion(); server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/speed50",  [](){ speedLevel=50;  speedValue=180; server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/speed80",  [](){ speedLevel=80;  speedValue=210; server.send(200,"application/json","{\"ok\":true}"); });
  server.on("/api/speed100", [](){ speedLevel=100; speedValue=255; server.send(200,"application/json","{\"ok\":true}"); });

  server.begin();
  Serial.println("Server started!");
}

// ========================================
// LOOP
// ========================================
void loop() {
  server.handleClient();
  distance = getDistance();
  if(autoMode){
    switch(autoState){
      case AUTO_FORWARD:
        if(distance<15){ stopMotors(); autoTimer=millis(); autoState=AUTO_STOPPING; }
        else if(robotStatus!="FORWARD"){ forwardMotion(); }
        break;
      case AUTO_STOPPING:
        if(millis()-autoTimer>=300){ turnRightMotion(); autoTimer=millis(); autoState=AUTO_TURNING; }
        break;
      case AUTO_TURNING:
        if(millis()-autoTimer>=700){ autoState=AUTO_FORWARD; }
        break;
    }
  }
}

// ========================================
// API STATUS
// ========================================
void handleApiStatus() {
  String json = "{\"distance\":" + String(distance) +
                ",\"mode\":\"" + currentMode +
                "\",\"status\":\"" + robotStatus +
                "\",\"speed\":" + String(speedLevel) +
                ",\"arrow\":\"" + (robotStatus=="STOP"?"⏹":
                                   robotStatus=="FORWARD"?"⬆":
                                   robotStatus=="BACKWARD"?"⬇":
                                   robotStatus=="LEFT"?"⬅":
                                   robotStatus=="RIGHT"?"➡":"↪") + "\"}";
  server.send(200, "application/json", json);
}

void handleRoot() { server.send_P(200, "text/html", index_html); }

// ========================================
// DISTANCE
// ========================================
float getDistance() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  duration = pulseIn(ECHO, HIGH, 25000);
  if(duration==0) return 999;
  return duration * 0.034 / 2;
}

// ========================================
// MOTOR FUNCTIONS
// ========================================
void forwardMotion() {
  robotStatus="FORWARD";
  ledcWrite(ENA,speedValue); ledcWrite(ENB,speedValue);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}
void backwardMotion() {
  robotStatus="BACKWARD";
  ledcWrite(ENA,speedValue); ledcWrite(ENB,speedValue);
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);
}
void leftMotion() {
  robotStatus="LEFT";
  ledcWrite(ENA,speedValue); ledcWrite(ENB,speedValue);
  digitalWrite(IN1,LOW);  digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}
void rightMotion() {
  robotStatus="RIGHT";
  ledcWrite(ENA,speedValue); ledcWrite(ENB,speedValue);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);  digitalWrite(IN4,LOW);
}
void turnRightMotion() {
  robotStatus="AUTO TURN";
  ledcWrite(ENA,speedValue); ledcWrite(ENB,speedValue);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);
}
void turnLeftMotion() {
  robotStatus="AUTO TURN";
  ledcWrite(ENA,speedValue); ledcWrite(ENB,speedValue);
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}
void stopMotors() {
  robotStatus="STOP";
  ledcWrite(ENA,0); ledcWrite(ENB,0);
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
}
// SPDX-License-Identifier: MPL-2.0
import { Updater } from './lib/fm1ota.js';
import { verifyPackage } from './verify.js?v=3';
import { readReleaseManifest, loadVerifiedRelease } from './releases.js?v=2';
import { t, onLang } from './i18n.js?v=1';
const $ = id => document.getElementById(id);
let midi, updater, connected, meta, image, writing = false, loading = false;
const supported = isSecureContext && !!navigator.requestMIDIAccess;
const lines = [];
function log(message) {
  lines.push(`${new Date().toISOString().slice(11,19)} ${message}`);
  if (lines.length > 200) lines.shift();
  $('log').textContent = lines.join('\n');
}
// Visible texts are kept as keys so a language switch re-renders them.
const shown = {};
function show(id, key, vars) { shown[id] = { key, vars }; $(id).textContent = t(key, vars); }
function rerender() { for (const id in shown) $(id).textContent = t(shown[id].key, shown[id].vars); }
function status(key, percent, vars) {
  show('status', key, vars);
  if (percent != null) {
    $('progress').value = Math.min(100, percent);
    $('percent').textContent = `${Math.round(Math.min(100, percent))}%`;
  }
}
function refresh() {
  $('connect').disabled = !supported || writing || loading;
  $('ports').disabled = !midi || writing || loading;
  $('load').disabled = !meta || writing || loading;
  $('file').disabled = !meta || writing || loading;
  $('ack').disabled = writing;
  $('install').disabled = !supported || !connected || !image || !$('ack').checked || writing || loading;
}
function error(e) { $('error').textContent = e.message || String(e); log($('error').textContent); }
const messages = Object.fromEntries(['denied','notfound','model','lost','stopped','noloader','noreturn','mismatch','open'].map(k => [k, `msg.${k}`]));
const message = code => messages[code] && t(messages[code]);
async function probe() {
  connected = null;
  const input = midi.inputs.get($('ports').value);
  const output = input && [...midi.outputs.values()].find(p => p.name === input.name && p.state !== 'disconnected');
  if (!input || !output) throw new Error(t('msg.nopair'));
  // Only the selected physical port is probed; the updater never scans other gear.
  updater = new Updater({inputs:new Map([[input.id,input]]),outputs:new Map([[output.id,output]])},log);
  const dev = await updater.find();
  if (!dev) throw new Error(message('notfound'));
  dev.link.close();
  if (dev.id.model !== 'FM-1' && !/^ota-/i.test(dev.id.model)) throw new Error(message('model'));
  if (dev.id.open) throw new Error(message('open'));
  connected = dev.id;
  delete shown.device; $('device').textContent = `${input.name} / ${dev.id.text}`;
  status('st.connected',0);
  log(`Device: ${dev.id.text}`);
}
$('connect').addEventListener('click',async()=>{
  $('error').textContent=''; loading=true; refresh();
  try {
    midi = await navigator.requestMIDIAccess({sysex:true});
    const inputs=[...midi.inputs.values()].filter(p=>p.state!=='disconnected' && /fm-?1|noct|felucca|ota/i.test(p.name||''));
    $('ports').replaceChildren(...inputs.map(p=>new Option(p.name,p.id)));
    if (!inputs.length) throw new Error(t('msg.noport'));
    if(inputs.length===1) await probe();
    else status('st.several');
    midi.onstatechange=()=>{
      if(writing)return;
      if(!midi.inputs.get($('ports').value) || midi.inputs.get($('ports').value).state==='disconnected'){
        connected=null; show('device','st.unplugged'); refresh();
      }
    };
  } catch(e) {error(e.name==='NotAllowedError'?new Error(message('denied')):e);}
  finally {loading=false;refresh();}
});
$('ports').addEventListener('change',async()=>{
  loading=true;refresh();$('error').textContent='';
  try{await probe();}catch(e){error(e);}finally{loading=false;refresh();}
});
async function choose(raw,name){
  image=null;
  image=await verifyPackage(raw,meta);
  packageReady(name);
}
function showBuild(){
  show('build','build.line',{version:meta.version,kb:Math.round(meta.bytes/1024)});
}
function packageReady(name){
  show('package','pkg.line',{name,product:meta.product});
  status('st.verified',0);
  log(`Verified ${meta.product}, SHA-256 ${meta.sha256}`);
}
$('load').addEventListener('click',async()=>{
  loading=true;image=null;$('error').textContent='';refresh();
  status('st.downloading',0);
  try{
    const release=await loadVerifiedRelease(location.href);
    meta=release.meta;image=release.image;
    showBuild();packageReady(meta.version);
  }catch(e){show('package','st.badfile');error(e);status('st.notloaded',0);}
  finally{loading=false;refresh();}
});
$('file').addEventListener('change',async e=>{
  image=null;loading=true;$('error').textContent='';refresh();
  try{const f=e.target.files[0];if(f)await choose(new Uint8Array(await f.arrayBuffer()),f.name);}
  catch(e){show('package','st.badfile');error(e);}
  finally{loading=false;refresh();}
});
$('ack').addEventListener('change',refresh);
function step(k,n){
  if(k!=='write'&&k!=='verify')log(`${k} ${n??''}`);
  if(k==='start')status('st.checkinstalled',1);
  if(k==='verify')status('st.prepare',Math.min(19,1+n/4));
  if(k==='loader')status('st.loader',20);
  if(k==='write')status('st.writing',Math.min(95,20+75*n/Math.ceil(image.length/512)));
  if(k==='reboot')status('st.reboot',97);
}
$('install').addEventListener('click',async()=>{
  if($('install').disabled)return;
  writing=true;$('error').textContent='';refresh();
  // Once rebooted into the loader its port ID changes. The library follows the
  // same FM-1 name; require exactly one FM-1 connected before starting.
  const eligible=[...midi.inputs.values()].filter(p=>p.state!=='disconnected'&&/fm-?1|noct|felucca|ota/i.test(p.name||''));
  try{
    if(eligible.length!==1)throw new Error(t('msg.onlyone'));
    const up=new Updater(midi,log);
    const ota=await up.find(id=>/^ota-/i.test(id.model));
    if(ota){
      ota.link.close();
      if(!await up.resume(image,step))throw new Error(message('stopped'));
      step('reboot');
      const back=await up.waitFor(id=>!/^ota-/i.test(id.model),40000);
      if(!back)throw new Error(message('noreturn'));
      const identity=back.id.text;back.link.close();
      if(identity!==meta.product)throw new Error(message('mismatch'));
    }else await up.install(image,meta.product,step);
    status('st.done',100);
    log(`Success: the device reports ${meta.product}`);
  }catch(e){error(new Error(message(e.code)||e.message));status('st.failed');}
  finally{writing=false;connected=null;show('device','st.reconnect');refresh();}
});
addEventListener('beforeunload',e=>{if(writing){e.preventDefault();e.returnValue='';}});
$('save-log').addEventListener('click',()=>{
  const url=URL.createObjectURL(new Blob([lines.join('\n')],{type:'text/plain;charset=utf-8'}));
  const a=document.createElement('a');a.href=url;a.download='noct-install.log';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
});
show('compatibility',supported?'compat.ok':'compat.no');
show('build','inst.checking');show('package','inst.nofile');show('device','inst.nodevice');status('st.ready');
onLang(rerender);
refresh();
try{
  const manifest=await readReleaseManifest(location.href);
  meta=manifest.meta;
  if(meta){
    showBuild();
  }else show('build','build.none');
}catch(e){meta=null;show('build','build.fail');error(e);}
refresh();

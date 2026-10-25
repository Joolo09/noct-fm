// SPDX-License-Identifier: MPL-2.0
import { Updater } from './lib/fm1ota.js';
import { verifyPackage } from './verify.js';
const $ = id => document.getElementById(id);
let midi, updater, connected, meta, image, writing = false, loading = false;
const supported = isSecureContext && !!navigator.requestMIDIAccess;
const lines = [];
function log(message) {
  lines.push(`${new Date().toISOString().slice(11,19)} ${message}`);
  if (lines.length > 200) lines.shift();
  $('log').textContent = lines.join('\n');
}
function status(message, percent) {
  $('status').textContent = message;
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
const messages = {
  denied:'Доступ к MIDI SysEx не разрешён. Разреши MIDI в настройках этого сайта.',
  notfound:'FM-1 не отвечает. Проверь USB-кабель и закрой DAW.',
  model:'Устройство не является FM-1. Запись не началась.',
  lost:'USB-соединение потеряно. Подключи тот же FM-1 и повтори установку тем же файлом.',
  stopped:'Установка остановилась. Сохрани журнал и проверь режим загрузчика.',
  noloader:'Загрузчик не появился. Переподключи FM-1 и повтори установку.',
  noreturn:'Запись закончилась, но FM-1 не вернулся. Перезапусти устройство и проверь версию.',
  mismatch:'После записи устройство сообщает другую версию. Установка не подтверждена.',
  open:'Сначала верни штатную прошивку через официальный M-UPGRADE: текущий Open-загрузчик не принимает NOCT напрямую.'
};
async function probe() {
  connected = null;
  const input = midi.inputs.get($('ports').value);
  const output = input && [...midi.outputs.values()].find(p => p.name === input.name && p.state !== 'disconnected');
  if (!input || !output) throw new Error('Не найдена пара MIDI IN/OUT. Проверь USB и выбери порт FM-1.');
  // Only the selected physical port is probed; the updater never scans other gear.
  updater = new Updater({inputs:new Map([[input.id,input]]),outputs:new Map([[output.id,output]])},log);
  const dev = await updater.find();
  if (!dev) throw new Error(messages.notfound);
  dev.link.close();
  if (dev.id.model !== 'FM-1' && !/^ota-/i.test(dev.id.model)) throw new Error(messages.model);
  if (dev.id.open) throw new Error(messages.open);
  connected = dev.id;
  $('device').textContent = `${input.name} / ${dev.id.text}`;
  status('FM-1 подключён. Выбери сборку.',0);
  log(`Устройство: ${dev.id.text}`);
}
$('connect').addEventListener('click',async()=>{
  $('error').textContent=''; loading=true; refresh();
  try {
    midi = await navigator.requestMIDIAccess({sysex:true});
    const inputs=[...midi.inputs.values()].filter(p=>p.state!=='disconnected' && /fm-?1|noct|felucca|ota/i.test(p.name||''));
    $('ports').replaceChildren(...inputs.map(p=>new Option(p.name,p.id)));
    if (!inputs.length) throw new Error('Не найден MIDI-порт FM-1. Подключи устройство по USB.');
    if(inputs.length===1) await probe();
    else status('Найдено несколько устройств. Выбери нужный FM-1 в списке.');
    midi.onstatechange=()=>{
      if(writing)return;
      if(!midi.inputs.get($('ports').value) || midi.inputs.get($('ports').value).state==='disconnected'){
        connected=null; $('device').textContent='USB отключён'; refresh();
      }
    };
  } catch(e) {error(e.name==='NotAllowedError'?new Error(messages.denied):e);}
  finally {loading=false;refresh();}
});
$('ports').addEventListener('change',async()=>{
  loading=true;refresh();$('error').textContent='';
  try{await probe();}catch(e){error(e);}finally{loading=false;refresh();}
});
async function choose(raw,name){
  image=null;
  image=await verifyPackage(raw,meta);
  $('package').textContent=`${name} / SHA-256 проверен / ${meta.product}`;
  status('Файл проверен. Можно перейти к установке.',0);
  log(`Проверен ${meta.product}, SHA-256 ${meta.sha256}`);
}
$('load').addEventListener('click',async()=>{
  loading=true;image=null;$('error').textContent='';refresh();
  try{
    const res=await fetch(meta.url,{cache:'no-store'});
    if(!res.ok)throw new Error(`Не удалось скачать сборку: HTTP ${res.status}`);
    await choose(new Uint8Array(await res.arrayBuffer()),meta.version);
  }catch(e){error(e);}finally{loading=false;refresh();}
});
$('file').addEventListener('change',async e=>{
  image=null;loading=true;$('error').textContent='';refresh();
  try{const f=e.target.files[0];if(f)await choose(new Uint8Array(await f.arrayBuffer()),f.name);}
  catch(e){$('package').textContent='Файл не прошёл проверку';error(e);}
  finally{loading=false;refresh();}
});
$('ack').addEventListener('change',refresh);
function step(k,n){
  if(k!=='write'&&k!=='verify')log(`${k} ${n??''}`);
  if(k==='start')status('Проверяем установленную прошивку…',1);
  if(k==='verify')status('Подготовка загрузчика…',Math.min(19,1+n/4));
  if(k==='loader')status('Переходим в режим записи…',20);
  if(k==='write')status('Запись прошивки. Не отключай USB.',Math.min(95,20+75*n/Math.ceil(image.length/512)));
  if(k==='reboot')status('FM-1 перезапускается. Проверяем версию…',97);
}
$('install').addEventListener('click',async()=>{
  if($('install').disabled)return;
  writing=true;$('error').textContent='';refresh();
  // Once rebooted into the loader its port ID changes. The library follows the
  // same FM-1 name; require exactly one FM-1 connected before starting.
  const eligible=[...midi.inputs.values()].filter(p=>p.state!=='disconnected'&&/fm-?1|noct|felucca|ota/i.test(p.name||''));
  try{
    if(eligible.length!==1)throw new Error('Оставь подключённым только один FM-1 на время установки.');
    const up=new Updater(midi,log);
    const ota=await up.find(id=>/^ota-/i.test(id.model));
    if(ota){
      ota.link.close();
      if(!await up.resume(image,step))throw new Error(messages.stopped);
      step('reboot');
      const back=await up.waitFor(id=>!/^ota-/i.test(id.model),40000);
      if(!back)throw new Error(messages.noreturn);
      const identity=back.id.text;back.link.close();
      if(identity!==meta.product)throw new Error(messages.mismatch);
    }else await up.install(image,meta.product,step);
    status('NOCT FM установлен. Версия устройства подтверждена.',100);
    log(`Успешно: устройство сообщает ${meta.product}`);
  }catch(e){error(new Error(messages[e.code]||e.message));status('Установка не подтверждена. Проверь сообщение и журнал.');}
  finally{writing=false;connected=null;$('device').textContent='Для новой операции подключи устройство повторно';refresh();}
});
addEventListener('beforeunload',e=>{if(writing){e.preventDefault();e.returnValue='';}});
$('save-log').addEventListener('click',()=>{
  const url=URL.createObjectURL(new Blob([lines.join('\n')],{type:'text/plain;charset=utf-8'}));
  const a=document.createElement('a');a.href=url;a.download='noct-install.log';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
});
$('compatibility').textContent=supported?'Браузер поддерживает Web MIDI SysEx. Доступ запрашивается только по кнопке подключения.':'Нужен Chrome или Edge на компьютере и HTTPS. В этом браузере установка недоступна.';
refresh();
try{
  const response=await fetch('./firmware.json',{cache:'no-store'});
  if(!response.ok)throw new Error(`Манифест недоступен: HTTP ${response.status}`);
  const manifest=await response.json();
  if(manifest.schema!==1||!Array.isArray(manifest.builds))throw new Error('Неподдерживаемый манифест.');
  meta=manifest.builds[0]||null;
  if(meta){
    const url=new URL(meta.url,location.href);
    if(url.origin!==location.origin || !url.pathname.startsWith(new URL('./firmware/',location.href).pathname))throw new Error('Неверный адрес сборки.');
    $('build').textContent=`${meta.version} / USB MIDI / ${Math.round(meta.bytes/1024)} КБ / экспериментальная сборка`;
  }else $('build').textContent=manifest.message||'Проверенная сборка пока не опубликована.';
}catch(e){meta=null;$('build').textContent='Не удалось получить сборку.';error(e);}
refresh();

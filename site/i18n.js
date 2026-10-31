// SPDX-License-Identifier: MPL-2.0
// English is the page's own text (index.html); Russian replaces it on request.
// Installer messages live here in both languages. Works without a DOM (tests).
const ru = {
  'title':'NOCT FM — MIDI-прошивка для M-VAVE FM-1',
  'nav.how':'Как работает','nav.pages':'Страницы','nav.buttons':'Кнопки','nav.install':'Установка',
  'hero.title':'MIDI-контроллер для FL&nbsp;Studio и Ableton&nbsp;Live',
  'hero.lead':'Прошивка превращает FM-1 в контроллер для DAW: клавиатура, аккорды, генератор мелодий, арпеджиатор, ручки микшера и транспорт. Встроенный синтезатор отключён — весь звук делает DAW.',
  'cta.install':'Установить','cta.scripts':'Скрипты для DAW',
  'fact.keys':'клавиш, F3–G5, ±3 октавы','fact.pages':'страниц по 4 ручки','fact.chords':'типов аккордов','fact.scales':'ладов в генераторе','fact.phrases':'фраз на одно зерно — крутилка ALGORITHM',
  'hero.caption':'настоящий кадр прошивки, 240×240',
  'how.title':'Как это работает',
  'flow.fm1':'клавиши, ручки, кнопки','flow.notes':'ноты · канал 1','flow.cc':'CC и транспорт · канал 16',
  'flow.back':'темп, play/rec, FX','flow.screen':'Экран','flow.screen2':'темп, доли, звучащие ноты',
  'how.1t':'Один пресет','how.1':'Ноты всегда идут на канале 1, ручки и транспорт — на канале 16. Ничего не нужно переключать между DAW.',
  'how.2t':'Страница = 4 ручки','how.2':'PRESET листает страницы, K1–K4 меняют параметры на экране. HOME возвращает ручки в микшер DAW.',
  'how.3t':'Синхронизация','how.3':'Режим AUTO берёт MIDI Clock из DAW, если он есть, а без него работает от внутреннего темпа. Скрипт DAW передаёт BPM в обе стороны.',
  'pages.title':'Страницы','pages.sub':'PRESET — следующая страница. Изменения генератора вступают со следующего такта.',
  'p.daw':'Ручки управляют микшером выбранного трека. Удержание HOME на 0,6 с включает MIDI MAP — свободные CC24–27 для любых параметров.',
  'p.daw1':'Громкость','p.daw2':'Панорама','p.daw4':'Канал / Send B',
  'p.chords':'Одна клавиша — целый аккорд. IN KEY строит аккорды в тональности. Держи SEL и нажми 2–5 клавиш — аккорд запомнится.',
  'p.chords1':'Тип: MAJ, MIN, 7, IN KEY, MEMORY…','p.chords2':'Обращение, OPEN, DROP 2, +BASS','p.chords3':'Страм 6–70 мс','p.chords4':'Клавиши только в ладу',
  'p.compose':'Генератор сочиняет фразу на 4 такта: мотив, ответ, контраст, завершение на тонике. Крути ALGORITHM — 1000 вариаций мелодии, ритма, баса и (с AUTO) аккордов.',
  'p.compose2':'Лад (8)','p.compose3':'Тоника','p.compose4':'Прогрессия (24) или AUTO',
  'p.rhythm':'Плотность и грув. Свинг действует и на генератор, и на ARP с сеткой 1/8 и 1/16.',
  'p.rhythm1':'Нот на такт, 0–16','p.rhythm2':'Длина нот','p.rhythm3':'Свинг 50–75%','p.rhythm4':'HUMAN — акценты velocity',
  'p.style':'Как звучат аккорды и какая выйдет мелодия: плавная, арпеджио, рифф, секвенция, педаль, скачки, вопрос–ответ.',
  'p.style2':'Трезвучия, септаккорды, add9, sus2, микс','p.style4':'Характер мелодии (AUTO + 7)','p.style3':'Диапазон 1–3 октавы',
  'p.arp':'Арпеджиатор по зажатым нотам. ARP + ENV — удержание без пальцев (latch).',
  'p.arp1':'Сетка 1/4 … 1/32','p.arp3':'Октавы 1–3','p.arp4':'Длина ноты',
  'p.sync':'Источник темпа. На этой странице GLO работает как tap tempo, и новый BPM сразу уходит в DAW.','p.sync4':'Чувствительность клавиш',
  'buttons.title':'Кнопки',
  'b.home':'Ручки → DAW. Удержание: MIXER / MIDI MAP','b.preset':'Листать страницы','b.algo':'Листать фразы #000–999','b.sel':'Аккорды вкл/выкл',
  'b.keys':'клавиши','b.learn':'Запомнить аккорд','b.seq':'Генератор: пуск / стоп','b.key':'клавиша','b.root':'Тоника генератора',
  'b.edit':'Новое зерно: 1000 новых фраз','b.hold':'0,6 с','b.edithold':'Новая мелодия и аккорды','b.undo':'Отменить, до 8 шагов',
  'b.arp':'Арпеджиатор вкл/выкл','b.glo':'SYNC; на SYNC — tap tempo','b.transport':'Транспорт DAW','b.fx':'Эффекты трека в DAW',
  'b.env':'Sustain, пока держишь','b.lfo':'Модуляция CC1','b.oct':'Октава ±3','b.select':'BPM ±1 на любой странице',
  'b.save':'Сохранить настройки','b.panic':'Panic — снять все ноты','b.master':'Мастер-громкость DAW',
  'buttons.sub':'Оранжевый квадрат в шапке экрана — есть несохранённые изменения. Сохранение происходит только по SAVE и только при остановленных нотах.',
  'inst.title':'Установка по USB','inst.badge':'экспериментальная',
  'inst.sub':'Chrome или Edge на компьютере. Перед установкой закрой DAW и подготовь официальный M-UPGRADE на случай отката.',
  'inst.1t':'Подключи FM-1','inst.1':'Кабель USB с передачей данных.','inst.connect':'Подключить','inst.port':'MIDI-порт','inst.portwait':'Сначала подключи устройство',
  'inst.2t':'Загрузи прошивку','inst.load':'Загрузить','inst.file':'или выбери файл .fwsc',
  'inst.3t':'Установи','inst.3':'Проверяются SHA-256, модель и идентификатор, затем образ записывается по USB MIDI.',
  'inst.ack':'Понимаю, что прошивка экспериментальная','inst.go':'Установить NOCT','inst.log':'Журнал','inst.savelog':'Скачать журнал',
  'inst.note':'Настройки 0.7 переносятся автоматически. Загрузчик, USB и запись во flash в этой версии не менялись. Если устройство не запускается, но видно в режиме обновления (OCT− и OCT+ на 5 с), верни заводскую прошивку через <a href="https://www.m-vave.com/product?id=fm-1" target="_blank" rel="noopener">M-UPGRADE</a>.',
  'daw.title':'Подключение к DAW',
  'daw.fl':'<li>Папку <code>FL Studio/NOCT</code> из архива скопируй в <code>Settings/Hardware/</code> внутри User data folder.</li><li>Перезапусти FL. В MIDI Settings выбери NOCT FM-1 → Controller type <b>NOCT FM</b>.</li><li>Назначь входу и выходу один Port number.</li><li>Для синхронизации включи Send master sync на выходе.</li>',
  'daw.live':'<li>Папку <code>Ableton Live/NOCT</code> скопируй в <code>Remote Scripts/</code> в User Library.</li><li>Перезапусти Live. Control Surface → <b>NOCT</b>, Input/Output → NOCT FM-1.</li><li>Включи Track на входе и Sync на выходе.</li><li>Выбери MIDI-дорожку с инструментом.</li>',
  'daw.scripts':'Скачать скрипты','daw.docs':'Полная документация',
  'footer':'На базе CTL-1, fm1-base и felucca-bsp · <a href="./LICENSE">MPL-2.0</a> · не связано с M-VAVE, Image-Line и Ableton',
  // installer (dynamic)
  'inst.checking':'Проверяем сборку…','inst.nofile':'Файл не выбран','inst.nodevice':'Не подключено','st.ready':'Готов к подключению.',
  'msg.denied':'Доступ к MIDI SysEx не разрешён. Разреши MIDI в настройках этого сайта.',
  'msg.notfound':'FM-1 не отвечает. Проверь USB-кабель и закрой DAW.',
  'msg.model':'Устройство не является FM-1. Запись не началась.',
  'msg.lost':'USB-соединение потеряно. Подключи тот же FM-1 и повтори установку тем же файлом.',
  'msg.stopped':'Установка остановилась. Сохрани журнал и проверь режим загрузчика.',
  'msg.noloader':'Загрузчик не появился. Переподключи FM-1 и повтори установку.',
  'msg.noreturn':'Запись закончилась, но FM-1 не вернулся. Перезапусти устройство и проверь версию.',
  'msg.mismatch':'После записи устройство сообщает другую версию. Установка не подтверждена.',
  'msg.open':'Сначала верни штатную прошивку через официальный M-UPGRADE: текущий Open-загрузчик не принимает NOCT напрямую.',
  'msg.nopair':'Не найдена пара MIDI IN/OUT. Проверь USB и выбери порт FM-1.',
  'msg.noport':'Не найден MIDI-порт FM-1. Подключи устройство по USB.',
  'msg.onlyone':'Оставь подключённым только один FM-1 на время установки.',
  'st.connected':'FM-1 подключён. Выбери сборку.','st.several':'Найдено несколько устройств. Выбери нужный FM-1 в списке.',
  'st.unplugged':'USB отключён','st.verified':'Файл проверен. Можно перейти к установке.',
  'st.downloading':'Скачиваем и проверяем опубликованную сборку…','st.notloaded':'Сборка не загружена. Запись не началась.',
  'st.badfile':'Файл не прошёл проверку','st.checkinstalled':'Проверяем установленную прошивку…','st.prepare':'Подготовка загрузчика…',
  'st.loader':'Переходим в режим записи…','st.writing':'Запись прошивки. Не отключай USB.','st.reboot':'FM-1 перезапускается. Проверяем версию…',
  'st.done':'NOCT FM установлен. Версия устройства подтверждена.','st.failed':'Установка не подтверждена. Проверь сообщение и журнал.',
  'st.reconnect':'Для новой операции подключи устройство повторно',
  'build.line':'{version} / USB MIDI / {kb} КБ / экспериментальная сборка','pkg.line':'{name} / SHA-256 проверен / {product}',
  'compat.ok':'Браузер поддерживает Web MIDI SysEx. Доступ запрашивается только по кнопке подключения.',
  'compat.no':'Нужен Chrome или Edge на компьютере и HTTPS. В этом браузере установка недоступна.',
  'build.none':'Проверенная сборка пока не опубликована.','build.fail':'Не удалось получить сборку.',
  'err.meta':'Неверные метаданные сборки NOCT.',
  'err.size':'Размер файла не соответствует опубликованной сборке: ожидалось {expected} байт, получено {got}. Повтори загрузку сборки.',
  'err.sha':'SHA-256 не совпадает. Файл повреждён или относится к другой сборке.',
  'err.product':'Файл содержит другой идентификатор прошивки.',
  'err.manifest':'Манифест недоступен: HTTP {status}','err.schema':'Неподдерживаемый манифест.','err.url':'Неверный адрес сборки.',
  'err.download':'Не удалось скачать сборку: HTTP {status}'
};
const en = {
  'inst.checking':'Checking the build…','inst.nofile':'No file selected','inst.nodevice':'Not connected','st.ready':'Ready to connect.',
  'msg.denied':'MIDI SysEx access was not granted. Allow MIDI in this site\'s settings.',
  'msg.notfound':'The FM-1 does not respond. Check the USB cable and close your DAW.',
  'msg.model':'This device is not an FM-1. Nothing was written.',
  'msg.lost':'The USB connection was lost. Reconnect the same FM-1 and install the same file again.',
  'msg.stopped':'The installation stopped. Save the log and check the loader mode.',
  'msg.noloader':'The loader did not appear. Reconnect the FM-1 and try again.',
  'msg.noreturn':'Writing finished but the FM-1 did not come back. Restart it and check the version.',
  'msg.mismatch':'After writing, the device reports a different version. The installation is not confirmed.',
  'msg.open':'Restore the stock firmware with the official M-UPGRADE first: the current Open loader does not accept NOCT directly.',
  'msg.nopair':'No MIDI IN/OUT pair found. Check USB and select the FM-1 port.',
  'msg.noport':'No FM-1 MIDI port found. Connect the device over USB.',
  'msg.onlyone':'Keep only one FM-1 connected during the installation.',
  'st.connected':'FM-1 connected. Choose a build.','st.several':'Several devices found. Select the FM-1 in the list.',
  'st.unplugged':'USB disconnected','st.verified':'Package verified. Ready to install.',
  'st.downloading':'Downloading and verifying the published build…','st.notloaded':'Build not loaded. Nothing was written.',
  'st.badfile':'The file failed verification','st.checkinstalled':'Checking the installed firmware…','st.prepare':'Preparing the loader…',
  'st.loader':'Switching to write mode…','st.writing':'Writing the firmware. Do not unplug USB.','st.reboot':'The FM-1 is restarting. Checking the version…',
  'st.done':'NOCT FM installed. Device version confirmed.','st.failed':'Installation not confirmed. Check the message and the log.',
  'st.reconnect':'Reconnect the device for another operation',
  'build.line':'{version} / USB MIDI / {kb} KB / experimental build','pkg.line':'{name} / SHA-256 verified / {product}',
  'compat.ok':'This browser supports Web MIDI SysEx. Access is requested only when you press Connect.',
  'compat.no':'Chrome or Edge on a computer over HTTPS is required. Installing is not available in this browser.',
  'build.none':'No verified build has been published yet.','build.fail':'Could not get the build.',
  'err.meta':'Invalid NOCT build metadata.',
  'err.size':'The file size does not match the published build: expected {expected} bytes, got {got}. Load the build again.',
  'err.sha':'SHA-256 mismatch. The file is damaged or belongs to another build.',
  'err.product':'The file carries a different firmware identity.',
  'err.manifest':'Manifest unavailable: HTTP {status}','err.schema':'Unsupported manifest.','err.url':'Invalid build address.',
  'err.download':'Could not download the build: HTTP {status}'
};
const dict = { en, ru };
let lang = 'en';
try { if (globalThis.localStorage?.getItem('noct-lang') === 'ru') lang = 'ru'; } catch {}
const listeners = [];

export function t(key, vars = {}) {
  const text = dict[lang][key] ?? en[key] ?? key;
  return text.replace(/\{(\w+)\}/g, (_, name) => String(vars[name] ?? ''));
}
export const getLang = () => lang;
export function onLang(fn) { listeners.push(fn); }

function apply() {
  const doc = globalThis.document;
  if (!doc?.querySelectorAll) return;
  doc.documentElement.lang = lang;
  for (const el of doc.querySelectorAll('[data-i18n]')) {
    const key = el.dataset.i18n;
    if (!(key in en)) en[key] = el.innerHTML;          // the page's English, captured once
    el.innerHTML = dict[lang][key] ?? en[key];
  }
  if (!('title' in en)) en.title = doc.title;
  doc.title = dict[lang].title ?? en.title;
  const button = doc.getElementById('lang');
  if (button) {
    button.textContent = lang === 'en' ? 'RU' : 'EN';
    button.setAttribute('aria-label', lang === 'en' ? 'Русский' : 'English');
  }
}
export function setLang(next) {
  lang = next === 'ru' ? 'ru' : 'en';
  try { globalThis.localStorage?.setItem('noct-lang', lang); } catch {}
  apply();
  for (const fn of listeners) fn();
}
apply();
globalThis.document?.getElementById?.('lang')?.addEventListener?.('click', () => setLang(lang === 'en' ? 'ru' : 'en'));

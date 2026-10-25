# NOCT MIDI 0.3.0

**AFTER HOURS / IN CONTROL** — один постоянный MIDI-пресет для M-VAVE FM-1, FL Studio и Ableton Live, теперь с синхронизацией темпа, аккордами и генератором нот.

**[Установщик и DAW-скрипты → joolo09.github.io/noct-fm](https://joolo09.github.io/noct-fm/)**

![Настоящий C-рендерер прошивки](preview/noct-fm.gif)

## Новое в 0.3

- Темп в обе стороны: DAW → FM-1 и SELECT → DAW, с шагом 0,1 BPM в протоколе и диапазоном 20,0–999,0 BPM. Физический SELECT меняет темп по 1 BPM.
- AUTO / INT / EXT синхронизация. AUTO использует входящий MIDI Clock, когда он есть, и внутреннюю сетку с темпом DAW, когда clock отсутствует. Синхронизация по фазе требует настоящего MIDI Clock.
- Обработка MIDI Start / Continue / Stop, состояния playback и записи из DAW. Stop освобождает ноты генератора/ARP; Continue сохраняет позицию, Start начинает рисунок с начала.
- Аккорды одной клавишей: Major, Minor, Sus2, Sus4, Maj7, Min7. SEL переключает аккордовый режим; ALGORITHM выбирает тип. Ещё одно SEL возвращает обычную хроматическую клавиатуру.
- Генератор из 16 шагов: Major / Minor / minor pentatonic, плотность 0–16, корень, диапазон 1–3 октавы. EDIT создаёт новый рисунок. При включённых аккордах генератор играет аккордами.
- FX больше не включает ARP latch. Теперь это переключение эффектов в DAW: effect slots выбранного mixer track в FL Studio или Device On выбранного устройства в Live. Без выбранного устройства в Live менять нечего; feedback показывает FX off.
- Общий счётчик владельцев нот для клавиатуры, перекрывающихся аккордов, ARP и генератора: отключение одного источника не обрывает ноту другого.

Постоянная маршрутизация 0.2 сохранена: клавиши всегда отправляют ноты на канале 1, четыре ручки — CC20–23 на канале 16. Пользовательских банков и смены MIDI-канала с панели нет. Синтезатор, USB audio и Bluetooth не включены. Повороты ручек и входящий feedback не пишут flash.

## Все кнопки и энкодеры

| Орган FM-1 | Действие |
| --- | --- |
| HOME | Главный экран |
| SEL | Аккорды вкл/выкл и экран аккордов |
| ARP | Арпеджиатор вкл/выкл |
| SEQ | Генератор вкл/выкл. Включение запускает транспорт; выключение останавливает только генератор |
| EDIT | Новый рисунок генератора и экран генератора |
| GLO | AUTO → INT → EXT синхронизация и экран синхронизации |
| FX | Эффекты выбранной дорожки/устройства DAW вкл/обход |
| ENV | Sustain, пока удерживается |
| LFO | Modulation CC1, плавно вверх/вниз |
| PLAY | Пуск/стоп DAW и генератора. После Stop новый нажим клавиши может снова запустить ARP для прослушивания |
| REC | Переключение общей записи DAW, не Session Record |
| SAVE | Сохранить настройки при остановленном транспорте, без активных нот и недавних поворотов ручек |
| OCT− / OCT+ | −3…+3 октавы; удержанные ноты переозвучиваются с правильными Note Off |
| SELECT | BPM +/−1; запрос изменения передаётся в DAW |
| ALGORITHM | Тип аккорда при включённом CHORDS, иначе порядок нот ARP |
| PRESETS | Лад генератора: Major / Minor / minor pentatonic. MIDI-маршрутизация не меняется |
| KNOB 1–4 | CC20–23, кроме явно удерживаемого ARP/SEQ для музыкальных настроек |
| MASTER | CC7 master DAW, только после движения |

Клавиши остаются музыкальными на всех экранах. В обычном режиме физические полутоны F3–G5; в CHORDS каждая клавиша играет аккорд от своей ноты, в том числе чёрная.

| Удерживать | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| ARP | Деление сетки | Порядок нот | Октавы ARP | Темп |
| SEQ | Деление сетки | Плотность 0–16 | Лад генератора | Октавы генератора |

ARP и генератор используют одно деление сетки, изначально 1/16. Удерживать SEQ и сыграть клавишу — выбрать корень генератора; сама клавиша тоже звучит. После настройки удерживаемого ARP/SEQ отпускание кнопки не переключает включение. Генератор использует gate 50% шага и velocity 100; редактирование gate с панели в этой версии не предусмотрено.

**ARP + ENV** — latch ARP вкл/выкл. **FX + REC** — panic: завершить ноты, sustain и текущую генерацию. **GLO + ENV** — фиксированная / измеренная velocity. По умолчанию velocity 100 без aftertouch. **GLO + PLAY** — также переключает режим синхронизации, для совместимости с прошлой раскладкой.

Генератор не стартует сам при загрузке или после перезапуска. Stop оставляет его включённым, но останавливает воспроизведение; PLAY/Start/Continue возобновляют его. Panic гасит генерацию до следующего явного запуска транспорта. Обычные удержанные клавиши остаются независимы от остановки генератора.

## Настройка FL Studio и Ableton

**Обязательно установи свежие DAW-скрипты 0.3:** они передают темп и FX. Скрипты 0.2 поддерживают только ноты, контроллеры и транспорт. Скрипты подключаются к одной DAW за раз.

**FL Studio:** папка `integrations/FL Studio/NOCT` → `Settings/Hardware/NOCT` внутри User data folder. Перезапусти FL, включи NOCT FM-1, выбери Controller type → NOCT FM. Входу и выходу назначь один Port number. Для двусторонней установки темпа нужна FL MIDI scripting API 38+ (`mixer.setCurrentTempo`), доступная в актуальной FL Studio. Для фазовой синхронизации включи Send master sync на выходе MIDI и Options → Enable MIDI master sync.

**Ableton Live 11/12:** папка `integrations/Ableton Live/NOCT` → `Remote Scripts/NOCT` внутри User Library. Перезапусти Live; Control Surface → NOCT; Input/Output → NOCT FM-1. Track включи для нот, Sync на MIDI-выходе — для MIDI Clock. Выбери MIDI-дорожку и инструмент; для FX выбери нужное устройство. Remote нужен только для дополнительных ручных MIDI Map.

| Ручка | FL Studio | Ableton Live |
| --- | --- | --- |
| KNOB 1 / CC20 | Volume выбранного mixer track | Volume выбранной track |
| KNOB 2 / CC21 | Pan выбранного mixer track | Pan выбранной track |
| KNOB 3 / CC22 | Stereo separation | Send A |
| KNOB 4 / CC23 | Volume выбранного channel | Send B |

BPM feedback работает и без Sync/Clock. Сами BPM-сообщения сообщают скорость, но не положение внутри такта: для точного старта по сетке включи MIDI Clock и используй Start/Continue. При отсутствии DAW-скрипта BPM оценивается по 24 входящим clock-сообщениям. При наличии скрипта приоритет у точного BPM DAW, чтобы не показывать дрожание из-за миллисекундного измерения.

Протокол на канале 16: CC7 master; CC20–23 ручки; CC110/111/112 play/stop/record; CC113 FX toggle (127), feedback состояния 0/127; CC114 BPM high и CC115 BPM low — 14-битное значение BPM×10, всегда парой; CC116/117 playback/recording feedback; CC119 value 2 — один MIDI-пресет. Ноты, sustain и modulation — канал 1. Обратная связь не эхоится и не вызывает запись flash.

## Установка, сборка и проверки

[Веб-установщик](https://joolo09.github.io/noct-fm/) работает в Chrome/Edge по HTTPS и USB MIDI SysEx. Пакет 0.3.0 имеет identity `FM-1_90625000300`. Проверяются SHA-256, размер, модель и identity после перезапуска. USB audio отсутствует; после установки проверь выбор MIDI-порта в DAW. Настройки 0.1/0.2 мигрируют с постоянной маршрутизацией, новыми настройками аккордов/генератора и режимом AUTO.

Это экспериментальная прошивка. Перед установкой сохрани данные и подготовь официальный M-UPGRADE и процедуру восстановления для своей версии FM-1. Полный аппаратный цикл с двумя DAW ещё не проверен. Мы проверяем настоящую C-программу в симуляторе и скрипты с подставными DAW API; эти проверки не заменяют тест USB, flash и звука внутри DAW на реальном FM-1.

```sh
python tests/test_integrations.py
sh build.sh --host --no-ble
mkdir -p test-output
firmware/app/build/host/noct-usb_host tests/noct.ctl test-output
python tests/test_midi_engine.py
node tests/installer.test.mjs
sh build.sh --no-ble --release 0.3.0
```

CI проверяет 12 тестов DAW-скриптов, 8 сценарных тестов C-программы и основной сценарий всех кнопок, 27 клавиш, 7 октав, CC, аккордов, совместного владения нотами, генератора, clock/BPM, Start/Stop/Continue, panic, сохранения и миграции. Сборка требует Linux x86-64, JieLi pi32v2 toolchain и AC79 SDK. Результат: `firmware/app/build/noct-usb-0.3.0.fwsc`. Сценарий GitHub Pages публикует последнюю успешную сборку. Исходники сайта — `site/`.

На Windows для host нужны Python 3.11+, Pillow и Zig 0.13.0; `tools/build-host.ps1 -NoBle`. Host — симулятор, он не управляет реальным MIDI и не прошивает устройство.

## Основа и лицензии

На основе [CTL-1](https://github.com/charlesvestal/fm1-ctl) `e181b4a1ac1510ce18ebf6e271a143c2fe364ccd`, fm1-base `b16c177a9086be955cd3e418383f2fd92c6d658b` и felucca-bsp `b4bb32ee0729595b7a30fbf239721286444eceb0`. Активное приложение: `app.c`, `project.c`, `note_bus.c`, `midi_engine.c`, `midi_ui.c`, `midi_screen.c`, `midi_host.c`; старые UI/engine-файлы сохранены как исходный материал и не включены в сборку.

MPL-2.0 для приложения и модификаций; авторские заголовки сохранены. Лицензии: `LICENSE`, `upstream/LICENSING.md`, `upstream/LICENSES`, `upstream/base/`. Terminus — SIL OFL. Sonivox EAS сохранён в исходниках под Apache-2.0, но не линкуется в MIDI-сборку. Проект независим от M-VAVE, Image-Line и Ableton; логотипы обозначают совместимые DAW. Development identity экспериментальная.

Документация API: [Image-Line MIDI scripting](https://www.image-line.com/fl-studio-learning-content/fl-studio-online-manual/html/midi_scripting.htm), [Live Song: tempo и transport](https://docs.cycling74.com/apiref/lom/song/).

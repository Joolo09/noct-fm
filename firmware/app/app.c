/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal */
/* CTL-1 on fm1-base: the app's unity build (compiled into fm1-base's, after the platform and its lib/touch.c) and
 * the glue between fm1-base's contract (app.h) and Ctl's parts: the settings (project.c), the engine (engine.c), the
 * UI (ui.c) and the GM synth (dsp/gm.c, its own -O2 unit). */
#include "project.c"
#include "midi_engine.c"
#include "midi_ui.c"

/* MIDI out: to the ports, and to the onboard synth (channel messages only: its queue is single-writer, and this
 * runs under the lock or in the tick; with Sound off no notes, but programs and controllers still set) */
static volatile uint8_t in_tick;
void ctl_send(uint32_t pkt, uint32_t to)
{
    uint32_t f = 0;
    if (!in_tick)
        f = plat_lock();
    if (to & (TO_USB | TO_BLE))
        plat_midi_send(pkt, to & (TO_USB | TO_BLE));
    if (!in_tick)
        plat_unlock(f);
}

/* ---- the contract */
void app_init(void) { project_load(); midi_engine_init(); }
static void silence(int32_t *lr,uint32_t n) { uint32_t i; for(i=0;i<2u*n;i++) lr[i]=0; }
void app_render(int32_t *lr,uint32_t n,uint32_t gain_q12) { (void)gain_q12; silence(lr,n); }
void app_quiet(int32_t *lr,uint32_t n) { silence(lr,n); }
void app_overload(void) {}
void app_panic(void) { engine_panic(); }
void app_tick(void)
{
    in_tick = 1;
    engine_tick();
    in_tick = 0;
}
void app_ui_init(void) { ui_init(); }
void app_frame(void) { ui_frame(); }
void app_input_only(void) { ui_input_only(); }
void app_say(const char *a, const char *b) { ui_say(a, b); }
void app_splash(void)
{
    lcd_fill(0, 0, 240, 240, K_BG);
    cv_begin(240, 120, K_BG);
    noct_sky(0);
    cv_text(24, 22, &FONT_L, "NOCT FM", K_TEXT);
    cv_text(24, 65, &FONT_XS, "AFTER HOURS / IN CONTROL", K_DIM);
    cv_rect(24, 90, 192, 2, K_HOT);
    cv_blit(0, 60);
}

#ifdef PLAT_HOST
#include "midi_host.c"                        /* the host simulator's part: checks and commands */
#endif

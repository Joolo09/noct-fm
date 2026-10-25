/* SPDX-License-Identifier: MPL-2.0
 * NOCT FM: deterministic pixel renderer, integer geometry, no heap allocation.
 * Included by ui.c. Each DMA strip fits the platform's existing canvas.
 */
static const char *const NOCT_BANK[4] = {"MIXER", "MACROS", "SENDS", "USER"};
static const char *const NOCT_LABEL[2][4][4] = {
    {{"LEVEL", "PAN", "WIDTH", "CH VOL"}, {"MAC 1", "MAC 2", "MAC 3", "MAC 4"},
     {"SEND 1", "SEND 2", "SEND 3", "SEND 4"}, {"USER 1", "USER 2", "USER 3", "USER 4"}},
    {{"LEVEL", "PAN", "SEND A", "SEND B"}, {"MAC 1", "MAC 2", "MAC 3", "MAC 4"},
     {"MAC 5", "MAC 6", "MAC 7", "MAC 8"}, {"USER 1", "USER 2", "USER 3", "USER 4"}}
};

/* Original 16x18 pixel interpretation of FL Studio's fruit mark.
 * 0 transparent, 1 orange, 2 lit edge, 3 green leaf. */
static const char NOCT_FRUIT[18][17] = {
    "0000000000033300", "0000000003333000", "0000000033300000",
    "0000333330000000", "0000033300000000", "0000111122000000",
    "0001111112200000", "0011111111220000", "0011111111120000",
    "0011111111110000", "0001111111110000", "0001111111100000",
    "0000111111100000", "0000111111000000", "0000011111000000",
    "0000001110000000", "0000001100000000", "0000001000000000"
};
static void noct_logo(int x, int y, int scale)
{
    int i, j;
    if (proj.rsv[0]) {
        /* Live's parallel vertical and horizontal bars, aligned to the pixel grid. */
        for (i = 0; i < 4; i++) {
            cv_rect(x + i * 3 * scale, y + 2 * scale, scale, 12 * scale, K_HOT);
            cv_rect(x + 13 * scale, y + (2 + i * 3) * scale, 8 * scale, scale, K_HOT);
        }
    } else {
        for (j = 0; j < 18; j++)
            for (i = 0; i < 16; i++) {
                char c = NOCT_FRUIT[j][i];
                if (c != '0') cv_rect(x + i * scale, y + j * scale, scale, scale,
                    c == '3' ? RGB(151, 214, 107) : c == '2' ? RGB(255, 211, 141) : K_HOT);
            }
    }
}
static void noct_sky(uint32_t tick)
{
    int i, x, y;
    for (i = 0; i < 34; i++) {
        x = (i * 83 + 17) % 240;
        y = (i * 41 + 11) % 72;
        cv_rect(x, y, 1 + (i % 9 == 0), 1,
                ((tick / 5 + (uint32_t)i) % 7 == 0) ? K_TEXT : K_LINE);
    }
    /* Low mountains and a slow perspective grid. */
    for (x = 0; x < 240; x += 3) {
        int a = (x + 27) % 110;
        y = 80 - (a < 55 ? a : 110 - a) / 3;
        cv_rect(x, y, 3, 95 - y, RGB(26, 26, 51));
    }
    cv_rect(0, 94, 240, 1, RGB(104, 85, 146));
    for (i = -4; i <= 4; i++) cv_line(120 + i * 15, 95, 120 + i * 56, 119, K_LINE);
    for (i = 0; i < 4; i++) {
        int t = (i * 6 + (int)(tick % 6)) % 24;
        cv_rect(0, 95 + t * t / 24, 240, 1, RGB(33, 34, 61));
    }
}
static void noct_center(int x, int y, const felucca_font_t *f, const char *s, uint16_t c)
{ cv_text(x - text_w(f, s) / 2, y, f, s, c); }

static void noct_draw(void)
{
    static uint32_t last_draw;
    uint32_t now = plat_ms(), tick = now / 80u;
    int k, d = proj.rsv[0], transition = now < noct_transition;
    char t[24];
    if (now - last_draw < 50u && ui.sig[0] == 0x4e4f4354u) return;
    last_draw = now;
    ui.sig[0] = 0x4e4f4354u;
    /* Four small strips: input is serviced between blits; no animation sleeps. */
    cv_begin(240, 24, K_BG);
    cv_text(8, 4, &FONT_B, "NOCT", K_TEXT);
    cv_text(48, 6, &FONT_XS, "FM", K_DIM);
    put_u(put_s(t, "CH "), proj.chan + 1u);
    cv_text(84, 6, &FONT_XS, t, K_DIM);
    cv_rect(139, 9, 4, 4, lit(PORT_USB, 1) || lit(PORT_USB, 0) ? K_HOT : K_LINE);
    cv_text(148, 6, &FONT_XS, "USB", K_DIM);
    cv_text(182, 6, &FONT_XS, ui.playing ? "PLAY" : "IDLE", ui.playing ? K_HOT : K_DIM);
    if (noct_recording) cv_rect(227, 8, 5, 5, RGB(255, 91, 115));
    cv_rect(8, 23, 224, 1, K_LINE);
    cv_blit(0, 0);
    plat_yield();

    cv_begin(240, 120, K_BG);
    noct_sky(tick);
    if (transition) {
        noct_logo(d ? 89 : 96, 6, 3);
        noct_center(120, 63, &FONT_B, d ? "ABLETON LIVE" : "FL STUDIO", K_TEXT);
        noct_center(120, 82, &FONT_XS, "LOADING YOUR ORBIT", K_DIM);
        cv_rect(48, 105, 144, 3, K_LINE);
        cv_rect(48, 105, (int)(650u - (noct_transition - now)) * 144 / 650, 3, K_HOT);
    } else {
        noct_logo(12, 19, 2);
        cv_text(d ? 65 : 54, 19, &FONT_B, d ? "ABLETON LIVE" : "FL STUDIO", K_TEXT);
        cv_text(d ? 65 : 54, 40, &FONT_XS, d ? "MINT / SESSION 02" : "AMBER / SESSION 01", K_HOT);
        cv_text(12, 66, &FONT_XS, ui.last[0] ? ui.last : "AFTER HOURS. MAKE SOMETHING.", K_DIM);
        /* MIDI activity spark trail. It visualizes messages, not an audio spectrum. */
        for (k = 0; k < 14; k++) {
            int h = ui.keys ? 2 + (int)((tick * 3 + (uint32_t)k * 7) % 14) : 2;
            cv_rect(174 + k * 4, 87 - h, 2, h, ui.keys ? K_HOT : K_LINE);
        }
        put_s(t, NOCT_BANK[proj.bank]);
        if (d && proj.bank == 2) put_s(t, "MACROS 5-8");
        cv_text(8, 103, &FONT_XS, t, K_TEXT);
        for (k = 0; k < 4; k++) cv_rect(200 + k * 8, 107, 4, 4, k == proj.bank ? K_HOT : K_LINE);
    }
    cv_blit(0, 24);
    plat_yield();

    cv_begin(240, 70, K_BG);
    for (k = 0; k < 4; k++) {
        int x = 8 + k * 58, v = proj.val[proj.bank][k];
        int hot = ui.touched == k && now < ui.touch_until;
        cv_rect(x, 2, 50, 64, K_PANEL);
        cv_rect(x, 2, 50, 1, hot ? K_HOT : K_LINE);
        noct_center(x + 25, 8, &FONT_XS,
            fx_cc() ? "CC EDIT" : NOCT_LABEL[d][proj.bank][k], hot ? K_HOT : K_DIM);
        put_u(t, fx_cc() ? proj.cc[proj.bank][k] : v);
        noct_center(x + 25, 25, &FONT_M, t, K_TEXT);
        cv_rect(x + 6, 57, 38, 3, K_LINE);
        cv_rect(x + 6, 57, 38 * v / 127, 3, K_HOT);
    }
    cv_blit(0, 144);
    plat_yield();

    cv_begin(240, 26, K_BG);
    cv_rect(8, 2, 224, 1, K_LINE);
    cv_text(8, 9, &FONT_XS, "PRESET: DAW", K_DIM);
    cv_text(104, 9, &FONT_XS, "SELECT: BANK", K_DIM);
    if (ui.dirty) cv_rect(228, 13, 3, 3, K_HOT);
    cv_blit(0, 214);
}

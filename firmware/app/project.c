/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal */
/* Ctl settings: one flash object (OBJ_PROJ). Settings of another format, or none, give the defaults.
 * Part of the unity build, after app.h. */
project_t proj;

/* the knob banks: A filter cutoff, resonance, modulation, volume; B attack, release, decay, vibrato;
 * C and D the general-purpose controllers 16-23 */
static const uint8_t CC_DEFAULT[NBANKS][NKNOBS] = {{20, 21, 22, 23}, {24, 25, 26, 27}, {28, 29, 30, 31}, {32, 33, 34, 35}};
/* the sequencer: General MIDI drums (kick, snare, closed hat, clap) */
static const uint8_t TRK_DEFAULT[NTRACKS][4] = {{16, 4, 0, 36}, {16, 2, 4, 38}, {16, 8, 0, 42}, {12, 3, 2, 39}};

/* hits spread as evenly as possible over steps (the Euclidean rhythm), turned right by rot */
uint16_t euclid(int steps, int hits, int rot)
{
    uint16_t p = 0;
    int i;
    if (steps < 1 || steps > 16)
        return 0;
    if (hits > steps)
        hits = steps;
    for (i = 0; i < steps; i++)
        if ((i * hits) % steps < hits)
            p |= (uint16_t)(1u << ((i + rot) % steps));
    return p;
}

void project_defaults(void)
{
    int b, k;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.rsv[0] = 2;
    proj.format = PROJ_FORMAT;
    proj.vel = 100;
    for (b = 0; b < NBANKS; b++)
        for (k = 0; k < NKNOBS; k++) {
            proj.cc[b][k] = (uint8_t)(20+k);
            proj.val[b][k] = CC_DEFAULT[b][k] == 7 ? 100 : CC_DEFAULT[b][k] == 1 ? 0 : 64;
        }
    proj.scale = 0;                              /* chromatic: the keys as a keyboard (root C for a scale) */
    proj.tempo = 120;
    proj.bpm10 = 1200;
    proj.rsv[5] = 8; proj.rsv[6] = 60; proj.rsv[7] = 1; proj.rsv[8] = 85;
    proj.gen_seed=978437u;
    proj.arp_gate=50;
    proj.custom_n=5;proj.custom[1]=4;proj.custom[2]=7;proj.custom[3]=11;proj.custom[4]=14;   /* MAJ9 until learned */
    proj.clock = CLK_INT_OUT; /* AUTO in the MIDI engine */
    proj.sound = SOUND_OFF;
    proj.master_cc = 7;
    proj.arp_rate = RATE_16;
    proj.arp_oct = 1;
    proj.seq_ch = 9;                             /* channel 10 */
    proj.glow = 1;
    proj.touch = 0;                              /* velocity from the keys */
    proj.at_mode = AT_OFF;                      /* and aftertouch, per note */
    for (k = 0; k < NTRACKS; k++) {
        track_t *t = &proj.trk[k];
        t->steps = TRK_DEFAULT[k][0];
        t->hits = TRK_DEFAULT[k][1];
        t->rot = TRK_DEFAULT[k][2];
        t->note = TRK_DEFAULT[k][3];
        t->pat = euclid(t->steps, t->hits, t->rot);
    }
}

static int project_valid(void)
{
    int b, k;
    if (proj.magic != PROJ_MAGIC || (proj.format != PROJ_FORMAT && proj.format != 2u && proj.format != 3u && proj.format != 4u && proj.format != 5u))
        return 0;
    if (proj.format >= 5u && proj.rsv[0] != 2u) return 0;
    if (proj.format >= 4u && (proj.bpm10<200u || proj.bpm10>9990u || proj.rsv[1]>1u ||
        (proj.format < 6u && (proj.rsv[2]>5u || proj.rsv[4]>2u)) || proj.rsv[5]>16u || proj.rsv[6]>115u ||
        proj.rsv[7]<1u || proj.rsv[7]>3u || proj.rsv[8]<10u || proj.rsv[8]>100u)) return 0;
    if (proj.format == 5u && (!proj.gen_seed || proj.gen_mode>2u || proj.gen_progression>3u)) return 0;
    if (proj.format == PROJ_FORMAT && (!proj.gen_seed || proj.gen_mode>=NGMODES || proj.gen_progression>NPROGS ||
        proj.rsv[2]>=NCHORDS || proj.rsv[4]>=NGSCALES || proj.gen_swing>=NSWING || proj.gen_style>=NSTYLES ||
        proj.gen_color>=NCOLORS || proj.gen_melody>=NMELODY || (proj.gen_var_lo|proj.gen_var_hi<<8)>=NVARS || proj.chord_voicing>=NVOICINGS || proj.chord_strum>=NSTRUMS || proj.key_scale>1u ||
        proj.arp_gate<10u || proj.arp_gate>100u || proj.custom_n<2u || proj.custom_n>5u)) return 0;
    if (proj.format == PROJ_FORMAT)
        for (k = 0; k < 5; k++) if (proj.custom[k] > 36u) return 0;
    if (proj.chan > 14u || proj.vel < 1u || proj.vel > 127u || proj.octave < -3 || proj.octave > 3 ||
        proj.program > 127u || proj.bank >= NBANKS || proj.root > 11u || proj.scale >= NSCALES ||
        proj.human > 40u || proj.tempo < 40u || proj.tempo > 240u || proj.clock >= NCLK || proj.arp_on > 1u ||
        proj.arp_mode >= NARP || proj.arp_rate >= NRATE || proj.arp_oct < 1u || proj.arp_oct > 3u ||
        proj.arp_latch > 1u || proj.seq_ch > 15u || proj.master_cc > 119u || proj.glow > 2u ||
        proj.sound >= NSOUND || proj.in_mute > 1u || proj.touch > 1u || proj.at_mode >= NAT ||
        proj.vel_curve < -5 || proj.vel_curve > 5 || proj.at_sens < -5 || proj.at_sens > 5)
        return 0;
    for (b = 0; b < NBANKS; b++)
        for (k = 0; k < NKNOBS; k++)
            if (proj.cc[b][k] > 119u || proj.val[b][k] > 127u)
                return 0;
    for (k = 0; k < NTRACKS; k++) {
        const track_t *t = &proj.trk[k];
        if (t->steps < 1u || t->steps > 16u || t->hits > t->steps || t->rot >= t->steps || t->note > 127u ||
            t->mute > 1u)
            return 0;
    }
    return 1;
}

int project_load(void)
{
    int n;
    memset(&proj, 0, sizeof proj);
    n = plat_store_load(OBJ_PROJ, &proj, sizeof proj);
    if (n >= 8 && project_valid()) {
        /* Migrate the old DAW/user-bank format before validating its fixed-route fields. */
        if (proj.format < 4u) {
            proj.bpm10=(uint16_t)(proj.tempo*10u);
            memset(proj.rsv,0,sizeof proj.rsv);
            proj.rsv[5]=8;proj.rsv[6]=60;proj.rsv[7]=1;proj.rsv[8]=50;
            proj.clock=CLK_INT_OUT;proj.touch=0;
        }
        if(proj.format<5u){proj.gen_seed=978437u;proj.gen_mode=0;proj.gen_progression=0;proj.rsv[8]=85;}
        if(proj.format<6u){
            /* 0.8 additions: the old sound stays identical (straight, held triads, close chords, no strum) */
            proj.gen_swing=proj.gen_style=proj.gen_color=proj.chord_voicing=proj.chord_strum=proj.key_scale=0;
            proj.arp_gate=50;proj.human=0;
            memset(proj.custom,0,sizeof proj.custom);memset(proj.rsv6,0,sizeof proj.rsv6);
            proj.gen_var_lo=proj.gen_var_hi=proj.gen_melody=0;
            proj.custom_n=5;proj.custom[1]=4;proj.custom[2]=7;proj.custom[3]=11;proj.custom[4]=14;
        }
        proj.format = PROJ_FORMAT;
        proj.chan = proj.bank = proj.scale = proj.program = 0;
        proj.sound = SOUND_OFF; proj.in_mute = 1; proj.at_mode = AT_OFF;
        proj.rsv[0] = 2;
        if(proj.rsv[3]>1u)proj.rsv[3]=0;
        /* Keyboard CHORDS is polyphonic and must not inherit a saved arp route. */
        if(proj.rsv[1])proj.arp_on=0;
        for (int b=0;b<NBANKS;b++) for(int k=0;k<NKNOBS;k++) proj.cc[b][k]=(uint8_t)(20+k);
        return 0;
    }
    project_defaults();
    return -1;
}

int project_save(void) { return plat_store_save(OBJ_PROJ, &proj, sizeof proj); }

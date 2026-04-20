declare name "MorphingResonator";
declare version "4.5";
declare author "Tombogo x Rheome";

import("stdfaust.lib");

// ================== UI CONTROLS ==================
use_mic = checkbox("1. Use Mic Input");
trigger = button("2. Pluck (If Mic Off)");

root_note = int(hslider("3. Root Note (MIDI)[36=C2]", 36, 24, 72, 1));
chord_idx = int(nentry("4. Chord Grid[style:radio{'i (min)':0; 'ii° (dim)':1; 'III (Maj)':2; 'iv (min)':3; 'v (min)':4; 'VI (Maj)':5; 'VII (Maj)':6; 'V7 (Dom)':7; 'i (m9)':8; 'iv (m9)':9; 'v (m9)':10; 'VI (M9)':11; 'III (M9)':12; 'VII (dom9)':13; 'i (m11)':14; 'i (sus4)':15}]", 0, 0, 15, 1));

glide = hslider("5. Chord Glide (s)", 0.5, 0.001, 10.0, 0.01);
timbre = hslider("6. Timbre Morph[0:Harm, 1:Inharm, 2:Metal, 3:Spectral Mask]", 0.0, 0.0, 3.0, 0.01) : si.smoo;

// Envelopes & Mix
decay = hslider("7. Decay Time (s)", 2.0, 0.1, 15.0, 0.01) : si.smoo;
drive = hslider("8. Harmonic Drive", 2.0, 1.0, 10.0, 0.1) : si.smoo;

// --- GAIN STAGING ---
input_trim = hslider("9. Input Trim", 0.25, 0.0, 1.0, 0.01) : si.smoo;
wet_trim = hslider("10. Resonator Trim", 0.4, 0.0, 2.0, 0.01) : si.smoo;
mix = hslider("11. Dry/Wet Mix", 1.0, 0.0, 1.0, 0.01) : si.smoo;
gain = hslider("12. Output Gain", 1.0, 0.0, 4.0, 0.01) : si.smoo;

// ================== CORE LOGIC ==================
slew = si.smooth(ba.tau2pole(glide));
base_freq = ba.midikey2hz(root_note) : slew;
saturate(x) = ma.tanh(x * drive);

decay_scaler = 1.0 / sqrt(max(1.0, decay));

// ================== 5-VOICE CHORD DICTIONARY ==================
c0(v) = (0, 3, 7, 12, 19 : ba.selectn(5, v)); 
c1(v) = (2, 5, 8, 14, 20 : ba.selectn(5, v)); 
c2(v) = (3, 7, 10, 15, 22: ba.selectn(5, v)); 
c3(v) = (5, 8, 12, 17, 24: ba.selectn(5, v)); 
c4(v) = (7, 10, 14, 19, 26: ba.selectn(5, v));
c5(v) = (8, 12, 15, 20, 27: ba.selectn(5, v)); 
c6(v) = (10, 14, 17, 22, 29: ba.selectn(5, v)); 
c7(v) = (7, 11, 14, 17, 23: ba.selectn(5, v)); 

c8(v) = (0, 3, 7, 10, 14 : ba.selectn(5, v)); 
c9(v) = (5, 8, 12, 15, 19: ba.selectn(5, v)); 
c10(v)= (7, 10, 14, 17, 20: ba.selectn(5, v)); 
c11(v)= (8, 12, 15, 19, 22: ba.selectn(5, v)); 
c12(v)= (3, 7, 10, 14, 17: ba.selectn(5, v)); 
c13(v)= (10, 14, 17, 20, 24: ba.selectn(5, v)); 
c14(v)= (0, 3, 7, 10, 17 : ba.selectn(5, v)); 
c15(v)= (0, 5, 7, 12, 17 : ba.selectn(5, v)); 

c_offset(v) = c0(v), c1(v), c2(v), c3(v), c4(v), c5(v), c6(v), c7(v), 
              c8(v), c9(v), c10(v), c11(v), c12(v), c13(v), c14(v), c15(v) : ba.selectn(16, chord_idx);

// ================== ENGINE A: CHORD RESONATOR (30 Bands) ==================
nVoices = 5;
nHarmonics = 6;

// voicingIdx (0–4): selects which voice gets emphasis for Mode B arpeggiation
// voicingIdx=0 → voice 0 (root) gets gain 2.0, others 0.6
// voicingIdx=1 → voice 1 gets gain 1.8, others 0.6
// voicingIdx=2 → voice 2 gets gain 1.8, others 0.6
// voicingIdx=3 → voice 3 gets gain 1.5, others 0.6
// voicingIdx=4 → voice 4 gets gain 1.5, others 0.6
voicingIdx = int(nentry("13. Voicing Index", 0, 0, 4, 1));

// Returns per-voice gain multiplier based on voicingIdx and voice index v.
// select2(cond, a, b) = a if cond!=0, else b.
// 5-step cascading select2 tree — one step per voicingIdx value.
voicing_curve(v) =
    // Step 0: is voicingIdx==0?
    select2((voicingIdx==0),
        // Yes: gain = 2.0 for voice 0, 0.05 for voices 1-4
        select2((v==0), 2.0, 0.05),
        // No, check next: is voicingIdx==1?
        select2((voicingIdx==1),
            // Yes: gain = 1.8 for voice 1, 0.05 for others
            select2((v==1), 1.8, 0.05),
            // No, check next: is voicingIdx==2?
            select2((voicingIdx==2),
                // Yes: gain = 1.8 for voice 2, 0.05 for others
                select2((v==2), 1.8, 0.05),
                // No, check next: is voicingIdx==3?
                select2((voicingIdx==3),
                    // Yes: gain = 1.5 for voice 3, 0.05 for others
                    select2((v==3), 1.5, 0.05),
                    // No, must be voicingIdx==4: gain = 1.5 for voice 4, 0.05 for others
                    select2((v==4), 1.5, 0.05)
                )
            )
        )
    )
    : si.smoo;

chord_freq(v) = base_freq * ba.semi2ratio(c_offset(v) : slew) * voicing_curve(v);

tc = max(0.0, min(2.0, timbre)); 
w0 = max(0.0, 1.0 - abs(tc - 0.0)); 
w1 = max(0.0, 1.0 - abs(tc - 1.0)); 
w2 = max(0.0, 1.0 - abs(tc - 2.0)); 

harm(f, n) = f * (n + 1);
inharm(f, n) = f * 0.44444 * pow((n + 1) + 0.5, 2);
fixed(f, n) = ((62, 115, 218, 411, 777, 1500) : ba.selectn(6, n)) * (f / 440.0);

mode_A(f, n) = (harm(f,n)*w0 + inharm(f,n)*w1 + fixed(f,n)*w2) : si.smoo;
voice_A(f) = _ <: par(i, nHarmonics, pm.modeFilter(mode_A(f, i), decay, 1.0)) :> _;
engine_A = _ <: par(v, nVoices, voice_A(chord_freq(v))) :> * (0.025 * decay_scaler); 

// ================== ENGINE B: 5-OCTAVE SPECTRAL MASK (20 Bands) ==================
nFiltersB = 20;
freq_B(i) = base_freq * ba.semi2ratio( (c_offset(int(i % 5)) + ((int(i / 5) - 1) * 12)) : slew );
engine_B = _ <: par(i, nFiltersB, pm.modeFilter(freq_B(i), decay, 1.0)) :> * (0.03 * decay_scaler);

// ================== CROSSFADE & ROUTING ==================
gain_A = max(0.0, min(1.0, 3.0 - timbre)) : si.smoo;
gain_B = max(0.0, min(1.0, timbre - 2.0)) : si.smoo;

// The Sine/Cosine Equal Power Math
dry_mix = cos(mix * ma.PI / 2.0);
wet_mix = sin(mix * ma.PI / 2.0);

// input_trim attenuates before drive/saturate to prevent distortion from hot sources (looper)
process_channel(x) =
    (x * dry_mix) +
    (((x * input_trim) : saturate <: (engine_A * gain_A), (engine_B * gain_B) :> ma.tanh) * wet_trim * wet_mix)
    : * (gain)
    : max(-1.0) : min(1.0);

// ================== MAIN EXECUTABLE ==================
exciter = no.noise * en.ar(0.001, 0.05, trigger);
inL_routed(inL) = select2(use_mic, exciter, inL);
inR_routed(inR) = select2(use_mic, 0.0, inR);

// When use_mic=0, exciter bypasses the dry/wet mix chain and goes straight
// into the reverb at half amplitude — bypasses the dry_mix=0 zeroing at mix=1.0.
exciter_to_reverb = select2(use_mic, exciter*0.5, 0.0);

process(inL, inR)
  = process_channel(inL_routed(inL)) + exciter_to_reverb,
    process_channel(inR_routed(inR)) + exciter_to_reverb
  : dm.zita_rev1;
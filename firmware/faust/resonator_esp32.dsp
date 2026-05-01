declare name "MorphingResonatorESP32";
declare version "1.0";
declare author "Tombogo x Rheome";

// ESP32-S3 variant of resonatorTest2.dsp:
//   - Mono in / mono out (was stereo)
//   - 5 voices x 3 harmonics = 15 mode filters (was 30)
//   - Engine B (spectral mask, 20 filters) removed entirely
//   - dm.zita_rev1 replaced with re.mono_freeverb (~10x lighter)
//   - All UI controls preserved with same names so the wrapper API can use them

import("stdfaust.lib");

// ================== UI CONTROLS ==================
root_note  = int(hslider("3. Root Note (MIDI)[36=C2]", 36, 24, 72, 1));
chord_idx  = int(nentry("4. Chord Grid", 0, 0, 15, 1));

glide  = hslider("5. Chord Glide (s)", 0.5, 0.001, 10.0, 0.01);
timbre = hslider("6. Timbre Morph", 0.0, 0.0, 2.0, 0.01) : si.smoo;

decay  = hslider("7. Decay Time (s)", 2.0, 0.1, 15.0, 0.01) : si.smoo;
drive  = hslider("8. Harmonic Drive", 2.0, 1.0, 10.0, 0.1)  : si.smoo;

input_trim = hslider("9. Input Trim", 0.25, 0.0, 1.0, 0.01) : si.smoo;
wet_trim   = hslider("10. Resonator Trim", 0.4, 0.0, 2.0, 0.01) : si.smoo;
mix        = hslider("11. Dry/Wet Mix", 0.0, 0.0, 1.0, 0.01) : si.smoo;
gain       = hslider("12. Output Gain", 1.0, 0.0, 4.0, 0.01) : si.smoo;

// ================== CORE LOGIC ==================
slew = si.smooth(ba.tau2pole(glide));
base_freq = ba.midikey2hz(root_note) : slew;
saturate(x) = ma.tanh(x * drive);
decay_scaler = 1.0 / sqrt(max(1.0, decay));

// ================== 5-VOICE CHORD DICTIONARY ==================
c0(v)  = (0, 3, 7, 12, 19  : ba.selectn(5, v));
c1(v)  = (2, 5, 8, 14, 20  : ba.selectn(5, v));
c2(v)  = (3, 7, 10, 15, 22 : ba.selectn(5, v));
c3(v)  = (5, 8, 12, 17, 24 : ba.selectn(5, v));
c4(v)  = (7, 10, 14, 19, 26: ba.selectn(5, v));
c5(v)  = (8, 12, 15, 20, 27: ba.selectn(5, v));
c6(v)  = (10, 14, 17, 22, 29:ba.selectn(5, v));
c7(v)  = (7, 11, 14, 17, 23: ba.selectn(5, v));
c8(v)  = (0, 3, 7, 10, 14  : ba.selectn(5, v));
c9(v)  = (5, 8, 12, 15, 19 : ba.selectn(5, v));
c10(v) = (7, 10, 14, 17, 20: ba.selectn(5, v));
c11(v) = (8, 12, 15, 19, 22: ba.selectn(5, v));
c12(v) = (3, 7, 10, 14, 17 : ba.selectn(5, v));
c13(v) = (10, 14, 17, 20, 24:ba.selectn(5, v));
c14(v) = (0, 3, 7, 10, 17  : ba.selectn(5, v));
c15(v) = (0, 5, 7, 12, 17  : ba.selectn(5, v));

c_offset(v) = c0(v),  c1(v),  c2(v),  c3(v),  c4(v),  c5(v),  c6(v),  c7(v),
              c8(v),  c9(v),  c10(v), c11(v), c12(v), c13(v), c14(v), c15(v)
              : ba.selectn(16, chord_idx);

// ================== ENGINE A: CHORD RESONATOR (15 Bands) ==================
nVoices    = 5;
nHarmonics = 3;

voicingIdx = int(nentry("13. Voicing Index", 0, 0, 4, 1));

voicing_curve(v) =
    select2((voicingIdx==0),
        select2((v==0), 2.0, 0.05),
        select2((voicingIdx==1),
            select2((v==1), 1.8, 0.05),
            select2((voicingIdx==2),
                select2((v==2), 1.8, 0.05),
                select2((voicingIdx==3),
                    select2((v==3), 1.5, 0.05),
                    select2((v==4), 1.5, 0.05)
                )
            )
        )
    ) : si.smoo;

chord_freq(v) = base_freq * ba.semi2ratio(c_offset(v) : slew) * voicing_curve(v);

tc = max(0.0, min(2.0, timbre));
w0 = max(0.0, 1.0 - abs(tc - 0.0));
w1 = max(0.0, 1.0 - abs(tc - 1.0));
w2 = max(0.0, 1.0 - abs(tc - 2.0));

harm(f, n)   = f * (n + 1);
inharm(f, n) = f * 0.44444 * pow((n + 1) + 0.5, 2);
fixed(f, n)  = ((62, 115, 218) : ba.selectn(3, n)) * (f / 440.0);

mode_A(f, n)  = (harm(f,n)*w0 + inharm(f,n)*w1 + fixed(f,n)*w2) : si.smoo;
voice_A(f)    = _ <: par(i, nHarmonics, pm.modeFilter(mode_A(f, i), decay, 1.0)) :> _;
engine_A      = _ <: par(v, nVoices, voice_A(chord_freq(v))) :> *(0.025 * decay_scaler);

// ================== CROSSFADE & ROUTING ==================
dry_mix = cos(mix * ma.PI / 2.0);
wet_mix = sin(mix * ma.PI / 2.0);

process_channel(x) =
    (x * dry_mix) +
    (((x * input_trim) : saturate : engine_A) * wet_trim * wet_mix)
    : *(gain)
    : max(-1.0) : min(1.0);

// ================== MAIN EXECUTABLE (mono) ==================
process = process_channel : re.mono_freeverb(0.5, 0.5, 0.5, 0);

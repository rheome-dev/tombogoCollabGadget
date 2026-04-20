declare name "GadgetResonatorMinorChords";
declare version "1.6";
declare author "Tombogo x Rheome";
declare license "MIT";

import("stdfaust.lib");

// ============== PARAMETERS ==============
use_mic = checkbox("1. Use Mic Input");
trigger = button("2. Pluck (If Mic Off)");

// The new Radio Button "Grid" selector for the 8 minor scale chords
chord_idx = int(nentry("3. Minor Scale Chords[style:radio{'i (min)':0; 'ii° (dim)':1; 'III (Maj)':2; 'iv (min)':3; 'v (min)':4; 'VI (Maj)':5; 'VII (Maj)':6; 'VIII (min)':7}]", 0, 0, 7, 1));

decay = hslider("decay", 2.0, 0.1, 10.0, 0.01) : si.smoo;
gain = hslider("gain", 0.5, 0, 1, 0.01) : si.smoo;
base_freq = hslider("freq", 440, 100, 2000, 1) : si.smoo;
input_gain = hslider("input_gain", 0.5, 0, 1, 0.01) : si.smoo;

timbre = int(hslider("timbre", 0, 0, 2, 1));

// ============== EXCITER ==============
exciter = no.noise * en.ar(0.001, 0.05, trigger);

// ============== DIATONIC MINOR CHORD GENERATION ==============
nVoices = 3;

// Semitone offsets for the 8 diatonic triads of a Natural Minor scale
c0(v) = (0,  3,  7  : ba.selectn(nVoices, v)); // i   (Root, minor 3rd, Perfect 5th)
c1(v) = (2,  5,  8  : ba.selectn(nVoices, v)); // ii° (Major 2nd, Perfect 4th, minor 6th)
c2(v) = (3,  7, 10  : ba.selectn(nVoices, v)); // III (minor 3rd, Perfect 5th, minor 7th)
c3(v) = (5,  8, 12  : ba.selectn(nVoices, v)); // iv  (Perfect 4th, minor 6th, Octave)
c4(v) = (7, 10, 14  : ba.selectn(nVoices, v)); // v   (Perfect 5th, minor 7th, Major 9th)
c5(v) = (8, 12, 15  : ba.selectn(nVoices, v)); // VI  (minor 6th, Octave, minor 10th)
c6(v) = (10, 14, 17 : ba.selectn(nVoices, v)); // VII (minor 7th, Major 9th, Perfect 11th)
c7(v) = (12, 15, 19 : ba.selectn(nVoices, v)); // VIII(Octave i)

// Select the current chord based on the radio button clicked
current_offset(v) = c0(v), c1(v), c2(v), c3(v), c4(v), c5(v), c6(v), c7(v) : ba.selectn(8, chord_idx);

voice_freq(v) = base_freq * ba.semi2ratio(current_offset(v));

// ============== HARMONIC GENERATION ==============
nHarmonics = 6;
bar_factor = 0.44444;

harmonic_freq(f, n) = f * (n + 1);
inharmonic_freq(f, n) = f * bar_factor * pow((n + 1) + 0.5, 2);
fixed_freq(f, n) = ((62, 115, 218, 411, 777, 1500) : ba.selectn(nHarmonics, n)) * (f / base_freq);

mode_freq(f, n) = harmonic_freq(f, n), inharmonic_freq(f, n), fixed_freq(f, n) : ba.selectn(3, timbre);

// ============== RESONATOR ==============
resonator_mode(f, n) = pm.modeFilter(mode_freq(f, n), decay, gain);

voice(f) = _ <: par(i, nHarmonics, resonator_mode(f, i)) :> * (1.0 / float(nHarmonics));

// ============== MAIN PROCESS ==============
process(inL, inR) = select2(use_mic, exciter, (inL + inR) / 2.0)
    : fi.dcblocker
    : fi.lowpass(1, 8000)
    : * (input_gain)
    <: par(v, nVoices, voice(voice_freq(v)))
    :> fi.dcblocker
    : * (gain / float(nVoices))
    : max(-1.0) : min(1.0) 
    <: _, _
    : dm.zita_rev1;
/* ------------------------------------------------------------
author: "Tombogo x Rheome"
name: "MorphingResonatorESP32"
version: "1.0"
Code generated with Faust 2.85.5 (https://faust.grame.fr)
Compilation options: -lang cpp -fpga-mem-th 4 -nvi -ct 1 -cn FaustResonator -es 1 -mcd 16 -mdd 1024 -mdy 33 -uim -single -ftz 0
------------------------------------------------------------ */

#ifndef  __FaustResonator_H__
#define  __FaustResonator_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include "faust_arch.h"  // Provides minimal dsp / UI / Meta stubs

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS FaustResonator
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

static float FaustResonator_faustpower2_f(float value) {
	return value * value;
}

struct FaustResonator final : public dsp {
	
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider0;
	float fConst2;
	float fRec0[2];
	FAUSTFLOAT fHslider1;
	float fRec1[2];
	FAUSTFLOAT fHslider2;
	float fRec2[2];
	FAUSTFLOAT fHslider3;
	float fRec3[2];
	FAUSTFLOAT fHslider4;
	float fRec5[2];
	FAUSTFLOAT fHslider5;
	float fRec6[2];
	float fConst3;
	FAUSTFLOAT fHslider6;
	float fRec7[2];
	float fConst4;
	FAUSTFLOAT fHslider7;
	FAUSTFLOAT fHslider8;
	float fRec9[2];
	FAUSTFLOAT fEntry0;
	float fRec10[2];
	FAUSTFLOAT fHslider9;
	float fRec11[2];
	FAUSTFLOAT fEntry1;
	float fRec12[2];
	float fRec8[2];
	float fRec4[3];
	float fRec14[2];
	float fRec13[3];
	float fRec16[2];
	float fRec15[3];
	float fRec19[2];
	float fRec20[2];
	float fRec18[2];
	float fRec17[3];
	float fRec22[2];
	float fRec21[3];
	float fRec24[2];
	float fRec23[3];
	float fRec27[2];
	float fRec28[2];
	float fRec26[2];
	float fRec25[3];
	float fRec30[2];
	float fRec29[3];
	float fRec32[2];
	float fRec31[3];
	float fRec35[2];
	float fRec36[2];
	float fRec34[2];
	float fRec33[3];
	float fRec38[2];
	float fRec37[3];
	float fRec40[2];
	float fRec39[3];
	float fRec43[2];
	float fRec44[2];
	float fRec42[2];
	float fRec41[3];
	float fRec46[2];
	float fRec45[3];
	float fRec48[2];
	float fRec47[3];
	float fRec58[2];
	int IOTA0;
	float fVec0[8192];
	int iConst5;
	float fRec57[2];
	float fRec60[2];
	float fVec1[8192];
	int iConst6;
	float fRec59[2];
	float fRec62[2];
	float fVec2[8192];
	int iConst7;
	float fRec61[2];
	float fRec64[2];
	float fVec3[8192];
	int iConst8;
	float fRec63[2];
	float fRec66[2];
	float fVec4[8192];
	int iConst9;
	float fRec65[2];
	float fRec68[2];
	float fVec5[8192];
	int iConst10;
	float fRec67[2];
	float fRec70[2];
	float fVec6[8192];
	int iConst11;
	float fRec69[2];
	float fRec72[2];
	float fVec7[8192];
	int iConst12;
	float fRec71[2];
	float fVec8[2048];
	int iConst13;
	float fRec55[2];
	float fVec9[2048];
	int iConst14;
	float fRec53[2];
	float fVec10[2048];
	int iConst15;
	float fRec51[2];
	float fVec11[1024];
	int iConst16;
	float fRec49[2];
	
	FaustResonator() {
	}
	
	FaustResonator(const FaustResonator&) = default;
	
	virtual ~FaustResonator() = default;
	
	FaustResonator& operator=(const FaustResonator&) = default;
	
	void metadata(Meta* m) { 
		m->declare("author", "Tombogo x Rheome");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -fpga-mem-th 4 -nvi -ct 1 -cn FaustResonator -es 1 -mcd 16 -mdd 1024 -mdy 33 -uim -single -ftz 0");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("filename", "resonator_esp32.dsp");
		m->declare("filters.lib/allpass_comb:author", "Julius O. Smith III");
		m->declare("filters.lib/allpass_comb:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/allpass_comb:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.7.1");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "MorphingResonatorESP32");
		m->declare("physmodels.lib/name", "Faust Physical Models Library");
		m->declare("physmodels.lib/version", "1.2.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("reverbs.lib/mono_freeverb:author", "Romain Michon");
		m->declare("reverbs.lib/name", "Faust Reverb Library");
		m->declare("reverbs.lib/version", "1.5.1");
		m->declare("signals.lib/name", "Faust Routing Library");
		m->declare("signals.lib/version", "1.6.0");
		m->declare("version", "1.0");
	}

	static constexpr int getStaticNumInputs() {
		return 1;
	}
	static constexpr int getStaticNumOutputs() {
		return 1;
	}
	int getNumInputs() {
		return 1;
	}
	int getNumOutputs() {
		return 1;
	}
	
	static void classInit(int sample_rate) {
	}
	
	void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 44.1f / fConst0;
		fConst2 = 1.0f - fConst1;
		fConst3 = 1.0f / fConst0;
		fConst4 = 6.2831855f / fConst0;
		iConst5 = std::max<int>(0, static_cast<int>(0.036666665f * fConst0) + -1);
		iConst6 = std::max<int>(0, static_cast<int>(0.035306122f * fConst0) + -1);
		iConst7 = std::max<int>(0, static_cast<int>(0.033809524f * fConst0) + -1);
		iConst8 = std::max<int>(0, static_cast<int>(0.0322449f * fConst0) + -1);
		iConst9 = std::max<int>(0, static_cast<int>(0.030748298f * fConst0) + -1);
		iConst10 = std::max<int>(0, static_cast<int>(0.028956916f * fConst0) + -1);
		iConst11 = std::max<int>(0, static_cast<int>(0.026938776f * fConst0) + -1);
		iConst12 = std::max<int>(0, static_cast<int>(0.025306122f * fConst0) + -1);
		iConst13 = std::min<int>(1024, std::max<int>(0, static_cast<int>(0.0126077095f * fConst0) + -1));
		iConst14 = std::min<int>(1024, std::max<int>(0, static_cast<int>(0.01f * fConst0) + -1));
		iConst15 = std::min<int>(1024, std::max<int>(0, static_cast<int>(0.0077324263f * fConst0) + -1));
		iConst16 = std::min<int>(1024, std::max<int>(0, static_cast<int>(0.0051020407f * fConst0) + -1));
	}
	
	void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider1 = static_cast<FAUSTFLOAT>(1.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.4f);
		fHslider4 = static_cast<FAUSTFLOAT>(0.1f);
		fHslider5 = static_cast<FAUSTFLOAT>(1.0f);
		fHslider6 = static_cast<FAUSTFLOAT>(2.0f);
		fHslider7 = static_cast<FAUSTFLOAT>(36.0f);
		fHslider8 = static_cast<FAUSTFLOAT>(0.5f);
		fEntry0 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider9 = static_cast<FAUSTFLOAT>(0.0f);
		fEntry1 = static_cast<FAUSTFLOAT>(0.0f);
	}
	
	void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			fRec0[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec1[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec2[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec3[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fRec5[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec6[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec7[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec9[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec10[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec11[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec12[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec8[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 3; l12 = l12 + 1) {
			fRec4[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = l13 + 1) {
			fRec14[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 3; l14 = l14 + 1) {
			fRec13[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec16[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 3; l16 = l16 + 1) {
			fRec15[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec19[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = l18 + 1) {
			fRec20[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = l19 + 1) {
			fRec18[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 3; l20 = l20 + 1) {
			fRec17[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 2; l21 = l21 + 1) {
			fRec22[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 3; l22 = l22 + 1) {
			fRec21[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 2; l23 = l23 + 1) {
			fRec24[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 3; l24 = l24 + 1) {
			fRec23[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 2; l25 = l25 + 1) {
			fRec27[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 2; l26 = l26 + 1) {
			fRec28[l26] = 0.0f;
		}
		for (int l27 = 0; l27 < 2; l27 = l27 + 1) {
			fRec26[l27] = 0.0f;
		}
		for (int l28 = 0; l28 < 3; l28 = l28 + 1) {
			fRec25[l28] = 0.0f;
		}
		for (int l29 = 0; l29 < 2; l29 = l29 + 1) {
			fRec30[l29] = 0.0f;
		}
		for (int l30 = 0; l30 < 3; l30 = l30 + 1) {
			fRec29[l30] = 0.0f;
		}
		for (int l31 = 0; l31 < 2; l31 = l31 + 1) {
			fRec32[l31] = 0.0f;
		}
		for (int l32 = 0; l32 < 3; l32 = l32 + 1) {
			fRec31[l32] = 0.0f;
		}
		for (int l33 = 0; l33 < 2; l33 = l33 + 1) {
			fRec35[l33] = 0.0f;
		}
		for (int l34 = 0; l34 < 2; l34 = l34 + 1) {
			fRec36[l34] = 0.0f;
		}
		for (int l35 = 0; l35 < 2; l35 = l35 + 1) {
			fRec34[l35] = 0.0f;
		}
		for (int l36 = 0; l36 < 3; l36 = l36 + 1) {
			fRec33[l36] = 0.0f;
		}
		for (int l37 = 0; l37 < 2; l37 = l37 + 1) {
			fRec38[l37] = 0.0f;
		}
		for (int l38 = 0; l38 < 3; l38 = l38 + 1) {
			fRec37[l38] = 0.0f;
		}
		for (int l39 = 0; l39 < 2; l39 = l39 + 1) {
			fRec40[l39] = 0.0f;
		}
		for (int l40 = 0; l40 < 3; l40 = l40 + 1) {
			fRec39[l40] = 0.0f;
		}
		for (int l41 = 0; l41 < 2; l41 = l41 + 1) {
			fRec43[l41] = 0.0f;
		}
		for (int l42 = 0; l42 < 2; l42 = l42 + 1) {
			fRec44[l42] = 0.0f;
		}
		for (int l43 = 0; l43 < 2; l43 = l43 + 1) {
			fRec42[l43] = 0.0f;
		}
		for (int l44 = 0; l44 < 3; l44 = l44 + 1) {
			fRec41[l44] = 0.0f;
		}
		for (int l45 = 0; l45 < 2; l45 = l45 + 1) {
			fRec46[l45] = 0.0f;
		}
		for (int l46 = 0; l46 < 3; l46 = l46 + 1) {
			fRec45[l46] = 0.0f;
		}
		for (int l47 = 0; l47 < 2; l47 = l47 + 1) {
			fRec48[l47] = 0.0f;
		}
		for (int l48 = 0; l48 < 3; l48 = l48 + 1) {
			fRec47[l48] = 0.0f;
		}
		for (int l49 = 0; l49 < 2; l49 = l49 + 1) {
			fRec58[l49] = 0.0f;
		}
		IOTA0 = 0;
		for (int l50 = 0; l50 < 8192; l50 = l50 + 1) {
			fVec0[l50] = 0.0f;
		}
		for (int l51 = 0; l51 < 2; l51 = l51 + 1) {
			fRec57[l51] = 0.0f;
		}
		for (int l52 = 0; l52 < 2; l52 = l52 + 1) {
			fRec60[l52] = 0.0f;
		}
		for (int l53 = 0; l53 < 8192; l53 = l53 + 1) {
			fVec1[l53] = 0.0f;
		}
		for (int l54 = 0; l54 < 2; l54 = l54 + 1) {
			fRec59[l54] = 0.0f;
		}
		for (int l55 = 0; l55 < 2; l55 = l55 + 1) {
			fRec62[l55] = 0.0f;
		}
		for (int l56 = 0; l56 < 8192; l56 = l56 + 1) {
			fVec2[l56] = 0.0f;
		}
		for (int l57 = 0; l57 < 2; l57 = l57 + 1) {
			fRec61[l57] = 0.0f;
		}
		for (int l58 = 0; l58 < 2; l58 = l58 + 1) {
			fRec64[l58] = 0.0f;
		}
		for (int l59 = 0; l59 < 8192; l59 = l59 + 1) {
			fVec3[l59] = 0.0f;
		}
		for (int l60 = 0; l60 < 2; l60 = l60 + 1) {
			fRec63[l60] = 0.0f;
		}
		for (int l61 = 0; l61 < 2; l61 = l61 + 1) {
			fRec66[l61] = 0.0f;
		}
		for (int l62 = 0; l62 < 8192; l62 = l62 + 1) {
			fVec4[l62] = 0.0f;
		}
		for (int l63 = 0; l63 < 2; l63 = l63 + 1) {
			fRec65[l63] = 0.0f;
		}
		for (int l64 = 0; l64 < 2; l64 = l64 + 1) {
			fRec68[l64] = 0.0f;
		}
		for (int l65 = 0; l65 < 8192; l65 = l65 + 1) {
			fVec5[l65] = 0.0f;
		}
		for (int l66 = 0; l66 < 2; l66 = l66 + 1) {
			fRec67[l66] = 0.0f;
		}
		for (int l67 = 0; l67 < 2; l67 = l67 + 1) {
			fRec70[l67] = 0.0f;
		}
		for (int l68 = 0; l68 < 8192; l68 = l68 + 1) {
			fVec6[l68] = 0.0f;
		}
		for (int l69 = 0; l69 < 2; l69 = l69 + 1) {
			fRec69[l69] = 0.0f;
		}
		for (int l70 = 0; l70 < 2; l70 = l70 + 1) {
			fRec72[l70] = 0.0f;
		}
		for (int l71 = 0; l71 < 8192; l71 = l71 + 1) {
			fVec7[l71] = 0.0f;
		}
		for (int l72 = 0; l72 < 2; l72 = l72 + 1) {
			fRec71[l72] = 0.0f;
		}
		for (int l73 = 0; l73 < 2048; l73 = l73 + 1) {
			fVec8[l73] = 0.0f;
		}
		for (int l74 = 0; l74 < 2; l74 = l74 + 1) {
			fRec55[l74] = 0.0f;
		}
		for (int l75 = 0; l75 < 2048; l75 = l75 + 1) {
			fVec9[l75] = 0.0f;
		}
		for (int l76 = 0; l76 < 2; l76 = l76 + 1) {
			fRec53[l76] = 0.0f;
		}
		for (int l77 = 0; l77 < 2048; l77 = l77 + 1) {
			fVec10[l77] = 0.0f;
		}
		for (int l78 = 0; l78 < 2; l78 = l78 + 1) {
			fRec51[l78] = 0.0f;
		}
		for (int l79 = 0; l79 < 1024; l79 = l79 + 1) {
			fVec11[l79] = 0.0f;
		}
		for (int l80 = 0; l80 < 2; l80 = l80 + 1) {
			fRec49[l80] = 0.0f;
		}
	}
	
	void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	
	void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	FaustResonator* clone() {
		return new FaustResonator(*this);
	}
	
	int getSampleRate() {
		return fSampleRate;
	}
	
	void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("MorphingResonatorESP32");
		ui_interface->addHorizontalSlider("10. Resonator Trim", &fHslider3, FAUSTFLOAT(0.4f), FAUSTFLOAT(0.0f), FAUSTFLOAT(2.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("11. Dry/Wet Mix", &fHslider2, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("12. Output Gain", &fHslider1, FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(4.0f), FAUSTFLOAT(0.01f));
		ui_interface->addNumEntry("13. Voicing Index", &fEntry0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(4.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("14. Reverb Mix", &fHslider0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.01f));
		ui_interface->declare(&fHslider7, "36=C2", "");
		ui_interface->addHorizontalSlider("3. Root Note (MIDI)", &fHslider7, FAUSTFLOAT(36.0f), FAUSTFLOAT(24.0f), FAUSTFLOAT(72.0f), FAUSTFLOAT(1.0f));
		ui_interface->addNumEntry("4. Chord Grid", &fEntry1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(15.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("5. Chord Glide (s)", &fHslider8, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.001f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("6. Timbre Morph", &fHslider9, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(2.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("7. Decay Time (s)", &fHslider6, FAUSTFLOAT(2.0f), FAUSTFLOAT(0.1f), FAUSTFLOAT(15.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("8. Harmonic Drive", &fHslider5, FAUSTFLOAT(1.0f), FAUSTFLOAT(0.1f), FAUSTFLOAT(4.0f), FAUSTFLOAT(0.1f));
		ui_interface->addHorizontalSlider("9. Input Trim", &fHslider4, FAUSTFLOAT(0.1f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.01f));
		ui_interface->closeBox();
	}
	
	void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = fConst1 * static_cast<float>(fHslider0);
		float fSlow1 = fConst1 * static_cast<float>(fHslider1);
		float fSlow2 = fConst1 * static_cast<float>(fHslider2);
		float fSlow3 = fConst1 * static_cast<float>(fHslider3);
		float fSlow4 = fConst1 * static_cast<float>(fHslider4);
		float fSlow5 = fConst1 * static_cast<float>(fHslider5);
		float fSlow6 = fConst1 * static_cast<float>(fHslider6);
		float fSlow7 = static_cast<float>(fHslider8);
		int iSlow8 = std::fabs(fSlow7) < 1.1920929e-07f;
		float fSlow9 = ((iSlow8) ? 0.0f : std::exp(-(fConst3 / ((iSlow8) ? 1.0f : fSlow7))));
		float fSlow10 = 1.0f - fSlow9;
		float fSlow11 = 4.4e+02f * std::pow(2.0f, 0.083333336f * (static_cast<float>(static_cast<int>(static_cast<float>(fHslider7))) + -69.0f)) * fSlow10;
		int iSlow12 = static_cast<int>(static_cast<float>(fEntry0));
		int iSlow13 = iSlow12 == 0;
		int iSlow14 = iSlow12 == 1;
		int iSlow15 = iSlow12 == 2;
		int iSlow16 = iSlow12 == 3;
		float fSlow17 = fConst1 * ((iSlow13) ? ((iSlow14) ? ((iSlow15) ? ((iSlow16) ? 0.05f : 1.5f) : 1.8f) : 1.8f) : 2.0f);
		float fSlow18 = fConst1 * static_cast<float>(fHslider9);
		int iSlow19 = static_cast<int>(static_cast<float>(fEntry1));
		int iSlow20 = iSlow19 >= 8;
		int iSlow21 = iSlow19 >= 4;
		int iSlow22 = iSlow19 >= 2;
		int iSlow23 = iSlow19 >= 1;
		int iSlow24 = iSlow19 >= 3;
		int iSlow25 = iSlow19 >= 6;
		int iSlow26 = iSlow19 >= 5;
		int iSlow27 = iSlow19 >= 7;
		int iSlow28 = iSlow19 >= 12;
		int iSlow29 = iSlow19 >= 10;
		int iSlow30 = iSlow19 >= 9;
		int iSlow31 = iSlow19 >= 11;
		int iSlow32 = iSlow19 >= 14;
		int iSlow33 = iSlow19 >= 13;
		float fSlow34 = fSlow10 * static_cast<float>(((iSlow20) ? ((iSlow28) ? ((iSlow32) ? 17 : ((iSlow33) ? 24 : 17)) : ((iSlow29) ? ((iSlow31) ? 22 : 20) : ((iSlow30) ? 19 : 14))) : ((iSlow21) ? ((iSlow25) ? ((iSlow27) ? 23 : 29) : ((iSlow26) ? 27 : 26)) : ((iSlow22) ? ((iSlow24) ? 24 : 22) : ((iSlow23) ? 20 : 19)))));
		float fSlow35 = fConst1 * ((iSlow13) ? ((iSlow14) ? ((iSlow15) ? ((iSlow16) ? 1.5f : 0.05f) : 1.8f) : 1.8f) : 2.0f);
		int iSlow36 = iSlow19 >= 15;
		float fSlow37 = fSlow10 * static_cast<float>(((iSlow20) ? ((iSlow28) ? ((iSlow32) ? ((iSlow36) ? 12 : 10) : ((iSlow33) ? 20 : 14)) : ((iSlow29) ? ((iSlow31) ? 19 : 17) : ((iSlow30) ? 15 : 10))) : ((iSlow21) ? ((iSlow25) ? ((iSlow27) ? 17 : 22) : ((iSlow26) ? 20 : 19)) : ((iSlow22) ? ((iSlow24) ? 17 : 15) : ((iSlow23) ? 14 : 12)))));
		float fSlow38 = fConst1 * ((iSlow13) ? ((iSlow14) ? ((iSlow15) ? 1.5f : 0.05f) : 1.8f) : 2.0f);
		float fSlow39 = fSlow10 * static_cast<float>(((iSlow20) ? ((iSlow28) ? ((iSlow32) ? 7 : ((iSlow33) ? 17 : 10)) : ((iSlow29) ? ((iSlow31) ? 15 : 14) : ((iSlow30) ? 12 : 7))) : ((iSlow21) ? ((iSlow25) ? ((iSlow27) ? 14 : 17) : ((iSlow26) ? 15 : 14)) : ((iSlow22) ? ((iSlow24) ? 12 : 10) : ((iSlow23) ? 8 : 7)))));
		float fSlow40 = ((iSlow15) ? 1.5f : 1.8f);
		float fSlow41 = fConst1 * ((iSlow13) ? ((iSlow14) ? fSlow40 : 0.05f) : 2.0f);
		float fSlow42 = fSlow10 * static_cast<float>(((iSlow20) ? ((iSlow28) ? ((iSlow32) ? ((iSlow36) ? 5 : 3) : ((iSlow33) ? 14 : 7)) : ((iSlow29) ? ((iSlow31) ? 12 : 10) : ((iSlow30) ? 8 : 3))) : ((iSlow21) ? ((iSlow25) ? ((iSlow27) ? 11 : 14) : ((iSlow26) ? 12 : 10)) : ((iSlow22) ? ((iSlow24) ? 8 : 7) : ((iSlow23) ? 5 : 3)))));
		float fSlow43 = fConst1 * ((iSlow13) ? ((iSlow14) ? fSlow40 : 1.8f) : 0.05f);
		float fSlow44 = fSlow10 * static_cast<float>(((iSlow20) ? ((iSlow28) ? ((iSlow32) ? 0 : ((iSlow33) ? 10 : 3)) : ((iSlow29) ? ((iSlow31) ? 8 : 7) : ((iSlow30) ? 5 : 0))) : ((iSlow21) ? ((iSlow25) ? ((iSlow27) ? 7 : 10) : ((iSlow26) ? 8 : 7)) : ((iSlow22) ? ((iSlow24) ? 5 : 3) : ((iSlow23) ? 2 : 0)))));
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			fRec0[0] = fSlow0 + fConst2 * fRec0[1];
			fRec1[0] = fSlow1 + fConst2 * fRec1[1];
			float fTemp0 = static_cast<float>(input0[i0]);
			fRec2[0] = fSlow2 + fConst2 * fRec2[1];
			float fTemp1 = 1.5707964f * fRec2[0];
			fRec3[0] = fSlow3 + fConst2 * fRec3[1];
			fRec5[0] = fSlow4 + fConst2 * fRec5[1];
			fRec6[0] = fSlow5 + fConst2 * fRec6[1];
			float fTemp2 = fTemp0 * fRec5[0] * fRec6[0];
			fRec7[0] = fSlow6 + fConst2 * fRec7[1];
			float fTemp3 = std::pow(0.001f, fConst3 / fRec7[0]);
			fRec9[0] = fSlow11 + fSlow9 * fRec9[1];
			fRec10[0] = fSlow17 + fConst2 * fRec10[1];
			float fTemp4 = fRec9[0] * fRec10[0];
			fRec11[0] = fSlow18 + fConst2 * fRec11[1];
			float fTemp5 = std::max<float>(0.0f, std::min<float>(2.0f, fRec11[0]));
			float fTemp6 = std::max<float>(0.0f, 1.0f - std::fabs(fTemp5));
			float fTemp7 = std::max<float>(0.0f, 1.0f - std::fabs(fTemp5 + -1.0f));
			float fTemp8 = std::max<float>(0.0f, 1.0f - std::fabs(fTemp5 + -2.0f));
			float fTemp9 = 3.0f * fTemp6 + 5.44439f * fTemp7 + 0.49545455f * fTemp8;
			fRec12[0] = fSlow34 + fSlow9 * fRec12[1];
			float fTemp10 = std::pow(2.0f, 0.083333336f * fRec12[0]);
			fRec8[0] = fConst1 * fTemp4 * fTemp9 * fTemp10 + fConst2 * fRec8[1];
			float fTemp11 = FaustResonator_faustpower2_f(fTemp3);
			fRec4[0] = fTemp2 + 2.0f * fTemp3 * fRec4[1] * std::cos(fConst4 * fRec8[0]) - fTemp11 * fRec4[2];
			float fTemp12 = 2.0f * fTemp6 + 2.77775f * fTemp7 + 0.26136363f * fTemp8;
			fRec14[0] = fConst1 * fTemp4 * fTemp12 * fTemp10 + fConst2 * fRec14[1];
			fRec13[0] = fTemp2 + 2.0f * fTemp3 * fRec13[1] * std::cos(fConst4 * fRec14[0]) - fTemp11 * fRec13[2];
			float fTemp13 = fTemp6 + 0.99999f * fTemp7 + 0.14090909f * fTemp8;
			fRec16[0] = fConst1 * fTemp4 * fTemp13 * fTemp10 + fConst2 * fRec16[1];
			fRec15[0] = fTemp2 + 2.0f * fTemp3 * fRec15[1] * std::cos(fConst4 * fRec16[0]) - fTemp11 * fRec15[2];
			fRec19[0] = fSlow35 + fConst2 * fRec19[1];
			float fTemp14 = fRec9[0] * fRec19[0];
			fRec20[0] = fSlow37 + fSlow9 * fRec20[1];
			float fTemp15 = std::pow(2.0f, 0.083333336f * fRec20[0]);
			fRec18[0] = fConst1 * fTemp14 * fTemp9 * fTemp15 + fConst2 * fRec18[1];
			fRec17[0] = fTemp2 + 2.0f * fTemp3 * fRec17[1] * std::cos(fConst4 * fRec18[0]) - fTemp11 * fRec17[2];
			fRec22[0] = fConst1 * fTemp14 * fTemp12 * fTemp15 + fConst2 * fRec22[1];
			fRec21[0] = fTemp2 + 2.0f * fTemp3 * fRec21[1] * std::cos(fConst4 * fRec22[0]) - fTemp11 * fRec21[2];
			fRec24[0] = fConst1 * fTemp14 * fTemp13 * fTemp15 + fConst2 * fRec24[1];
			fRec23[0] = fTemp2 + 2.0f * fTemp3 * fRec23[1] * std::cos(fConst4 * fRec24[0]) - fTemp11 * fRec23[2];
			fRec27[0] = fSlow38 + fConst2 * fRec27[1];
			float fTemp16 = fRec9[0] * fRec27[0];
			fRec28[0] = fSlow39 + fSlow9 * fRec28[1];
			float fTemp17 = std::pow(2.0f, 0.083333336f * fRec28[0]);
			fRec26[0] = fConst1 * fTemp16 * fTemp9 * fTemp17 + fConst2 * fRec26[1];
			fRec25[0] = fTemp2 + 2.0f * fTemp3 * fRec25[1] * std::cos(fConst4 * fRec26[0]) - fTemp11 * fRec25[2];
			fRec30[0] = fConst1 * fTemp16 * fTemp12 * fTemp17 + fConst2 * fRec30[1];
			fRec29[0] = fTemp2 + 2.0f * fTemp3 * fRec29[1] * std::cos(fConst4 * fRec30[0]) - fTemp11 * fRec29[2];
			fRec32[0] = fConst1 * fTemp16 * fTemp13 * fTemp17 + fConst2 * fRec32[1];
			fRec31[0] = fTemp2 + 2.0f * fTemp3 * fRec31[1] * std::cos(fConst4 * fRec32[0]) - fTemp11 * fRec31[2];
			fRec35[0] = fSlow41 + fConst2 * fRec35[1];
			float fTemp18 = fRec9[0] * fRec35[0];
			fRec36[0] = fSlow42 + fSlow9 * fRec36[1];
			float fTemp19 = std::pow(2.0f, 0.083333336f * fRec36[0]);
			fRec34[0] = fConst1 * fTemp18 * fTemp9 * fTemp19 + fConst2 * fRec34[1];
			fRec33[0] = fTemp2 + 2.0f * fTemp3 * fRec33[1] * std::cos(fConst4 * fRec34[0]) - fTemp11 * fRec33[2];
			fRec38[0] = fConst1 * fTemp18 * fTemp12 * fTemp19 + fConst2 * fRec38[1];
			fRec37[0] = fTemp2 + 2.0f * fTemp3 * fRec37[1] * std::cos(fConst4 * fRec38[0]) - fTemp11 * fRec37[2];
			fRec40[0] = fConst1 * fTemp18 * fTemp13 * fTemp19 + fConst2 * fRec40[1];
			fRec39[0] = fTemp2 + 2.0f * fTemp3 * fRec39[1] * std::cos(fConst4 * fRec40[0]) - fTemp11 * fRec39[2];
			fRec43[0] = fSlow43 + fConst2 * fRec43[1];
			fRec44[0] = fSlow44 + fSlow9 * fRec44[1];
			float fTemp20 = fRec9[0] * fRec43[0] * std::pow(2.0f, 0.083333336f * fRec44[0]);
			fRec42[0] = fConst1 * fTemp20 * fTemp9 + fConst2 * fRec42[1];
			fRec41[0] = fTemp2 + 2.0f * fTemp3 * fRec41[1] * std::cos(fConst4 * fRec42[0]) - fTemp11 * fRec41[2];
			fRec46[0] = fConst1 * fTemp20 * fTemp13 + fConst2 * fRec46[1];
			fRec45[0] = fTemp2 + 2.0f * fRec45[1] * fTemp3 * std::cos(fConst4 * fRec46[0]) - fRec45[2] * fTemp11;
			fRec48[0] = fConst1 * fTemp20 * fTemp12 + fConst2 * fRec48[1];
			fRec47[0] = fTemp2 + 2.0f * fTemp3 * fRec47[1] * std::cos(fConst4 * fRec48[0]) - fTemp11 * fRec47[2];
			float fTemp21 = std::min<float>(1.0f, std::max<float>(-1.0f, fRec1[0] * (fTemp0 * std::cos(fTemp1) + 0.025f * (fRec3[0] * (fRec4[0] + fRec13[0] + fRec15[0] + fRec17[0] + fRec21[0] + fRec23[0] + fRec25[0] + fRec29[0] + fRec31[0] + fRec33[0] + fRec37[0] + fRec39[0] + fRec41[0] + fRec45[0] + fRec47[0] - (fRec4[2] + fRec13[2] + fRec17[2] + fRec21[2] + fRec25[2] + fRec29[2] + fRec33[2] + fRec37[2] + fRec45[2] + fRec47[2] + fRec41[2] + fRec39[2] + fRec31[2] + fRec23[2] + fRec15[2])) * std::sin(fTemp1) / std::sqrt(std::max<float>(1.0f, fRec7[0]))))));
			fRec58[0] = 0.5f * (fRec58[1] + fRec57[1]);
			fVec0[IOTA0 & 8191] = fTemp21 + 0.5f * fRec58[0];
			fRec57[0] = fVec0[(IOTA0 - iConst5) & 8191];
			fRec60[0] = 0.5f * (fRec60[1] + fRec59[1]);
			fVec1[IOTA0 & 8191] = fTemp21 + 0.5f * fRec60[0];
			fRec59[0] = fVec1[(IOTA0 - iConst6) & 8191];
			fRec62[0] = 0.5f * (fRec62[1] + fRec61[1]);
			fVec2[IOTA0 & 8191] = fTemp21 + 0.5f * fRec62[0];
			fRec61[0] = fVec2[(IOTA0 - iConst7) & 8191];
			fRec64[0] = 0.5f * (fRec64[1] + fRec63[1]);
			fVec3[IOTA0 & 8191] = fTemp21 + 0.5f * fRec64[0];
			fRec63[0] = fVec3[(IOTA0 - iConst8) & 8191];
			fRec66[0] = 0.5f * (fRec66[1] + fRec65[1]);
			fVec4[IOTA0 & 8191] = fTemp21 + 0.5f * fRec66[0];
			fRec65[0] = fVec4[(IOTA0 - iConst9) & 8191];
			fRec68[0] = 0.5f * (fRec68[1] + fRec67[1]);
			fVec5[IOTA0 & 8191] = fTemp21 + 0.5f * fRec68[0];
			fRec67[0] = fVec5[(IOTA0 - iConst10) & 8191];
			fRec70[0] = 0.5f * (fRec70[1] + fRec69[1]);
			fVec6[IOTA0 & 8191] = fTemp21 + 0.5f * fRec70[0];
			fRec69[0] = fVec6[(IOTA0 - iConst11) & 8191];
			fRec72[0] = 0.5f * (fRec72[1] + fRec71[1]);
			fVec7[IOTA0 & 8191] = fTemp21 + 0.5f * fRec72[0];
			fRec71[0] = fVec7[(IOTA0 - iConst12) & 8191];
			float fTemp22 = fRec57[1] + fRec59[1] + fRec61[1] + fRec63[1] + fRec65[1] + fRec67[1] + fRec69[1] + 0.5f * fRec55[1] + fRec71[1];
			fVec8[IOTA0 & 2047] = fTemp22;
			fRec55[0] = fVec8[(IOTA0 - iConst13) & 2047];
			float fRec56 = -(0.5f * fTemp22);
			float fTemp23 = fRec55[1] + fRec56 + 0.5f * fRec53[1];
			fVec9[IOTA0 & 2047] = fTemp23;
			fRec53[0] = fVec9[(IOTA0 - iConst14) & 2047];
			float fRec54 = -(0.5f * fTemp23);
			float fTemp24 = fRec53[1] + fRec54 + 0.5f * fRec51[1];
			fVec10[IOTA0 & 2047] = fTemp24;
			fRec51[0] = fVec10[(IOTA0 - iConst15) & 2047];
			float fRec52 = -(0.5f * fTemp24);
			float fTemp25 = fRec51[1] + fRec52 + 0.5f * fRec49[1];
			fVec11[IOTA0 & 1023] = fTemp25;
			fRec49[0] = fVec11[(IOTA0 - iConst16) & 1023];
			float fRec50 = -(0.5f * fTemp25);
			output0[i0] = static_cast<FAUSTFLOAT>((1.0f - fRec0[0]) * fTemp21 + fRec0[0] * (fRec50 + fRec49[1]));
			fRec0[1] = fRec0[0];
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec3[1] = fRec3[0];
			fRec5[1] = fRec5[0];
			fRec6[1] = fRec6[0];
			fRec7[1] = fRec7[0];
			fRec9[1] = fRec9[0];
			fRec10[1] = fRec10[0];
			fRec11[1] = fRec11[0];
			fRec12[1] = fRec12[0];
			fRec8[1] = fRec8[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec14[1] = fRec14[0];
			fRec13[2] = fRec13[1];
			fRec13[1] = fRec13[0];
			fRec16[1] = fRec16[0];
			fRec15[2] = fRec15[1];
			fRec15[1] = fRec15[0];
			fRec19[1] = fRec19[0];
			fRec20[1] = fRec20[0];
			fRec18[1] = fRec18[0];
			fRec17[2] = fRec17[1];
			fRec17[1] = fRec17[0];
			fRec22[1] = fRec22[0];
			fRec21[2] = fRec21[1];
			fRec21[1] = fRec21[0];
			fRec24[1] = fRec24[0];
			fRec23[2] = fRec23[1];
			fRec23[1] = fRec23[0];
			fRec27[1] = fRec27[0];
			fRec28[1] = fRec28[0];
			fRec26[1] = fRec26[0];
			fRec25[2] = fRec25[1];
			fRec25[1] = fRec25[0];
			fRec30[1] = fRec30[0];
			fRec29[2] = fRec29[1];
			fRec29[1] = fRec29[0];
			fRec32[1] = fRec32[0];
			fRec31[2] = fRec31[1];
			fRec31[1] = fRec31[0];
			fRec35[1] = fRec35[0];
			fRec36[1] = fRec36[0];
			fRec34[1] = fRec34[0];
			fRec33[2] = fRec33[1];
			fRec33[1] = fRec33[0];
			fRec38[1] = fRec38[0];
			fRec37[2] = fRec37[1];
			fRec37[1] = fRec37[0];
			fRec40[1] = fRec40[0];
			fRec39[2] = fRec39[1];
			fRec39[1] = fRec39[0];
			fRec43[1] = fRec43[0];
			fRec44[1] = fRec44[0];
			fRec42[1] = fRec42[0];
			fRec41[2] = fRec41[1];
			fRec41[1] = fRec41[0];
			fRec46[1] = fRec46[0];
			fRec45[2] = fRec45[1];
			fRec45[1] = fRec45[0];
			fRec48[1] = fRec48[0];
			fRec47[2] = fRec47[1];
			fRec47[1] = fRec47[0];
			fRec58[1] = fRec58[0];
			IOTA0 = IOTA0 + 1;
			fRec57[1] = fRec57[0];
			fRec60[1] = fRec60[0];
			fRec59[1] = fRec59[0];
			fRec62[1] = fRec62[0];
			fRec61[1] = fRec61[0];
			fRec64[1] = fRec64[0];
			fRec63[1] = fRec63[0];
			fRec66[1] = fRec66[0];
			fRec65[1] = fRec65[0];
			fRec68[1] = fRec68[0];
			fRec67[1] = fRec67[0];
			fRec70[1] = fRec70[0];
			fRec69[1] = fRec69[0];
			fRec72[1] = fRec72[0];
			fRec71[1] = fRec71[0];
			fRec55[1] = fRec55[0];
			fRec53[1] = fRec53[0];
			fRec51[1] = fRec51[0];
			fRec49[1] = fRec49[0];
		}
	}

};

#ifdef FAUST_UIMACROS
	
	#define FAUST_FILE_NAME "resonator_esp32.dsp"
	#define FAUST_CLASS_NAME "FaustResonator"
	#define FAUST_COMPILATION_OPIONS "-lang cpp -fpga-mem-th 4 -nvi -ct 1 -cn FaustResonator -es 1 -mcd 16 -mdd 1024 -mdy 33 -uim -single -ftz 0"
	#define FAUST_INPUTS 1
	#define FAUST_OUTPUTS 1
	#define FAUST_ACTIVES 12
	#define FAUST_PASSIVES 0

	FAUST_ADDHORIZONTALSLIDER("10. Resonator Trim", fHslider3, 0.4f, 0.0f, 2.0f, 0.01f);
	FAUST_ADDHORIZONTALSLIDER("11. Dry/Wet Mix", fHslider2, 0.0f, 0.0f, 1.0f, 0.01f);
	FAUST_ADDHORIZONTALSLIDER("12. Output Gain", fHslider1, 1.0f, 0.0f, 4.0f, 0.01f);
	FAUST_ADDNUMENTRY("13. Voicing Index", fEntry0, 0.0f, 0.0f, 4.0f, 1.0f);
	FAUST_ADDHORIZONTALSLIDER("14. Reverb Mix", fHslider0, 0.0f, 0.0f, 1.0f, 0.01f);
	FAUST_ADDHORIZONTALSLIDER("3. Root Note (MIDI)", fHslider7, 36.0f, 24.0f, 72.0f, 1.0f);
	FAUST_ADDNUMENTRY("4. Chord Grid", fEntry1, 0.0f, 0.0f, 15.0f, 1.0f);
	FAUST_ADDHORIZONTALSLIDER("5. Chord Glide (s)", fHslider8, 0.5f, 0.001f, 1e+01f, 0.01f);
	FAUST_ADDHORIZONTALSLIDER("6. Timbre Morph", fHslider9, 0.0f, 0.0f, 2.0f, 0.01f);
	FAUST_ADDHORIZONTALSLIDER("7. Decay Time (s)", fHslider6, 2.0f, 0.1f, 15.0f, 0.01f);
	FAUST_ADDHORIZONTALSLIDER("8. Harmonic Drive", fHslider5, 1.0f, 0.1f, 4.0f, 0.1f);
	FAUST_ADDHORIZONTALSLIDER("9. Input Trim", fHslider4, 0.1f, 0.0f, 1.0f, 0.01f);

	#define FAUST_LIST_ACTIVES(p) \
		p(HORIZONTALSLIDER, 10._Resonator_Trim, "10. Resonator Trim", fHslider3, 0.4f, 0.0f, 2.0f, 0.01f) \
		p(HORIZONTALSLIDER, 11._Dry/Wet_Mix, "11. Dry/Wet Mix", fHslider2, 0.0f, 0.0f, 1.0f, 0.01f) \
		p(HORIZONTALSLIDER, 12._Output_Gain, "12. Output Gain", fHslider1, 1.0f, 0.0f, 4.0f, 0.01f) \
		p(NUMENTRY, 13._Voicing_Index, "13. Voicing Index", fEntry0, 0.0f, 0.0f, 4.0f, 1.0f) \
		p(HORIZONTALSLIDER, 14._Reverb_Mix, "14. Reverb Mix", fHslider0, 0.0f, 0.0f, 1.0f, 0.01f) \
		p(HORIZONTALSLIDER, 3._Root_Note_(MIDI), "3. Root Note (MIDI)", fHslider7, 36.0f, 24.0f, 72.0f, 1.0f) \
		p(NUMENTRY, 4._Chord_Grid, "4. Chord Grid", fEntry1, 0.0f, 0.0f, 15.0f, 1.0f) \
		p(HORIZONTALSLIDER, 5._Chord_Glide_(s), "5. Chord Glide (s)", fHslider8, 0.5f, 0.001f, 1e+01f, 0.01f) \
		p(HORIZONTALSLIDER, 6._Timbre_Morph, "6. Timbre Morph", fHslider9, 0.0f, 0.0f, 2.0f, 0.01f) \
		p(HORIZONTALSLIDER, 7._Decay_Time_(s), "7. Decay Time (s)", fHslider6, 2.0f, 0.1f, 15.0f, 0.01f) \
		p(HORIZONTALSLIDER, 8._Harmonic_Drive, "8. Harmonic Drive", fHslider5, 1.0f, 0.1f, 4.0f, 0.1f) \
		p(HORIZONTALSLIDER, 9._Input_Trim, "9. Input Trim", fHslider4, 0.1f, 0.0f, 1.0f, 0.01f) \

	#define FAUST_LIST_PASSIVES(p) \

#endif

#endif

/**
 * Minimal architecture stubs for Faust-generated DSP code.
 *
 * Faust generates classes that extend `dsp` and reference `UI` and `Meta`.
 * On a desktop these come from libfaust's runtime; on the ESP32 we don't
 * use the UI builder at all (we poke parameter members directly), so we
 * provide do-nothing stubs to satisfy the type signatures.
 */

#ifndef FAUST_ARCH_H
#define FAUST_ARCH_H

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

struct Meta {
    virtual ~Meta() = default;
    virtual void declare(const char*, const char*) {}
};

struct UI {
    virtual ~UI() = default;
    virtual void openVerticalBox(const char*) {}
    virtual void openHorizontalBox(const char*) {}
    virtual void openTabBox(const char*) {}
    virtual void closeBox() {}
    virtual void addButton(const char*, FAUSTFLOAT*) {}
    virtual void addCheckButton(const char*, FAUSTFLOAT*) {}
    virtual void addHorizontalSlider(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addVerticalSlider(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addNumEntry(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addHorizontalBargraph(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addVerticalBargraph(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addSoundfile(const char*, const char*, struct Soundfile**) {}
    virtual void declare(FAUSTFLOAT*, const char*, const char*) {}
};

struct dsp {
    virtual ~dsp() = default;
    virtual int  getNumInputs() = 0;
    virtual int  getNumOutputs() = 0;
    virtual void buildUserInterface(UI* ui_interface) = 0;
    virtual int  getSampleRate() = 0;
    virtual void init(int sample_rate) = 0;
    virtual void instanceInit(int sample_rate) = 0;
    virtual void instanceConstants(int sample_rate) = 0;
    virtual void instanceResetUserInterface() = 0;
    virtual void instanceClear() = 0;
    virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) = 0;
};

#endif // FAUST_ARCH_H

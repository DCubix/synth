#ifndef SYN_SYNTH_VM_H
#define SYN_SYNTH_VM_H

#include "../log/log.h"
#include "../common.h"
#include "phase.h"
#include "adsr.h"
#include "chorus.h"

#include "compiler.h"

extern "C" {
	#include "../../sndfilter/src/compressor.h"
}

#include <mutex>

constexpr u32 SynMaxVoices = 64;
constexpr u32 SynSampleCount = 8;

using Float8 = std::array<f32, SynSampleCount>;

class SynthVM {
public:
	SynthVM();
	~SynthVM() = default;

	void load(const Program& program);
	Float8 execute(f32 sampleRate, f32 frequency, u32 channel = 0);

	std::array<ADSR, 128>& envelopes() { return m_envs; }
	u32 usedEnvelopes() const { return m_usedEnvs; }

	ADSR* getLongestEnvelope();

private:
	util::Stack<Float8, 512> m_resultStack;
	util::Stack<f32, 512> m_valueStack;
	Float8 m_out{ 0.0f };

	Program m_program;

	std::array<Phase, 128> m_phases;
	std::array<ADSR, 128> m_envs;
	std::array<Float8, SynMaxNodes> m_storage;

	u32 m_usedEnvs{ 0 };

	ADSR* m_longestEnv{ nullptr };

	std::mutex m_lock;
};

struct Voice {
	void setNote(u32 note);
	void free() { active = false; note = 0; sustained = false; }

	u32 note{ 0 }, index{ 0 };
	f32 velocity{ 0.0f }, frequency{ 0.0f };
	bool active{ false }, sustained{ false };
};

class Synth {
public:
	Synth();
	~Synth() = default;

	void noteOn(u32 noteNumber, f32 velocity = 1.0f);
	void noteOff(u32 noteNumber, f32 velocity = 1.0f);
	Float8 sample(u32 channel = 0);

	void setProgram(const Program& program);

	f32 sampleRate() const { return m_sampleRate; }

	Chorus& chorusEffect() { return m_chorus; }

	bool chorusEnabled() const { return m_chorusEnabled; }
	void chorusEnabled(bool v) { m_chorusEnabled = v; }

	void sustainOn();
	void sustainOff();

private:
	std::array<Voice, SynMaxVoices> m_voices;
	std::array<std::unique_ptr<SynthVM>, SynMaxVoices> m_vms;
	Voice* findFreeVoice(u32 note);
	Voice* getVoice(u32 note);

	f32 m_sampleRate{ 44100.0f };

	sf_compressor_state_st m_compressor;
	Chorus m_chorus;
	bool m_chorusEnabled{ false }, m_sustain{ false };

	std::mutex m_lock;
};

#endif // SYN_SYNTH_VM_H

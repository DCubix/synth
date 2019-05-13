#include "synth_vm.h"

SynthVM::SynthVM() {
	load(Program());
}

void SynthVM::load(const Program& program) {
	m_lock.lock();
	m_program = program;
	m_longestEnv = nullptr;
	m_valueStack.clear(0.0f);
	m_resultStack.clear(Float8{ 0.0f });
	m_storage.fill(Float8{ 0.0f });
	m_lock.unlock();
}

Float8 SynthVM::execute(f32 sampleRate, f32 frequency, u32 channel) {
#define PAN ((channel == 0) ? 1.0f - pan : pan);

	u32 i = 0 /* Phase */,
		e = 0 /* Env */;
	u32 pc = 0;

	while (pc < m_program.size()) {
		Instruction& ins = m_program[pc++];
		Float8 result = m_storage[ins.nodeID];

		f32 pan = m_valueStack.pop().value_or(0.5f);
		f32 level = m_valueStack.pop().value_or(1.0f);
		f32 coef = level * PAN;

		switch (ins.opcode) {
			case OpPush: m_valueStack.push(ins.param); break;
			case OpPushPtr: m_valueStack.push(*ins.paramPtr); break;
			case OpOut: {
				Float8 ret{ 0.0f };
				while (!m_resultStack.empty()) {
					Float8 res = m_resultStack.pop().value();
					for (u32 i = 0; i < SynSampleCount; i++) {
						ret[i] += res[i] * coef;
					}
				}
				result = ret;
				m_out = result;
			} break;
			case OpSine: {
				Float8 freqMod{ 0.0f };
				f32 lvlm = m_valueStack.pop().value_or(1.0f);
				f32 offset = m_valueStack.pop().value_or(0.0f);
				while (!m_resultStack.empty()) {
					Float8 res = m_resultStack.pop().value();
					for (u32 i = 0; i < SynSampleCount; i++) {
						freqMod[i] += res[i];
					}
				}

				Phase& phase = m_phases[i++];
				for (u32 i = 0; i < SynSampleCount; i++) {
					f32 p = phase.advance(frequency + offset, sampleRate);
					result[i] = std::sin(p + freqMod[i]) * coef * lvlm;
				}
				m_resultStack.push(result);
			} break;
			case OpLFO: {
				f32 lvlm = m_valueStack.pop().value_or(1.0f);
				f32 freq = m_valueStack.pop().value_or(0.0f);
				f32 vmax = m_valueStack.pop().value_or(1.0f);
				f32 vmin = m_valueStack.pop().value_or(0.0f);

				Phase& phase = m_phases[i++];
				for (u32 i = 0; i < SynSampleCount; i++) {
					f32 p = phase.advance(freq, sampleRate);
					f32 s = vmin + (std::sin(p) * 0.5f + 0.5f) * (vmax - vmin);
					result[i] = s * level * lvlm;
				}
				m_resultStack.push(result);
			} break;
			case OpADSR: {
				f32 r = m_valueStack.pop().value_or(0.0f);
				f32 s = m_valueStack.pop().value_or(1.0f);
				f32 d = m_valueStack.pop().value_or(0.0f);
				f32 a = m_valueStack.pop().value_or(0.0f);
				ADSR& env = m_envs[e++];
				env.attack(a * sampleRate);
				env.decay(d * sampleRate);
				env.sustain(s);
				env.release(r * sampleRate);

				for (u32 i = 0; i < SynSampleCount; i++) {
					result[i] = env.sample() * level;
				}
				m_resultStack.push(result);
			} break;
			case OpMap: {
				Float8 in = m_resultStack.pop().value_or(Float8{ 0.0f });
				f32 tma = m_valueStack.pop().value_or(1.0f);
				f32 tmi = m_valueStack.pop().value_or(0.0f);
				f32 fma = m_valueStack.pop().value_or(1.0f);
				f32 fmi = m_valueStack.pop().value_or(0.0f);

				for (u32 i = 0; i < SynSampleCount; i++) {
					f32 norm = (in[i] - fmi) / (fma - fmi);
					result[i] = (norm + tmi) * (tma - tmi);
				}
				m_resultStack.push(result);
			} break;
			case OpWrite: {
				Float8 in = m_resultStack.pop().value_or(Float8{ 0.0f });
				f32 chan = m_valueStack.pop().value_or(0.0f);
				u32 idx = u32(chan) % m_storage.size();
				result = m_storage[idx] = in;
				m_resultStack.push(m_storage[idx]);
			} break;
			case OpRead: {
				f32 chan = m_valueStack.pop().value_or(0.0f);
				u32 idx = u32(chan) % m_storage.size();
				result = m_storage[idx];
				m_resultStack.push(result);
			} break;
			case OpValue: {
				f32 val = m_valueStack.pop().value_or(0.0f);
				result.fill(val);
				m_resultStack.push(result);
			} break;
			case OpMul: {
				Float8 b = m_resultStack.pop().value_or(Float8{ 0.0f });
				Float8 a = m_resultStack.pop().value_or(Float8{ 0.0f });
				for (u32 i = 0; i < SynSampleCount; i++) {
					result[i] = a[i] * b[i];
				}
				m_resultStack.push(result);
			} break;
		}
		// Write result to storage
		m_storage[ins.nodeID] = result;
	}
	m_usedEnvs = e;

	m_longestEnv = nullptr;
	if (e > 0) {
		f32 time = -1.0f;
		for (u32 i = 0; i < e; i++) {
			if (m_envs[i].totalTime() > time) {
				time = m_envs[i].totalTime();
				m_longestEnv = &m_envs[i];
			}
		}
	}

	return m_out;
}

ADSR* SynthVM::getLongestEnvelope() {
	return m_longestEnv;
}

void Voice::setNote(u32 note) {
	this->note = note;
	this->frequency = (440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f));
}

Synth::Synth() {
	sf_simplecomp(&m_compressor, int(m_sampleRate), 0.001f, -30.0f, 30.0f, 10.0f, 0.03f, 0.3f);
	for (u32 i = 0; i < SynMaxVoices; i++) {
		m_vms[i] = std::make_unique<SynthVM>();
		m_voices[i].index = i;
	}
}

void Synth::noteOn(u32 noteNumber, f32 velocity) {
	Voice* voice = findFreeVoice(noteNumber);
	if (!voice) return;

	voice->velocity = velocity;
	voice->active = true;
	if (voice->note == noteNumber) {
		//LogW("!!Retrigger Voice!!");
		for (auto&& env : m_vms[voice->index]->envelopes()) {
			env.active = true;
			env.gate(false);
			env.reset();
			env.gate(true);
		}
	} else {
		for (auto&& env : m_vms[voice->index]->envelopes()) {
			env.active = true;
			env.gate(true);
		}
	}
	voice->setNote(noteNumber);
	voice->sustained = m_sustain;
}

void Synth::noteOff(u32 noteNumber, f32 velocity) {
	Voice* voice = getVoice(noteNumber);
	if (!voice) return;

	SynthVM* vm = m_vms[voice->index].get();
	if (!voice->sustained) {
		if (vm->usedEnvelopes() == 0) voice->free();
		else {
			for (u32 e = 0; e < vm->usedEnvelopes(); e++) {
				vm->envelopes()[e].gate(false);
			}
		}
	}
}

Float8 Synth::sample(u32 channel) {
	Float8 out{ 0.0f };
	for (int i = 0; i < SynMaxVoices; i++) {
		Voice* voice = &m_voices[i];
		if (!voice->active && !voice->sustained) continue;

		SynthVM* vm = m_vms[voice->index].get();
		Float8 s = vm->execute(m_sampleRate, voice->frequency, channel);

		ADSR* env = vm->getLongestEnvelope();
		if (env && !env->active) {
			voice->free();
			//LogI("Freed voice #", i);
		}

		for (u32 i = 0; i < SynSampleCount; i++) {
			out[i] += s[i];
		}
	}

	// Chorus
	if (m_chorusEnabled) {
		for (u32 i = 0; i < SynSampleCount; i++) {
			out[i] += m_chorus.process(out[i], m_sampleRate, channel);
		}
	}

	return out;
}

void Synth::setProgram(const Program& program) {
	for (u32 i = 0; i < m_vms.size(); i++)
		m_vms[i]->load(program);
}

void Synth::sustainOn() {
	m_sustain = true;
	for (u32 i = 0; i < SynMaxVoices; i++) {
		Voice* voice = &m_voices[i];
		if (voice->active) voice->sustained = true;
	}
}

void Synth::sustainOff() {
	m_sustain = false;
	for (u32 i = 0; i < SynMaxVoices; i++) {
		Voice* voice = &m_voices[i];
		if (voice->active) continue;

		SynthVM* vm = m_vms[voice->index].get();
		if (vm->usedEnvelopes() == 0) voice->free();
		else {
			for (u32 e = 0; e < vm->usedEnvelopes(); e++) {
				vm->envelopes()[e].gate(false);
			}
		}
		voice->sustained = false;
	}
}

Voice* Synth::findFreeVoice(u32 note) {
	Voice* freeVoice = nullptr;
	for (auto&& voice : m_voices) {
		if (!voice.active) {
			freeVoice = &voice;
			break;
		} else if (voice.active && voice.note == note) {
			freeVoice = &voice;
			break;
		}
	}
	if (freeVoice == nullptr) {
		freeVoice = &m_voices[0];
	}
	return freeVoice;
}

Voice* Synth::getVoice(u32 note) {
	Voice* voc = nullptr;
	for (auto&& voice : m_voices) {
		if (voice.active && voice.note == note) {
			voc = &voice;
			break;
		}
	}
	return voc;
}

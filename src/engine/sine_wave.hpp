#ifndef SYN_SINE_WAVE_HPP
#define SYN_SINE_WAVE_HPP

#include "node_logic.h"
#include "phase.h"

class SineWave : public Node {
public:
	inline SineWave() {
		addParam("Mod.");
		addParam("Off.");
	}

	inline f32 sample(NodeSystem* system) {
		f32 freqMod = param(0).value;
		f32 offset  = param(1).value;
		f32 freqVal = m_phase.advance(system->frequency() + offset, system->sampleRate()) + freqMod;
		return ::sinf(freqVal);
	}

	inline virtual Program build() override {
		return ProgramBuilder()
			.pushp(&param(0).value)
			.pushp(&param(1).value)
			.pushp(&m_level)
			.sine()
			.build();
	}

	inline NodeType type() { return NodeType::SineWave; }

private:
	Phase m_phase{ PI * 2.0f };
};

#endif // SYN_SINE_WAVE_HPP
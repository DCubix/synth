#ifndef SYN_LFO_HPP
#define SYN_LFO_HPP

#include "node_logic.h"
#include "phase.h"

#include "../log/log.h"

class LFO : public Node {
public:
	inline LFO() {
		addParam("Frequency");
		addParam("Min");
		addParam("Max");
		param(0).value = 2.0f;
		param(1).value = 0.0f;
		param(2).value = 1.0f;
	}

	inline f32 sample(NodeSystem* system) {
		f32 freq = param(0).value;
		f32 min  = param(1).value;
		f32 max  = param(2).value;
		f32 freqVal = m_phase.advance(freq, system->sampleRate());
		f32 val = min + (::sinf(freqVal) * 0.5f + 0.5f) * (max - min);
		//LogI(val);
		return val;
	}

private:
	Phase m_phase{ PI * 2.0f };
};

#endif // SYN_LFO_HPP
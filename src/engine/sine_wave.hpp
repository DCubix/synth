#ifndef SYN_SINE_WAVE_HPP
#define SYN_SINE_WAVE_HPP

#include "node_logic.h"
#include "phase.h"

class SineWave : public Node {
public:
	inline SineWave() {
		addParam("Mod.");
		addParam("Off.");
		addParam("Lvl.");
		param(2).value = 1.0f;
	}

	inline NodeType type() { return NodeType::SineWave; }
	
private:
	Phase m_phase{ PI * 2.0f };
};

#endif // SYN_SINE_WAVE_HPP
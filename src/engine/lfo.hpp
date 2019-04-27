#ifndef SYN_LFO_HPP
#define SYN_LFO_HPP

#include "node_logic.h"
#include "phase.h"

#include "../log/log.h"

class LFO : public Node {
public:
	inline LFO() {
		addParam("Freq.");
		addParam("Lvl.");
		param(0).value = 2.0f;
		param(1).value = 1.0f;
	}

	inline NodeType type() { return NodeType::LFO; }

	f32 min{ 0.0f }, max{ 1.0f };

private:
	Phase m_phase{ PI * 2.0f };
};

#endif // SYN_LFO_HPP
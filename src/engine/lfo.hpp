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

	inline NodeType type() override { return NodeType::LFO; }

	inline virtual void load(Json json) override {
		Node::load(json);
		min = json.value("min", 0.0f);
		max = json.value("max", 1.0f);
		param(0).value = json.value("freq", 2.0f);
	}

	inline virtual void save(Json& json) override {
		Node::save(json);
		json["min"] = min;
		json["max"] = max;
		json["freq"] = param(0).value;
	}

	f32 min{ 0.0f }, max{ 1.0f };

private:
	Phase m_phase{ PI * 2.0f };
};

#endif // SYN_LFO_HPP
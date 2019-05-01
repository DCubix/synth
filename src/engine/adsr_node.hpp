#ifndef SYN_ADSR_HPP
#define SYN_ADSR_HPP

#include "node_logic.h"

class ADSRNode : public Node {
public:
	inline NodeType type() override { return NodeType::ADSR; }

	inline virtual void load(Json json) override {
		Node::load(json);
		a = json.value("a", 0.0f);
		d = json.value("d", 0.0f);
		s = json.value("s", 1.0f);
		r = json.value("r", 0.0f);
	}

	inline virtual void save(Json& json) override {
		Node::save(json);
		json["a"] = a;
		json["d"] = d;
		json["s"] = s;
		json["r"] = r;
	}

	f32 a{ 0.0f }, d{ 0.0f }, s{ 1.0f }, r{ 0.0f };
};

#endif // SYN_ADSR_HPP
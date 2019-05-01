#ifndef SYN_MAP_HPP
#define SYN_MAP_HPP

#include "node_logic.h"

class Map : public Node {
public:
	inline Map() {
		addParam("In");
	}

	inline NodeType type() override { return NodeType::Map; }

	inline virtual void load(Json json) override {
		Node::load(json);
		fromMin = json.value("fromMin", 0.0f);
		fromMax = json.value("fromMax", 1.0f);
		toMin = json.value("toMin", 0.0f);
		toMax = json.value("toMax", 1.0f);
	}

	inline virtual void save(Json& json) override {
		Node::save(json);
		json["fromMin"] = fromMin;
		json["fromMax"] = fromMax;
		json["toMin"] = toMin;
		json["toMax"] = toMax;
	}

	f32 fromMin{ 0.0f }, fromMax{ 1.0f }, toMin{ 0.0f }, toMax{ 1.0f };
};

#endif // SYN_MAP_HPP
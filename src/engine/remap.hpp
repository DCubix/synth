#ifndef SYN_MAP_HPP
#define SYN_MAP_HPP

#include "node_logic.h"

class Map : public Node {
public:
	inline Map() {
		addParam("In");
	}

	inline NodeType type() { return NodeType::Map; }

	f32 fromMin{ 0.0f }, fromMax{ 1.0f }, toMin{ 0.0f }, toMax{ 1.0f };
};

#endif // SYN_MAP_HPP
#ifndef SYN_ADSR_HPP
#define SYN_ADSR_HPP

#include "node_logic.h"

class ADSRNode : public Node {
public:
	inline NodeType type() { return NodeType::ADSR; }

	f32 a{ 0.0f }, d{ 0.0f }, s{ 1.0f }, r{ 0.0f };
};

#endif // SYN_ADSR_HPP
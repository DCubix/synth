#ifndef SYN_STORAGE_HPP
#define SYN_STORAGE_HPP

#include "node_logic.h"

class Writer : public Node {
public:
	inline Writer() {
		addParam("In.");
	}
	virtual NodeType type() { return NodeType::Writer; }
	f32 channel{ 0 };
};

class Reader : public Node {
public:
	virtual NodeType type() { return NodeType::Reader; }
	f32 channel{ 0 };
};

#endif // SYN_STORAGE_HPP
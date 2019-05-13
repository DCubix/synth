#include "node_logic.h"
#include "../log/log.h"

#include <numeric>

#include "sine_wave.hpp"
#include "lfo.hpp"
#include "adsr_node.hpp"
#include "remap.hpp"
#include "storage.hpp"

#include "chorus.h"

void Node::load(Json json) {
	m_level = json.value("level", 1.0f);
	m_pan = json.value("pan", 0.5f);
}

void Node::save(Json& json) {
	json["level"] = m_level;
	json["pan"] = m_pan;
}

void Node::addParam(const std::string& name) {
	m_paramNames.push_back(name);
	m_params.push_back(Param{});
}

Node::Param& Node::param(u32 id) {
	return m_params[id];
}

std::string Node::paramName(u32 id) {
	return m_paramNames[id];
}

NodeSystem::NodeSystem() {
	create<Output>();
}

void NodeSystem::destroy(u32 id) {
	auto pos = std::find(m_usedNodes.begin(), m_usedNodes.end(), id);
	if (pos == m_usedNodes.end()) return;
	for (u32 cid : getAllConnections(id)) {
		disconnect(cid);
	}
	m_lock.lock();
	m_usedNodes.erase(pos);
	m_nodes[id].reset();
	m_lock.unlock();
}

void NodeSystem::clear() {
	m_lock.lock();
	m_usedNodes.clear();
	m_usedConnections.clear();

	for (auto&& ptr : m_nodes) if (ptr) ptr.reset();
	for (auto&& ptr : m_connections) if (ptr) ptr.reset();
	m_lock.unlock();
	create<Output>();
}

u32 NodeSystem::connect(u32 src, u32 dest, u32 param) {
	// is it full?
	if (m_usedConnections.size() == SynMaxConnections-1) return UINT32_MAX;

	auto poss = std::find(m_usedNodes.begin(), m_usedNodes.end(), src);
	if (poss == m_usedNodes.end()) return UINT32_MAX;

	auto posd = std::find(m_usedNodes.begin(), m_usedNodes.end(), dest);
	if (posd == m_usedNodes.end()) return UINT32_MAX;

	if (getConnection(src, dest, param) != UINT32_MAX) {
		LogE("Already connected!");
		return UINT32_MAX;
	}

	m_lock.lock();
	// find a free spot
	u32 spot = 0;
	for (spot = 0; spot < SynMaxConnections; spot++)
		if (!m_connections[spot]) break;
	m_usedConnections.push_back(spot);

	Connection* conn = new Connection();
	conn->src = src;
	conn->dest = dest;
	conn->destParam = param;
	m_connections[spot] = std::unique_ptr<Connection>(conn);

	m_nodes[dest]->param(param).connected = true;

	m_lock.unlock();

	return spot;
}

void NodeSystem::disconnect(u32 connection) {
	auto pos = std::find(m_usedConnections.begin(), m_usedConnections.end(), connection);
	if (pos == m_usedConnections.end()) return;
	m_lock.lock();

	Connection* conn = m_connections[connection].get();
	m_nodes[conn->dest]->param(conn->destParam).connected = false;

	m_usedConnections.erase(pos);
	m_connections[connection].reset();
	m_lock.unlock();
}

u32 NodeSystem::getConnection(u32 dest, u32 param) {
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		if (conn->dest == dest && conn->destParam == param) {
			return cid;
		}
	}
	return UINT32_MAX;
}

std::vector<u32> NodeSystem::getConnections(u32 dest, u32 param) {
	std::vector<u32> res;
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		if (conn->dest == dest && conn->destParam == param) {
			res.push_back(cid);
		}
	}
	return res;
}

u32 NodeSystem::getConnection(u32 src, u32 dest, u32 param) {
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		if (conn->src == src && conn->dest == dest && conn->destParam == param) {
			return cid;
		}
	}
	return UINT32_MAX;
}

u32 NodeSystem::getNodeConnection(u32 src, u32 dest) {
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		if (conn->src == src && conn->dest == dest) {
			return cid;
		}
	}
	return UINT32_MAX;
}

std::vector<u32> NodeSystem::getAllConnections(u32 node) {
	std::vector<u32> res;
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		if (conn->dest == node || conn->src == node) {
			res.push_back(cid);
		}
	}
	return res;
}

static void process(NodeSystem* sys, Node* node, Node* prev, ProgramBuilder& out) {
	auto type = node->type();
	const u32 id = node->id();

	switch (type) {
		case NodeType::ADSR: {
			ADSRNode* an = (ADSRNode*)node;
			out.pushp(id, &an->a);
			out.pushp(id, &an->d);
			out.pushp(id, &an->s);
			out.pushp(id, &an->r);
		} break;
		case NodeType::Map: {
			Map* mn = (Map*)node;
			out.pushp(id, &mn->fromMin);
			out.pushp(id, &mn->fromMax);
			out.pushp(id, &mn->toMin);
			out.pushp(id, &mn->toMax);
		} break;
		case NodeType::Reader: {
			Reader* mn = (Reader*)node;
			out.pushp(id, &mn->channel);
		} break;
		case NodeType::Writer: {
			Writer* mn = (Writer*)node;
			out.pushp(id, &mn->channel);
		} break;
		case NodeType::LFO: {
			LFO* mn = (LFO*)node;
			out.pushp(id, &mn->min);
			out.pushp(id, &mn->max);
		} break;
		case NodeType::Value: {
			Value* mn = (Value*)node;
			out.pushp(id, &mn->value);
		} break;
		default: break;
	}

	for (u32 i = 0; i < node->paramCount(); i++) {
		if (!node->param(i).connected) {
			out.pushp(id, &node->param(i).value);
		} else {
			auto&& conns = sys->getConnections(node->id(), i);
			for (u32 cid : conns) {
				auto conn = sys->getConnection(cid);
				if (conn) {
					if (conn->src == conn->dest ||
						sys->getNodeConnection(conn->dest, conn->src) != UINT32_MAX) { // Handle loops
						out.push(id, conn->dest); // Channel
						out.push(id, 1.0f); // Lvl
						out.read(id);
					} else {
						Node* nd = sys->get<Node>(conn->src);
						process(sys, nd, node, out);
					}
				}
			}
		}
	}
	out.pushp(id, &node->level());
	out.pushp(id, &node->pan());
	switch (type) {
		case NodeType::LFO: out.lfo(id); break;
		case NodeType::SineWave: out.sine(id); break;
		case NodeType::Out: out.out(id); break;
		case NodeType::ADSR: out.adsr(id); break;
		case NodeType::Map: out.map(id); break;
		case NodeType::Reader: out.read(id); break;
		case NodeType::Writer: out.write(id); break;
		case NodeType::Value: out.value(id, ((Value*)node)->value); break;
		case NodeType::Mul: out.mul(id); break;
		default: break;
	}
}

Program NodeSystem::compile() {
	ProgramBuilder builder;
	process(this, m_nodes[0].get(), nullptr, builder);
	return builder.build();
}

Output::Output() {
	addParam("Out");
}

void Value::load(Json json) {
	Node::load(json);
	value = json.value("value", 0.0f);
}

void Value::save(Json& json) {
	Node::save(json);
	json["value"] = value;
}

Mul::Mul() {
	addParam("A");
	addParam("B");
}

#include "node_logic.h"
#include "../log/log.h"

#include "sine_wave.hpp"
#include "lfo.hpp"

void Node::addParam(const std::string& name) {
	m_paramNames.push_back(name);
	m_params.push_back(Param{});
}

Node::Param& Node::param(u32 id) {
	return m_params[id];
}

NodeSystem::NodeSystem() {
	m_sampleRate = 44100.0f;
	m_frequency = 220.0f;
	m_nodeBuffer.fill(0.0f);
	create<Output>();
}

void NodeSystem::destroy(u32 id) {
	auto pos = std::find(m_usedNodes.begin(), m_usedNodes.end(), id);
	if (pos == m_usedNodes.end()) return;
	m_lock.lock();
	m_usedNodes.erase(pos);
	m_nodes[id].reset();
	m_nodeBuffer[id] = 0.0f;
	m_lock.unlock();
}

u32 NodeSystem::connect(u32 src, u32 dest, u32 param) {
	// is it full?
	if (m_usedConnections.size() == SynMaxConnections-1) return UINT32_MAX;

	auto poss = std::find(m_usedNodes.begin(), m_usedNodes.end(), src);
	if (poss == m_usedNodes.end()) return UINT32_MAX;

	auto posd = std::find(m_usedNodes.begin(), m_usedNodes.end(), dest);
	if (posd == m_usedNodes.end()) return UINT32_MAX;

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

Sample NodeSystem::sample() {
	std::vector<u32> broken;

	// "Dirtying" step
	for (u32 nid : m_usedNodes) {
		Node* node = m_nodes[nid].get();
		if (node == nullptr) continue;
		node->solved = false;
	}

	// Solving step
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();

		Node* src  = m_nodes[conn->src].get();
		Node* dest = m_nodes[conn->dest].get();
		if (src == nullptr || dest == nullptr) { // it's a broken connection!
			broken.push_back(cid);
			continue;
		}

		// Solve
		if (!src->solved) { // to prevent from solving twice
			m_nodeBuffer[conn->src] = src->sample(this) * src->level();
			src->solved = true;
		}
	}

	// Cleanup step
	for (u32 conn : broken) {
		disconnect(conn);
	}

	// Assignment step
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		Node* src  = m_nodes[conn->src].get();
		Node* dest = m_nodes[conn->dest].get();
		
		dest->param(conn->destParam).value = m_nodeBuffer[src->m_id];

		if (!dest->solved) {
			m_nodeBuffer[conn->dest] = dest->sample(this) * dest->level();
			dest->solved = true;
		}
	}

	// Return the Output node's value
	auto out = get<Output>(0);
	if (!out) return { 0.0f, 0.0f };
	return {
		m_nodeBuffer[0] * (1.0f - out->pan()),
		m_nodeBuffer[0] * out->pan()
	};

}

f32 Output::sample(NodeSystem* system) {
	return param(0).value;
}

Output::Output() {
	addParam("Out");
	addParam("Pan");
	param(1).value = 0.5f;
}

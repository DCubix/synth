#ifndef SYE_NODE_H
#define SYE_NODE_H

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <algorithm>
#include <optional>
#include <utility>

#include "../common.h"

constexpr u32 SynMaxNodes = 256;
constexpr u32 SynMaxConnections = 512;
constexpr u32 OutputNode = 0;

using Sample = std::pair<f32, f32>;

enum class NodeType {
	None = 0,
	Value,
	Out,
	SineWave,
	LFO
};

class NodeSystem;
class Node {
	friend class NodeSystem;
public:
	struct Param {
		f32 value{ 0.0f };
		bool connected{ false };
	};

	Node() = default;
	~Node() = default;

	virtual f32 sample(NodeSystem* system) = 0;
	virtual NodeType type() { return NodeType::None; }

	float level() const { return m_level; }
	void level(f32 v) { m_level = v; }
	
	u32 id() const { return m_id; }

	void addParam(const std::string& name);
	Param& param(u32 id);
	std::string paramName(u32 id);

	u32 paramCount() const { return m_params.size(); }

protected:
	bool solved{ false };

	f32 m_level{ 1.0f };
	u32 m_id{ 0 };

	std::vector<Param> m_params;
	std::vector<std::string> m_paramNames;
};
using NodePtr = std::unique_ptr<Node>;

class Output : public Node {
public:
	Output();
	f32 sample(NodeSystem* system);
	virtual NodeType type() { return NodeType::Out; }

	float pan() { return param(1).value; }
	void pan(f32 v) { param(1).value = v; }
};

class Value : public Node {
public:
	inline f32 sample(NodeSystem* system) { return level(); }
	inline virtual NodeType type() { return NodeType::Value; }
};

class NodeSystem {
public:
	struct Connection {
		u32 src, dest, destParam;

		Connection() = default;
		~Connection() = default;
	};

	NodeSystem();
	~NodeSystem() = default;

	template <class T, typename... Args>
	u32 create(Args&&... args) {
		// is it full?
		if (m_usedNodes.size() == SynMaxNodes-1) return UINT32_MAX;

		m_lock.lock();
		// find a free spot
		u32 spot = 0;
		for (spot = 0; spot < SynMaxNodes; spot++)
			if (!m_nodes[spot]) break;
		m_usedNodes.push_back(spot);

		// create node
		T* node = new T(std::forward<Args>(args)...);
		node->m_id = spot;
		m_nodes[spot] = std::unique_ptr<T>(node);

		m_lock.unlock();
		return spot;
	}

	void destroy(u32 id);

	template <class T>
	T* get(u32 id) {
		auto pos = std::find(m_usedNodes.begin(), m_usedNodes.end(), id);
		if (pos == m_usedNodes.end()) return nullptr;
		return dynamic_cast<T*>(m_nodes[id].get());
	}

	u32 connect(u32 src, u32 dest, u32 param);
	void disconnect(u32 connection);
	u32 getConnection(u32 dest, u32 param);

	Sample sample();

	f32 sampleRate() const { return m_sampleRate; }

	f32 frequency() const { return m_frequency; }
	void frequency(f32 v) { m_frequency = v; }

	std::vector<u32> nodes() { return m_usedNodes; }
	std::vector<u32> connections() { return m_usedConnections; }

	Connection* getConnection(u32 id) { return m_connections[id].get(); }

	f32 master() const { return m_master; }
	void master(f32 v) { m_master = v; }

private:
	
	std::array<std::unique_ptr<Connection>, SynMaxConnections> m_connections;
	std::vector<u32> m_usedConnections;

	std::array<NodePtr, SynMaxNodes> m_nodes;
	std::vector<u32> m_usedNodes;
	std::array<f32, SynMaxNodes> m_nodeBuffer;

	std::mutex m_lock;

	f32 m_sampleRate, m_frequency, m_master{ 1.0f };
};

#endif // SYE_NODE_H
#ifndef SYE_NODE_H
#define SYE_NODE_H

#include <array>
#include <stack>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <algorithm>
#include <optional>
#include <utility>

#include "../common.h"
#include "../log/log.h"

#include "compiler.h"

constexpr u32 OutputNode = 0;

enum class NodeType {
	None = 0,
	Value,
	Out,
	SineWave,
	LFO,
	ADSR,
	Map,
	Writer,
	Reader,
	Mul
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
	virtual ~Node() = default;

	virtual NodeType type() { return NodeType::None; }
	virtual void load(Json json);
	virtual void save(Json& json);

	float& level() { return m_level; }
	void level(f32 v) { m_level = v; }

	float& pan() { return m_pan; }
	void pan(f32 v) { m_pan = v; }

	u32 id() const { return m_id; }

	void addParam(const std::string& name);
	Param& param(u32 id);
	std::string paramName(u32 id);

	u32 paramCount() const { return m_params.size(); }

protected:
	f32 m_level{ 1.0f }, m_pan{ 0.5f };
	u32 m_id{ 0 };

	std::vector<Param> m_params;
	std::vector<std::string> m_paramNames;
};
using NodePtr = std::unique_ptr<Node>;

class Output : public Node {
public:
	Output();
	virtual NodeType type() override { return NodeType::Out; }
};

class Value : public Node {
public:
	virtual NodeType type() override { return NodeType::Value; }

	virtual void load(Json json) override;
	virtual void save(Json& json) override;

	f32 value{ 0.0f };
};

class Mul : public Node {
public:
	Mul();
	virtual NodeType type() override { return NodeType::Mul; }
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
	void clear();

	template <class T>
	T* get(u32 id) {
		auto pos = std::find(m_usedNodes.begin(), m_usedNodes.end(), id);
		if (pos == m_usedNodes.end()) return nullptr;
		return dynamic_cast<T*>(m_nodes[id].get());
	}

	u32 connect(u32 src, u32 dest, u32 param);
	void disconnect(u32 connection);
	u32 getConnection(u32 dest, u32 param);
	std::vector<u32> getConnections(u32 dest, u32 param);
	u32 getConnection(u32 src, u32 dest, u32 param);
	u32 getNodeConnection(u32 src, u32 dest);
	std::vector<u32> getAllConnections(u32 node);

	Program compile();

	std::vector<u32> nodes() { return m_usedNodes; }
	std::vector<u32> connections() { return m_usedConnections; }

	Connection* getConnection(u32 id) { return m_connections[id].get(); }

private:
	std::array<std::unique_ptr<Connection>, SynMaxConnections> m_connections;
	std::vector<u32> m_usedConnections;

	std::array<NodePtr, SynMaxNodes> m_nodes;
	std::vector<u32> m_usedNodes;

	std::mutex m_lock;
};

#endif // SYE_NODE_H

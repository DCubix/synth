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
#include "phase.h"
#include "adsr.h"
#include "../log/log.h"

extern "C" {
#include "../../sndfilter/src/compressor.h"
}

constexpr u32 SynMaxNodes = 256;
constexpr u32 SynMaxConnections = 512;
constexpr u32 SynMaxVoices = 16;
constexpr u32 OutputNode = 0;

using Sample = std::pair<f32, f32>;

enum OpCode {
	OpPush = 0,
	OpPushPtr,
	OpSine,
	OpLFO,
	OpOut,
	OpADSR,
	OpMap,
	OpWrite,
	OpRead,
	OpNodeID
};

struct Instruction {
	OpCode opcode;
	f32 param;
	f32* paramPtr;
	u32 nodeID;
};
using Program = std::vector<Instruction>;

class ProgramBuilder {
public:
	inline ProgramBuilder& concat(const Program& initial) {
		m_program.insert(m_program.begin(), initial.begin(), initial.end());
		return *this;
	}
	inline ProgramBuilder& push(u32 id, f32 value) { m_program.push_back({ OpPush, value, nullptr, id }); return *this; }
	inline ProgramBuilder& pushp(u32 id, f32* value) { m_program.push_back({ OpPushPtr, 0, value, id }); return *this; }
	inline ProgramBuilder& sine(u32 id) { m_program.push_back({ OpSine, 0, nullptr, id }); return *this; }
	inline ProgramBuilder& lfo(u32 id) { m_program.push_back({ OpLFO, 0, nullptr, id }); return *this; }
	inline ProgramBuilder& out(u32 id) { m_program.push_back({ OpOut, 0, nullptr, id }); return *this; }
	inline ProgramBuilder& adsr(u32 id) { m_program.push_back({ OpADSR, 0, nullptr, id }); return *this; }
	inline ProgramBuilder& map(u32 id) { m_program.push_back({ OpMap, 0, nullptr, id }); return *this; }
	inline ProgramBuilder& read(u32 id) { m_program.push_back({ OpRead, 0, nullptr, id }); return *this; }
	inline ProgramBuilder& write(u32 id) { m_program.push_back({ OpWrite, 0, nullptr, id }); return *this; }

	Program build() const { return m_program; }

private:
	Program m_program;
};

class SynthVM {
public:
	SynthVM();

	void load(const Program& program);
	Sample execute(f32 sampleRate);

	f32 frequency() const { return m_frequency; }
	void frequency(f32 v) { m_frequency = v; }

	std::array<ADSR, 128>& envelopes() { return m_envs; }
	u32 usedEnvelopes() const { return m_usedEnvs; }

private:
	f32 m_frequency{ 220.0f };

	util::Stack<f32, 1024> m_stack{};

	Program m_program;
	Sample m_out{ 0.0f, 0.0f };

	std::array<Phase, 128> m_phases;
	std::array<ADSR, 128> m_envs;
	std::array<f32, SynMaxNodes> m_storage;
	u32 m_usedEnvs{ 0 };

	std::mutex m_lock;
};

class Voice {
	friend class Synth;
public:
	Voice();

	void setNote(u32 note);

	bool active() const { return m_active; }
	void free() { m_active = false; }

	Sample sample(f32 sampleRate);
	SynthVM& vm() { return *m_vm.get(); }

protected:
	std::unique_ptr<SynthVM> m_vm;
	u32 m_note;
	f32 m_velocity{ 0.0f };
	bool m_active{ false };
};

class Synth {
public:
	Synth();

	void noteOn(u32 noteNumber, f32 velocity = 1.0f);
	void noteOff(u32 noteNumber, f32 velocity = 1.0f);
	Sample sample();

	void setProgram(const Program& program);

	f32 sampleRate() const { return m_sampleRate; }

private:
	std::array< std::unique_ptr<Voice>, SynMaxVoices> m_voices;
	Voice* findFreeVoice();
	Program m_program;

	f32 m_sampleRate{ 44100.0f };

	sf_compressor_state_st m_compressor;
};

enum class NodeType {
	None = 0,
	Value,
	Out,
	SineWave,
	LFO,
	ADSR,
	Map,
	Writer,
	Reader
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

	virtual NodeType type() { return NodeType::None; }
	virtual void load(Json json) { m_level = json.value("level", 1.0f); }
	virtual void save(Json& json) { json["level"] = m_level; }

	float& level() { return m_level; }
	void level(f32 v) {
		m_level = v;
	}

	u32 id() const { return m_id; }

	void addParam(const std::string& name);
	Param& param(u32 id);
	std::string paramName(u32 id);

	u32 paramCount() const { return m_params.size(); }

protected:
	f32 m_level{ 1.0f };
	u32 m_id{ 0 };

	std::vector<Param> m_params;
	std::vector<std::string> m_paramNames;
};
using NodePtr = std::unique_ptr<Node>;

class Output : public Node {
public:
	Output();

	virtual NodeType type() override { return NodeType::Out; }

	float pan() { return param(1).value; }
	void pan(f32 v) { param(1).value = v; }

	virtual void load(Json json) override;
	virtual void save(Json& json) override;
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
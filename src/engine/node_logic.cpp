#include "node_logic.h"
#include "../log/log.h"

#include <numeric>

#include "sine_wave.hpp"
#include "lfo.hpp"

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

f32 Node::vu() {
	return (std::accumulate(m_buffer.begin(), m_buffer.end(), 0.0f) / m_buffer.size()) * 0.5f + 0.5f;
}

void Node::update(f32 val) {
	m_buffer[(m_bufferPtr++) % m_buffer.size()] = val;
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

u32 NodeSystem::getConnection(u32 dest, u32 param) {
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		if (conn->dest == dest && conn->destParam == param) {
			return cid;
		}
	}
	return UINT32_MAX;
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
		if (conn == nullptr) { // it's a broken connection!
			broken.push_back(cid);
			continue;
		}

		Node* src  = m_nodes[conn->src].get();
		Node* dest = m_nodes[conn->dest].get();
		if (src == nullptr || dest == nullptr) { // it's a broken connection!
			broken.push_back(cid);
			continue;
		}

		// Solve
		if (!src->solved) { // to prevent from solving twice
			m_nodeBuffer[conn->src] = src->sample(this) * src->level();
			src->update(m_nodeBuffer[conn->src]);
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

static void process(NodeSystem* sys, Node* node, ProgramBuilder& out) {
	auto type = node->type();
	for (u32 i = 0; i < node->paramCount(); i++) {
		if (!node->param(i).connected) {
			out.pushp(&node->param(i).value);
		} else {
			u32 cid = sys->getConnection(node->id(), i);
			auto conn = sys->getConnection(cid);
			if (conn) {
				Node* nd = sys->get<Node>(conn->src);
				process(sys, nd, out);
			}
		}
	}
	out.pushp(&node->level());
	switch (type) {
		case NodeType::LFO: out.lfo(); break;
		case NodeType::SineWave: out.sine(); break;
		case NodeType::Out: out.out(); break;
		default: break;
	}
}

Program NodeSystem::compile() {
	std::vector<u32> nodes = buildNodes(0);

	ProgramBuilder builder;
	process(this, m_nodes[0].get(), builder);
	return builder.build();
}

std::vector<u32> NodeSystem::getInputs(u32 node) {
	std::vector<u32> ins;
	for (u32 cid : m_usedConnections) {
		Connection* conn = m_connections[cid].get();
		if (conn == nullptr) continue;
		if (conn->dest == node) {
			if (std::find(ins.begin(), ins.end(), conn->src) == ins.end())
				ins.push_back(conn->src);
		}
	}
	return ins;
}

std::vector<u32> NodeSystem::buildNodes(const std::vector<u32>& nodes) {
	if (nodes.empty()) return std::vector<u32>();

	std::vector<u32> nodesg;

	nodesg.insert(nodesg.end(), nodes.begin(), nodes.end());

	for (u32 node : nodes) {
		for (u32 in : getInputs(node)) {
			if (!get<Node>(in)) continue;

			nodesg.push_back(in);

			for (u64 rnd : buildNodes(in)) {
				if (!get<Node>(rnd)) continue;

				if (std::find(nodesg.begin(), nodesg.end(), rnd) == nodesg.end()) {
					nodesg.push_back(rnd);
					LogI("New node found. ", rnd);
				}
			}
		}
	}

	return nodesg;
}

std::vector<u32> NodeSystem::buildNodes(u32 node) {
	std::vector<u32> ret;
	ret.push_back(node);
	return buildNodes(ret);
}

f32 Output::sample(NodeSystem* system) {
	return param(0).value;
}

Program Output::build() {
	return ProgramBuilder()
		.pushp(&m_level)
		.out()
		.build();
}

Output::Output() {
	addParam("Out");
	addParam("Pan");
	param(1).value = 0.5f;
}

static const f32 NOTE[] = {
	27.50f,
	29.14f,
	30.87f,
	32.70f,
	34.65f,
	36.71f,
	38.89f,
	41.20f,
	43.65f,
	46.25f,
	49.00f,
	51.91f
};

static f32 noteFreq(i32 index, i32 octave) {
	return NOTE[index] * std::pow(2, octave);
}

static f32 noteFrequency(i32 index) {
	return noteFreq(index % 12, (index / 12));
}

SynthVM::SynthVM() {
	load(Program());
}

void SynthVM::load(const Program& program) {
	m_lock.lock();
	m_program = program;
	m_stack.clear();
	m_lock.unlock();
}

Sample SynthVM::execute() {
#define POP(x) if (!m_stack.empty()) { x = m_stack.back(); m_stack.pop_back(); }
	
	u32 i = 0;
	for (auto&& ins : m_program) {
		switch (ins.opcode) {
			case OpPush: m_stack.push_back(ins.param); break;
			case OpPushPtr: m_stack.push_back(*ins.paramPtr); break;
			case OpOut: {
				f32 val = 0.0f;
				f32 level = 1.0f;
				f32 pan = 0.5f;
				POP(level);
				POP(pan);
				POP(val);
				m_out = { val * level, val * level };
			} break;
			case OpSine: {
				f32 freqMod = 0.0f;
				f32 offset = 0.0f;
				f32 lvl = 1.0f;
				POP(lvl);
				POP(offset);
				POP(freqMod);
				f32 freqVal = m_phases[i++].advance(m_frequency + offset, m_sampleRate) + freqMod;
				f32 s = ::sinf(freqVal);
				m_stack.push_back(s * lvl);
			} break;
			case OpLFO: {
				f32 freq = 0.0f;
				f32 vmin = 0.0f;
				f32 vmax = 0.0f;
				f32 lvl = 1.0f;
				POP(lvl);
				POP(vmax);
				POP(vmin);
				POP(freq);
				f32 freqVal = m_phases[i++].advance(freq, m_sampleRate);
				f32 s = vmin + (::sinf(freqVal) * 0.5f + 0.5f) * (vmax - vmin);
				m_stack.push_back(s * lvl);
			} break;
		}
	}
	return m_out;
}

void Voice::setNote(u32 note) {
	m_note = note;
	m_vm.frequency(440.0f * std::powf(2.0f, (note - 69.0f) / 12.0f));
}

Sample Voice::sample() {
	if (!m_active) return { 0.0f, 0.0f };
	auto s = m_vm.execute();
	return {
		s.first * m_velocity,
		s.second * m_velocity
	};
}

void Synth::noteOn(u32 noteNumber, f32 velocity) {
	Voice* voice = findFreeVoice();
	if (!voice) return;
	voice->setNote(noteNumber);
	voice->m_velocity = velocity;
	voice->m_active = true;
}

void Synth::noteOff(u32 noteNumber, f32 velocity) {
	for (int i = 0; i < SynMaxVoices; i++) {
		Voice& voice = m_voices[i];
		if (voice.active() && voice.m_note == noteNumber) {
			voice.free();
		}
	}
}

Sample Synth::sample() {
	Sample out{ 0.0f, 0.0f };
	for (int i = 0; i < SynMaxVoices; i++) {
		Voice& voice = m_voices[i];
		auto s = voice.sample();
		out.first += s.first;
		out.second += s.second;
	}
	return out;
}

void Synth::setProgram(const Program& program) { 
	m_program = program;
	for (auto&& voice : m_voices) voice.vm().load(program);
}

Voice* Synth::findFreeVoice() {
	for (auto&& voice : m_voices) {
		if (!voice.active()) return &voice;
	}
	return nullptr;
}

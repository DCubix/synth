#include "node_logic.h"
#include "../log/log.h"

#include <numeric>

#include "sine_wave.hpp"
#include "lfo.hpp"
#include "adsr_node.hpp"
#include "remap.hpp"
#include "storage.hpp"

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
	switch (type) {
		case NodeType::LFO: out.lfo(id); break;
		case NodeType::SineWave: out.sine(id); break;
		case NodeType::Out: out.out(id); break;
		case NodeType::ADSR: out.adsr(id); break;
		case NodeType::Map: out.map(id); break;
		case NodeType::Reader: out.read(id); break;
		case NodeType::Writer: out.write(id); break;
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
	addParam("Pan");
	param(1).value = 0.5f;
}

void Output::load(Json json) {
	Node::load(json);
	pan(json.value("pan", 0.5f));
}

void Output::save(Json& json) {
	Node::save(json);
	json["pan"] = pan();
}

SynthVM::SynthVM() {
	load(Program());
	m_stack.clear();
}

void SynthVM::load(const Program& program) {
	m_lock.lock();
	m_program = program;
	m_stack.clear();
	m_lock.unlock();
	m_storage.fill(0.0f);
}

Sample SynthVM::execute(f32 sampleRate) {
#define PUSH(x) if (m_stack.canPush()) { m_stack.push((x)); }
#define POP(x) if (!m_stack.empty()) { x = m_stack.top(); m_stack.pop(); }
#define POPA(x) if (!m_stack.empty()) { x += m_stack.top(); m_stack.pop(); }
	
	u32 i = 0, e = 0 /*Env*/;
	f32 result = 0.0f;
	for (auto&& ins : m_program) {
		result = m_storage[ins.nodeID];
		switch (ins.opcode) {
			case OpPush: PUSH(ins.param); break;
			case OpPushPtr: PUSH(*ins.paramPtr); break;
			case OpOut: {
				f32 val = 0.0f;
				f32 level = 1.0f;
				f32 pan = 0.5f;
				POP(level);
				POP(pan);

				// mix all the inputs
				while (!m_stack.empty()) {
					POPA(val);
				}
				result = val * level;
				m_out = { result * (1.0f - pan), result * pan };
			} break;
			case OpSine: {
				f32 freqMod = 0.0f;
				f32 offset = 0.0f;
				f32 lvl = 1.0f;
				f32 lvlm = 1.0f;
				POP(lvl);
				POP(lvlm);
				POP(offset);
				//POP(freqMod);
				while (!m_stack.empty()) {
					POPA(freqMod);
				}
				f32 freqVal = m_phases[i++].advance(m_frequency + offset, sampleRate) + freqMod;
				f32 s = ::sinf(freqVal);
				result = s * lvl * lvlm;
				PUSH(result);
			} break;
			case OpLFO: {
				f32 freq = 0.0f;
				f32 vmin = 0.0f;
				f32 vmax = 0.0f;
				f32 lvl = 1.0f, lvlm = 1.0f;
				POP(lvl);
				POP(lvlm);
				POP(freq);
				POP(vmax);
				POP(vmin);
				f32 freqVal = m_phases[i++].advance(freq, sampleRate);
				f32 s = vmin + (::sinf(freqVal) * 0.5f + 0.5f) * (vmax - vmin);
				result = s * lvl * lvlm;
				PUSH(result);
			} break;
			case OpADSR: {
				f32 lvl = 1.0f;
				f32 a = 0.0f, d = 0.0f, s = 1.0f, r = 0.0f;
				POP(lvl);
				POP(r);
				POP(s);
				POP(d);
				POP(a);
				ADSR& env = m_envs[e++];
				env.attack(a * sampleRate);
				env.decay(d * sampleRate);
				env.sustain(s);
				env.release(r * sampleRate);
				result = env.sample() * lvl;
				PUSH(result);
			} break;
			case OpMap: {
				f32 lvl = 1.0f, in = 0.0f;
				f32 fmi = 0.0f, fma = 1.0f, tmi = 0.0f, tma = 1.0f;
				POP(lvl);
				POP(in);
				POP(tma);
				POP(tmi);
				POP(fma);
				POP(fmi);
				f32 norm = (in - fmi) / (fma - fmi);
				result = (norm + tmi) * (tma - tmi);
				PUSH(result);
			} break;
			case OpWrite: {
				f32 lvl = 1.0f, chan = 0.0f, in = 0.0f;
				POP(lvl);
				POP(in);
				POP(chan);
				m_storage[u32(chan) % m_storage.size()] = in * lvl;
				result = m_storage[u32(chan) % m_storage.size()];
				PUSH(result);
			} break;
			case OpRead: {
				f32 lvl = 1.0f, chan = 0.0f;
				POP(lvl);
				POP(chan);
				result = m_storage[u32(chan) % m_storage.size()] * lvl;
				PUSH(result);
			} break;
		}
		// Write result to storage
		m_storage[ins.nodeID] = result;
	}
	m_usedEnvs = e;
	return m_out;
}

ADSR* SynthVM::getLongestEnvelope() {
	f32 time = -1.0f;
	ADSR* env = nullptr;
	for (auto&& aenv : m_envs) {
		if (aenv.totalTime() > time) {
			env = &aenv;
			time = aenv.totalTime();
		}
	}
	return env;
}

Voice::Voice() {
	m_vm = std::make_unique<SynthVM>();
	/*for (auto&& adsr : m_vm->envelopes()) {
		adsr.setOnFinish([&]() {
			free();
		});
	}*/
}

void Voice::setNote(u32 note) {
	m_note = note;
	m_vm->frequency(440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f));
}

Sample Voice::sample(f32 sampleRate) {
	if (!m_active) return { 0.0f, 0.0f };
	auto s = m_vm->execute(sampleRate);
	return {
		s.first * m_velocity,
		s.second * m_velocity
	};
}

Synth::Synth() {
	sf_simplecomp(&m_compressor, int(m_sampleRate), 0.001f, -30.0f, 30.0f, 10.0f, 0.03f, 0.3f);
	for (u32 i = 0; i < SynMaxVoices; i++) {
		m_voices[i] = std::make_unique<Voice>();
	}
}

void Synth::noteOn(u32 noteNumber, f32 velocity) {
	Voice* voice = findFreeVoice();
	if (!voice) return;
	voice->setNote(noteNumber);
	voice->m_velocity = velocity;
	voice->m_active = true;
	for (auto&& env : voice->vm().envelopes()) {
		env.active = true;
		env.gate(true);
	}
}

void Synth::noteOff(u32 noteNumber, f32 velocity) {
	for (u32 i = 0; i < SynMaxVoices; i++) {
		Voice& voice = *m_voices[i].get();
		if (voice.active() && voice.m_note == noteNumber) {
			if (voice.vm().usedEnvelopes() == 0) voice.free();
			else {
				for (u32 e = 0; e < voice.vm().usedEnvelopes(); e++) {
					voice.vm().envelopes()[e].gate(false);
				}
			}
		}
	}
}

Sample Synth::sample() {
	Sample out{ 0.0f, 0.0f };
	for (int i = 0; i < SynMaxVoices; i++) {
		Voice& voice = *m_voices[i].get();
		auto s = voice.sample(m_sampleRate);

		if (!voice.vm().getLongestEnvelope()->active) voice.free();

		out.first += s.first;
		out.second += s.second;
	}

	// Compressor
	sf_sample_st sin, sout;
	sin.L = out.first;
	sin.R = out.second;
	sout.L = out.first;
	sout.R = out.second;
	sf_compressor_process(&m_compressor, 1, &sin, &sout);

	return { sout.L, sout.R };
}

void Synth::setProgram(const Program& program) { 
	m_program = program;
	for (auto&& voice : m_voices) voice->vm().load(program);
}

Voice* Synth::findFreeVoice() {
	for (auto&& voice : m_voices) {
		if (!voice->active()) return voice.get();
	}
	return nullptr;
}

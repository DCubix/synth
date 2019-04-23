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
	m_nodeBuffer.fill(0.0f);
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

static void process(NodeSystem* sys, Node* node, ProgramBuilder& out) {
	auto type = node->type();

	switch (type) {
		case NodeType::ADSR: {
			ADSRNode* an = (ADSRNode*)node;
			out.pushp(&an->a);
			out.pushp(&an->d);
			out.pushp(&an->s);
			out.pushp(&an->r);
		} break;
		case NodeType::Map: {
			Map* mn = (Map*)node;
			out.pushp(&mn->fromMin);
			out.pushp(&mn->fromMax);
			out.pushp(&mn->toMin);
			out.pushp(&mn->toMax);
		} break;
		case NodeType::Reader: {
			Reader* mn = (Reader*)node;
			out.pushp(&mn->channel);
		} break;
		case NodeType::Writer: {
			Writer* mn = (Writer*)node;
			out.pushp(&mn->channel);
		} break;
		default: break;
	}

	for (u32 i = 0; i < node->paramCount(); i++) {
		if (!node->param(i).connected) {
			out.pushp(&node->param(i).value);
		} else {
			auto&& conns = sys->getConnections(node->id(), i);
			for (u32 cid : conns) {
				auto conn = sys->getConnection(cid);
				if (conn) {
					Node* nd = sys->get<Node>(conn->src);
					process(sys, nd, out);
				}
			}
		}
	}
	out.pushp(&node->level());
	switch (type) {
		case NodeType::LFO: out.lfo(); break;
		case NodeType::SineWave: out.sine(); break;
		case NodeType::Out: out.out(); break;
		case NodeType::ADSR: out.adsr(); break;
		case NodeType::Map: out.map(); break;
		case NodeType::Reader: out.read(); break;
		case NodeType::Writer: out.write(); break;
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

Output::Output() {
	addParam("Out");
	addParam("Pan");
	param(1).value = 0.5f;
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
	
	u32 i = 0, e = 0 /*Env*/;
	for (auto&& ins : m_program) {
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
				f32 valmix = 0.0f;
				while (!m_stack.empty()) {
					POP(val);
					valmix += val;
				}
				m_out = { valmix * level, valmix * level };
			} break;
			case OpSine: {
				f32 freqMod = 0.0f;
				f32 offset = 0.0f;
				f32 lvl = 1.0f;
				f32 lvlm = 1.0f;
				POP(lvl);
				POP(lvlm);
				POP(offset);
				POP(freqMod);
				f32 freqVal = m_phases[i++].advance(m_frequency + offset, sampleRate) + freqMod;
				f32 s = ::sinf(freqVal);
				PUSH(s * lvl * lvlm);
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
				f32 freqVal = m_phases[i++].advance(freq, sampleRate);
				f32 s = vmin + (::sinf(freqVal) * 0.5f + 0.5f) * (vmax - vmin);
				PUSH(s * lvl);
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
				env.sustain(s * sampleRate);
				env.release(r * sampleRate);
				PUSH(env.sample() * lvl);
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
				PUSH((norm + tmi) * (tma - tmi));
			} break;
			case OpWrite: {
				f32 lvl = 1.0f, chan = 0.0f, in = 0.0f;
				POP(lvl);
				POP(in);
				POP(chan);
				m_storage[u32(chan) % m_storage.size()] = in * lvl;
				PUSH(m_storage[u32(chan) % m_storage.size()]);
			} break;
			case OpRead: {
				f32 lvl = 1.0f, chan = 0.0f;
				POP(lvl);
				POP(chan);
				PUSH(m_storage[u32(chan) % m_storage.size()] * lvl);
			} break;
		}
	}
	m_usedEnvs = e;
	return m_out;
}

Voice::Voice() {
	m_vm = std::make_unique<SynthVM>();
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

		bool done = true;
		for (auto&& env : voice.vm().envelopes()) {
			if (env.active) done = false; break;
		}

		if (done) voice.free();

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

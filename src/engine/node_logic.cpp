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

SynthVM::SynthVM() {
	load(Program());
}

void SynthVM::load(const Program& program) {
	m_lock.lock();
	m_program = program;
	m_lock.unlock();
}

Sample SynthVM::execute(f32 sampleRate, u32 channel) {
#define PUSH(x) if (m_stack.canPush()) m_stack.push((x))
#define PUSHf(x) if (m_stack.canPush()) m_stack.push(Sample(x))
#define POP(x) if (!m_stack.empty()) { x = m_stack.top(); m_stack.pop(); }
#define POPf(x) if (!m_stack.empty()) { x = m_stack.top().L; m_stack.pop(); }
#define POPA(x) if (!m_stack.empty()) { x += m_stack.top(); m_stack.pop(); }
#define POPAf(x) if (!m_stack.empty()) { x += m_stack.top().L; m_stack.pop(); }

	u32 i = 0 /* Phase */, e = 0 /* Env */;
	u32 pc = 0;
	Sample result{ 0.0f };
	f32 level = 1.0f, pan = 0.5f;

	while (pc < m_program.size()) {
		Instruction& ins = m_program[pc++];
		result = m_storage[ins.nodeID];
		switch (ins.opcode) {
			case OpPush: PUSH(ins.param); break;
			case OpPushPtr: PUSH(*ins.paramPtr); break;
			case OpOut: {
				POPf(pan);
				POPf(level);
				Sample val{ 0.0f };

				// mix all the inputs
				while (!m_stack.empty()) {
					POPA(val);
				}
				result.L = val.L * level * (1.0f - pan);
				result.R = val.R * level * pan;
				m_out = result;
			} break;
			case OpSine: {
				Sample freqMod{ 0.0f };
				f32 offset = 0.0f;
				f32 lvlm = 1.0f;
				POPf(pan);
				POPf(level);
				POPf(lvlm);
				POPf(offset);
				while (!m_stack.empty()) {
					POPA(freqMod);
				}
				f32 p = m_phases[i++].advance(m_frequency + offset, sampleRate);
				f32 freqL = p + freqMod.L;
				f32 freqR = p + freqMod.R;
				f32 sL = ::cosf(freqL);
				f32 sR = ::cosf(freqR);
				result.L = sL * level * lvlm * (1.0f - pan);
				result.R = sR * level * lvlm * pan;
				PUSH(result);
			} break;
			case OpLFO: {
				f32 freq = 0.0f;
				f32 vmin = 0.0f;
				f32 vmax = 0.0f;
				f32 lvlm = 1.0f;
				POPf(pan);
				POPf(level);
				POPf(lvlm);
				POPf(freq);
				POPf(vmax);
				POPf(vmin);
				f32 freqVal = m_phases[i++].advance(freq, sampleRate);
				f32 s = vmin + (::sinf(freqVal) * 0.5f + 0.5f) * (vmax - vmin);
				result.L = s * level * lvlm * (1.0f - pan);
				result.R = s * level * lvlm * pan;
				PUSH(result);
			} break;
			case OpADSR: {
				f32 a = 0.0f, d = 0.0f, s = 1.0f, r = 0.0f;
				POPf(pan);
				POPf(level);
				POPf(r);
				POPf(s);
				POPf(d);
				POPf(a);
				ADSR& env = m_envs[e++];
				env.attack(a * sampleRate);
				env.decay(d * sampleRate);
				env.sustain(s);
				env.release(r * sampleRate);
				result.L = result.R = env.sample() * level;
				PUSH(result);
			} break;
			case OpMap: {
				Sample in{ 0.0f };
				f32 fmi = 0.0f, fma = 1.0f, tmi = 0.0f, tma = 1.0f;
				POPf(pan);
				POPf(level);
				POP(in);
				POPf(tma);
				POPf(tmi);
				POPf(fma);
				POPf(fmi);
				f32 normL = (in.L - fmi) / (fma - fmi);
				f32 normR = (in.R - fmi) / (fma - fmi);
				result.L = (normL + tmi) * (tma - tmi);
				result.R = (normR + tmi) * (tma - tmi);
				PUSH(result);
			} break;
			case OpWrite: {
				f32 chan = 0.0f;
				Sample in{ 0.0f };
				POPf(pan);
				POPf(level);
				POP(in);
				POPf(chan);
				m_storage[u32(chan) % m_storage.size()] = in * level;
				result = m_storage[u32(chan) % m_storage.size()];
				PUSH(result);
			} break;
			case OpRead: {
				f32 chan = 0.0f;
				POPf(pan);
				POPf(level);
				POPf(chan);
				result = m_storage[u32(chan) % m_storage.size()] * level;
				PUSH(result);
			} break;
			case OpValue: {
				f32 val = 0.0f;
				POPf(pan);
				POPf(level);
				POPf(val);
				result.L = result.R = val * level;
				PUSH(result);
			} break;
			case OpMul: {
				f32 a = 0.0f, b = 0.0f;
				POPf(pan);
				POPf(level);
				POPf(b);
				POPf(a);
				result.L = result.R = (a * b) * level;
				PUSH(result);
			} break;
		}
		// Write result to storage
		m_storage[ins.nodeID] = result;
	}
	m_usedEnvs = e;

	if (e > 0) {
		f32 time = -1.0f;
		m_longestEnv = nullptr;
		for (u32 i = 0; i < e; i++) {
			if (m_envs[i].totalTime() > time) {
				time = m_envs[i].totalTime();
				m_longestEnv = &m_envs[i];
			}
		}
	}

	return m_out;
}

ADSR* SynthVM::getLongestEnvelope() {
	return m_longestEnv;
}

Voice::Voice() {
	m_vm = std::make_unique<SynthVM>();
}

void Voice::setNote(u32 note) {
	m_note = note;
	m_vm->frequency(440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f));
}

Sample Voice::sample(f32 sampleRate) {
	Sample s = m_vm->execute(sampleRate);
	return {
		s.L * m_velocity,
		s.R * m_velocity
	};
}

Synth::Synth() {
	sf_simplecomp(&m_compressor, int(m_sampleRate), 0.001f, -30.0f, 30.0f, 10.0f, 0.03f, 0.3f);
	for (u32 i = 0; i < SynMaxVoices; i++) {
		m_voices[i] = std::make_unique<Voice>();
	}
}

void Synth::noteOn(u32 noteNumber, f32 velocity) {
	Voice* voice = findFreeVoice(noteNumber);
	if (!voice) return;

	voice->m_velocity = velocity;
	voice->m_active = true;
	if (voice->m_note == noteNumber) {
		LogW("!!Retrigger Voice!!");
		for (auto&& env : voice->vm().envelopes()) {
			env.active = true;
			env.gate(false);
			env.reset();
			env.gate(true);
		}
	} else {
		for (auto&& env : voice->vm().envelopes()) {
			env.active = true;
			env.gate(true);
		}
	}
	voice->setNote(noteNumber);
}

void Synth::noteOff(u32 noteNumber, f32 velocity) {
	Voice* voice = getVoice(noteNumber);
	if (!voice) return;
	if (!m_sustain) {
		if (voice->vm().usedEnvelopes() == 0) voice->free();
		else {
			for (u32 e = 0; e < voice->vm().usedEnvelopes(); e++) {
				voice->vm().envelopes()[e].gate(false);
			}
		}
	} else {
		voice->m_sustained = true;
	}
}

Sample Synth::sample() {
	Sample out{ 0.0f, 0.0f };
	for (int i = 0; i < SynMaxVoices; i++) {
		Voice& voice = *m_voices[i].get();
		if (!voice.active()) continue;

		auto s = voice.sample(m_sampleRate);

		auto env = voice.vm().getLongestEnvelope();
		if (env && !env->active) {
			voice.free();
			LogI("Freed voice #", i);
		}

		out.L += s.L;
		out.R += s.R;
	}

	// Chorus
	if (m_chorusEnabled) {
		out.L = m_chorus.process(out.L, m_sampleRate, 0);
		out.R = m_chorus.process(out.R, m_sampleRate, 1);
	}

	// Compressor
	sf_sample_st sin, sout;
	sin.L = out.L;
	sin.R = out.R;
	sout.L = out.L;
	sout.R = out.R;
	sf_compressor_process(&m_compressor, 1, &sin, &sout);

	return { sout.L, sout.R };
}

void Synth::setProgram(const Program& program) {
	m_program = program;
	for (auto&& voice : m_voices) voice->vm().load(program);
}

void Synth::sustainOn() {
	m_sustain = true;
}

void Synth::sustainOff() {
	m_sustain = false;
	for (u32 i = 0; i < SynMaxVoices; i++) {
		Voice& voice = *m_voices[i].get();
		if (!voice.m_sustained) continue;
		for (u32 e = 0; e < voice.vm().usedEnvelopes(); e++) {
			voice.vm().envelopes()[e].gate(false);
			voice.m_sustained = false;
		}
	}
}

Voice* Synth::findFreeVoice(u32 note) {
	Voice* freeVoice = nullptr;
	for (auto&& voice : m_voices) {
		if (!voice->active()) {
			freeVoice = voice.get();
			break;
		} else if (voice->active() && voice->m_note == note) {
			freeVoice = voice.get();
			break;
		}
	}
	if (freeVoice == nullptr) {
		freeVoice = m_voices[0].get();
	}
	return freeVoice;
}

Voice* Synth::getVoice(u32 note) {
	Voice* voc = nullptr;
	for (auto&& voice : m_voices) {
		if (voice->active() && voice->m_note == note) {
			voc = voice.get();
			break;
		}
	}
	return voc;
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

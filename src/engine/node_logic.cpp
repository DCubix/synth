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
	m_longestEnv = nullptr;
	m_valueStack.clear(0.0f);
	m_resultStack.clear(Float8{ 0.0f });
}

void SynthVM::load(const Program& program) {
	m_lock.lock();
	m_program = program;
	m_lock.unlock();
}

Float8 SynthVM::execute(f32 sampleRate, f32 frequency, u32 channel) {
#define PAN ((channel == 0) ? 1.0f - pan : pan);

	u32 i = 0 /* Phase */,
		e = 0 /* Env */;
	u32 pc = 0;

	Float8 result;
	f32 level = 1.0f, pan = 0.5f, coef = 0.0f;

	while (pc < m_program.size()) {
		Instruction& ins = m_program[pc++];
		result = m_storage[ins.nodeID];

		pan = m_valueStack.pop().value_or(0.5f);
		level = m_valueStack.pop().value_or(1.0f);
		coef = level * PAN;

		switch (ins.opcode) {
			case OpPush: m_valueStack.push(ins.param); break;
			case OpPushPtr: m_valueStack.push(*ins.paramPtr); break;
			case OpOut: {
				Float8 ret{ 0.0f };
				while (!m_resultStack.empty()) {
					Float8 res = m_resultStack.pop().value();
					for (u32 i = 0; i < SynSampleCount; i++) {
						ret[i] += res[i] * coef;
					}
				}
				result = ret;
				m_out = result;
			} break;
			case OpSine: {
				Float8 freqMod{ 0.0f };
				f32 lvlm = m_valueStack.pop().value_or(1.0f);
				f32 offset = m_valueStack.pop().value_or(0.0f);
				while (!m_resultStack.empty()) {
					Float8 res = m_resultStack.pop().value();
					for (u32 i = 0; i < SynSampleCount; i++) {
						freqMod[i] += res[i] * coef;
					}
				}

				Phase& phase = m_phases[i++];
				for (u32 i = 0; i < SynSampleCount; i++) {
					f32 p = phase.advance(frequency + offset, sampleRate);
					result[i] = std::sin(p + freqMod[i]) * coef * lvlm;
				}
				m_resultStack.push(result);
			} break;
			case OpLFO: {
				f32 lvlm = m_valueStack.pop().value_or(1.0f);
				f32 freq = m_valueStack.pop().value_or(0.0f);
				f32 vmax = m_valueStack.pop().value_or(1.0f);
				f32 vmin = m_valueStack.pop().value_or(0.0f);

				Phase& phase = m_phases[i++];
				for (u32 i = 0; i < SynSampleCount; i++) {
					f32 p = phase.advance(freq, sampleRate);
					f32 s = vmin + (std::sin(p) * 0.5f + 0.5f) * (vmax - vmin);
					result[i] = s * level * lvlm;
				}
				m_resultStack.push(result);
			} break;
			case OpADSR: {
				f32 r = m_valueStack.pop().value_or(0.0f);
				f32 s = m_valueStack.pop().value_or(1.0f);
				f32 d = m_valueStack.pop().value_or(0.0f);
				f32 a = m_valueStack.pop().value_or(0.0f);
				ADSR& env = m_envs[e++];
				env.attack(a * sampleRate);
				env.decay(d * sampleRate);
				env.sustain(s);
				env.release(r * sampleRate);

				for (u32 i = 0; i < SynSampleCount; i++) {
					result[i] = env.sample() * level;
				}
				m_resultStack.push(result);
			} break;
			case OpMap: {
				Float8 in = m_resultStack.pop().value_or(Float8{ 0.0f });
				f32 tma = m_valueStack.pop().value_or(1.0f);
				f32 tmi = m_valueStack.pop().value_or(0.0f);
				f32 fma = m_valueStack.pop().value_or(1.0f);
				f32 fmi = m_valueStack.pop().value_or(0.0f);

				for (u32 i = 0; i < SynSampleCount; i++) {
					f32 norm = (in[i] - fmi) / (fma - fmi);
					result[i] = (norm + tmi) * (tma - tmi);
				}
				m_resultStack.push(result);
			} break;
			case OpWrite: {
				Float8 in = m_resultStack.pop().value_or(Float8{ 0.0f });
				f32 chan = m_valueStack.pop().value_or(0.0f);
				u32 idx = u32(chan) % m_storage.size();
				result = m_storage[idx] = in;
				m_resultStack.push(m_storage[idx]);
			} break;
			case OpRead: {
				f32 chan = m_valueStack.pop().value_or(0.0f);
				u32 idx = u32(chan) % m_storage.size();
				result = m_storage[idx];
				m_resultStack.push(result);
			} break;
			case OpValue: {
				f32 val = m_valueStack.pop().value_or(0.0f);
				result.fill(val);
				m_resultStack.push(result);
			} break;
			case OpMul: {
				Float8 b = m_resultStack.pop().value_or(Float8{ 0.0f });
				Float8 a = m_resultStack.pop().value_or(Float8{ 0.0f });
				for (u32 i = 0; i < SynSampleCount; i++) {
					result[i] = a[i] * b[i];
				}
				m_resultStack.push(result);
			} break;
		}
		// Write result to storage
		m_storage[ins.nodeID] = result;
	}
	m_usedEnvs = e;

	m_longestEnv = nullptr;
	if (e > 0) {
		f32 time = -1.0f;
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

void Voice::setNote(u32 note) {
	this->note = note;
	this->frequency = (440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f));
}

Synth::Synth() {
	sf_simplecomp(&m_compressor, int(m_sampleRate), 0.001f, -30.0f, 30.0f, 10.0f, 0.03f, 0.3f);
	for (u32 i = 0; i < SynMaxVoices; i++) {
		m_vms[i] = std::make_unique<SynthVM>();
		m_voices[i].index = i;
	}
}

void Synth::noteOn(u32 noteNumber, f32 velocity) {
	Voice* voice = findFreeVoice(noteNumber);
	if (!voice) return;

	voice->velocity = velocity;
	voice->active = true;
	if (voice->note == noteNumber) {
		//LogW("!!Retrigger Voice!!");
		for (auto&& env : m_vms[voice->index]->envelopes()) {
			env.active = true;
			env.gate(false);
			env.reset();
			env.gate(true);
		}
	} else {
		for (auto&& env : m_vms[voice->index]->envelopes()) {
			env.active = true;
			env.gate(true);
		}
	}
	voice->setNote(noteNumber);
	voice->sustained = m_sustain;
}

void Synth::noteOff(u32 noteNumber, f32 velocity) {
	Voice* voice = getVoice(noteNumber);
	if (!voice) return;

	SynthVM* vm = m_vms[voice->index].get();
	if (!voice->sustained) {
		if (vm->usedEnvelopes() == 0) voice->free();
		else {
			for (u32 e = 0; e < vm->usedEnvelopes(); e++) {
				vm->envelopes()[e].gate(false);
			}
		}
	}
}

Float8 Synth::sample(u32 channel) {
	Float8 out{ 0.0f };
	for (int i = 0; i < SynMaxVoices; i++) {
		Voice* voice = &m_voices[i];
		if (!voice->active && !voice->sustained) continue;

		SynthVM* vm = m_vms[voice->index].get();
		Float8 s = vm->execute(m_sampleRate, voice->frequency, channel);

		ADSR* env = vm->getLongestEnvelope();
		if (env && !env->active) {
			voice->free();
			//LogI("Freed voice #", i);
		}

		for (u32 i = 0; i < SynSampleCount; i++) {
			out[i] += s[i];
		}
	}

	// Chorus
	if (m_chorusEnabled) {
		for (u32 i = 0; i < SynSampleCount; i++) {
			out[i] += m_chorus.process(out[i], m_sampleRate, channel);
		}
	}

	return out;
}

void Synth::setProgram(const Program& program) {
	m_program = program;
	for (u32 i = 0; i < m_vms.size(); i++)
		m_vms[i]->load(program);
}

void Synth::sustainOn() {
	m_sustain = true;
	for (u32 i = 0; i < SynMaxVoices; i++) {
		Voice* voice = &m_voices[i];
		if (voice->active) voice->sustained = true;
	}
}

void Synth::sustainOff() {
	m_sustain = false;
	for (u32 i = 0; i < SynMaxVoices; i++) {
		Voice* voice = &m_voices[i];
		if (voice->active) continue;

		SynthVM* vm = m_vms[voice->index].get();
		if (vm->usedEnvelopes() == 0) voice->free();
		else {
			for (u32 e = 0; e < vm->usedEnvelopes(); e++) {
				vm->envelopes()[e].gate(false);
			}
		}
		voice->sustained = false;
	}
}

Voice* Synth::findFreeVoice(u32 note) {
	Voice* freeVoice = nullptr;
	for (auto&& voice : m_voices) {
		if (!voice.active) {
			freeVoice = &voice;
			break;
		} else if (voice.active && voice.note == note) {
			freeVoice = &voice;
			break;
		}
	}
	if (freeVoice == nullptr) {
		freeVoice = &m_voices[0];
	}
	return freeVoice;
}

Voice* Synth::getVoice(u32 note) {
	Voice* voc = nullptr;
	for (auto&& voice : m_voices) {
		if (voice.active && voice.note == note) {
			voc = &voice;
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

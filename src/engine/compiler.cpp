#include "compiler.h"

ProgramBuilder& ProgramBuilder::concat(const Program& initial) {
	m_program.insert(m_program.begin(), initial.begin(), initial.end());
	return *this;
}

ProgramBuilder& ProgramBuilder::push(u32 id, f32 value) {
	m_program.push_back({ OpPush, value, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::pushp(u32 id, f32* value) {
	m_program.push_back({ OpPushPtr, 0, value, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::sine(u32 id) {
	m_program.push_back({ OpSine, 0, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::lfo(u32 id) {
	m_program.push_back({ OpLFO, 0, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::out(u32 id) {
	m_program.push_back({ OpOut, 0, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::adsr(u32 id) {
	m_program.push_back({ OpADSR, 0, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::map(u32 id) {
	m_program.push_back({ OpMap, 0, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::read(u32 id) {
	m_program.push_back({ OpRead, 0, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::write(u32 id) {
	m_program.push_back({ OpWrite, 0, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::value(u32 id, f32 value) {
	m_program.push_back({ OpValue, value, nullptr, id });
	return *this;
}

ProgramBuilder& ProgramBuilder::mul(u32 id) {
	m_program.push_back({ OpMul, 0.0f, nullptr, id });
	return *this;
}

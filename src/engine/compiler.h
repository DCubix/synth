#ifndef SYN_COMPILER_H
#define SYN_COMPILER_H

#include "../common.h"

constexpr u32 SynMaxNodes = 256;
constexpr u32 SynMaxConnections = 512;

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
	OpValue,
	OpMul
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
	ProgramBuilder& concat(const Program& initial);
	ProgramBuilder& push(u32 id, f32 value);
	ProgramBuilder& pushp(u32 id, f32* value);
	ProgramBuilder& sine(u32 id);
	ProgramBuilder& lfo(u32 id);
	ProgramBuilder& out(u32 id);
	ProgramBuilder& adsr(u32 id);
	ProgramBuilder& map(u32 id);
	ProgramBuilder& read(u32 id);
	ProgramBuilder& write(u32 id);
	ProgramBuilder& value(u32 id, f32 value);
	ProgramBuilder& mul(u32 id);

	Program build() const { return m_program; }

private:
	Program m_program;
};

class Compiler {
private:

};

#endif // COMPILER_H

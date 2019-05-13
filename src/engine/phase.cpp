#include "phase.h"

Phase::Phase(f32 period) {
	m_phase = 0.0f;
	m_period = period;
}

f32 Phase::advance(f32 freq, f32 sampleRate) {
	m_phase += freq * (PI * 2.0f / sampleRate);
	m_phase = std::fmod(m_phase, m_period);
	return m_phase;
}

void Phase::reset() {
	m_phase = 0.0f;
}

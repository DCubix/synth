#ifndef SY_PHASE_H
#define SY_PHASE_H

#include "../common.h"

class Phase {
public:
	Phase(f32 period = PI * 2.0f);

	f32 advance(f32 freq, f32 sampleRate);
	void reset();

private:
	f32 m_phase, m_period;
};

#endif // SY_PHASE_H
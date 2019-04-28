#ifndef SY_PHASE_H
#define SY_PHASE_H

#include "../common.h"

class Phase {
public:
	Phase(f32 period = PI * 2.0f);

	f32 advance(f32 freq, f32 sampleRate);
	void reset();

	bool active() const { return m_active; }
	void active(bool v) { m_active = v; }

	f32 value() const { return m_phase; }

private:
	f32 m_phase, m_period;
	bool m_active{ true };
};

#endif // SY_PHASE_H
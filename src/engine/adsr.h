#ifndef SY_ADSR_H
#define SY_ADSR_H

#include <functional>

#include "../common.h"

class ADSR {
public:
	enum State {
		Idle = 0,
		Attack,
		Decay,
		Sustain,
		Release
	};

	ADSR()
		: m_attack(0.0f), m_decay(0.0f), m_sustain(1.0f), m_release(0.0f),
		m_targetRatioA(0.3f), m_targetRatioDR(0.0001f), m_out(0.0f)
	{}

	ADSR(f32 a, f32 d, f32 s, f32 r)
		: m_targetRatioA(0.3f), m_targetRatioDR(0.0001f), m_out(0.0f)
	{
		attack(a);
		decay(d);
		sustain(s);
		release(r);
	}

	f32 attack() const { return m_attack; }
	void attack(f32 v) {
		m_attack = v;
		m_attackCoef = coef(v, m_targetRatioA);
		m_attackBase = (1.0f + m_targetRatioA) * (1.0f - m_attackCoef);
	}

	f32 decay() const { return m_decay; }
	void decay(f32 v) {
		m_decay = v;
		m_decayCoef = coef(v, m_targetRatioDR);
		m_decayBase = (m_sustain - m_targetRatioDR) * (1.0f - m_decayCoef);
	}

	f32 release() const { return m_release; }
	void release(f32 v) {
		m_release = v;
		m_releaseCoef = coef(v, m_targetRatioDR);
		m_releaseBase = -m_targetRatioDR * (1.0f - m_releaseCoef);
	}

	f32 sustain() const { return m_sustain; }
	void sustain(f32 v) {
		m_sustain = v;
		m_decayBase = (m_sustain - m_targetRatioDR) * (1.0f - m_decayCoef);
	}

	f32 targetRatioA() const { return m_targetRatioA; }
	void targetRatioA(f32 v) {
		if (v < 0.000000001f)
			v = 0.000000001f;  // -180 dB
		m_targetRatioA = v;
		m_attackCoef = coef(m_attack, m_targetRatioA);
		m_attackBase = (1.0f + m_targetRatioA) * (1.0f - m_attackCoef);
	}

	f32 targetRatioDR() const { return m_targetRatioDR; }
	void targetRatioDR(f32 v) {
		if (v < 0.000000001f)
			v = 0.000000001f;  // -180 dB
		m_targetRatioDR = v;
		m_decayCoef = coef(m_decay, m_targetRatioDR);
		m_releaseCoef = coef(m_release, m_targetRatioDR);
		m_decayBase = (m_sustain - m_targetRatioDR) * (1.0f - m_decayCoef);
		m_releaseBase = -m_targetRatioDR * (1.0f - m_releaseCoef);
	}

	void gate(bool g);
	f32 sample();

	void reset();
	bool active{ false };

	void setOnFinish(const std::function<void()>& cb) { m_onFinish = cb; }

	f32 totalTime() const { return m_attack + m_release + m_decay; }

private:
	State m_state;
	f32 m_attack,
		m_decay,
		m_release,
		m_sustain,
		m_attackBase,
		m_decayBase,
		m_releaseBase,
		m_attackCoef,
		m_decayCoef,
		m_releaseCoef,
		m_targetRatioA,
		m_targetRatioDR,
		m_out;

	f32 coef(f32 rate, f32 targetRatio);

	std::function<void()> m_onFinish;

};

#endif // SY_ADSR_H
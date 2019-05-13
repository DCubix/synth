#ifndef SYN_CHORUS_H
#define SYN_CHORUS_H

#include "../common.h"
#include "phase.h"
#include "wave_guide.h"

class Chorus {
public:
	~Chorus() = default;

	f32 process(f32 in, f32 sampleRate, u32 channel);

	f32 delay{ 30.0f }, width{ 20.0f }, depth{ 1.0f }, freq{ 0.2f };
	f32 numVoices{ 3 };

private:
	Phase m_phase{ 2.0f * PI };
	WaveGuide m_wg{};
	f32 m_phaseOff{ 0.0f };
};

#endif // SYN_CHORUS_H

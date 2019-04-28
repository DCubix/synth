#ifndef SYN_WAVE_GUIDE_H
#define SYN_WAVE_GUIDE_H

#include "../common.h"

#define WAVE_GUIDE_SAMPLES 22050
class WaveGuide {
public:
	WaveGuide() : m_counter(0) { m_buffer.fill(0); }

	void clear();
	float process(f32 in, f32 feedBack, f32 delay);

private:
	std::array<f32, WAVE_GUIDE_SAMPLES> m_buffer;
	i32 m_counter{ 0 };
};

#endif // SYN_WAVE_GUIDE_H
#include "wave_guide.h"

void WaveGuide::clear() {
	m_counter = 0;
	m_buffer.fill(0.0f);
}

f32 WaveGuide::process(f32 in, f32 feedBack, f32 delay) {
	// calculate delay offset
	f32 readPos = ::fmod(f32(m_counter) - delay + f32(WAVE_GUIDE_SAMPLES), WAVE_GUIDE_SAMPLES);
	i32 back = ::floor(readPos);

	f32 fraction = readPos - f32(back);
	f32 fractionSqrt = fraction * fraction;
	f32 fractionCube = fractionSqrt * fraction;

	f32 sample0 = m_buffer[(back - 1 + WAVE_GUIDE_SAMPLES) % WAVE_GUIDE_SAMPLES];
	f32 sample1 = m_buffer[(back + 0)];
	f32 sample2 = m_buffer[(back + 1) % WAVE_GUIDE_SAMPLES];
	f32 sample3 = m_buffer[(back + 2) % WAVE_GUIDE_SAMPLES];

	f32 a0 = -0.5f * sample0 + 1.5f * sample1 - 1.5f * sample2 + 0.5f * sample3;
	f32 a1 = sample0 - 2.5f * sample1 + 2.0f * sample2 - 0.5f * sample3;
	f32 a2 = -0.5f * sample0 + 0.5f * sample2;
	f32 a3 = sample1;
	f32 output = a0 * fractionCube + a1 * fractionSqrt + a2 * fraction + a3;

	// add to delay buffer
	m_buffer[m_counter] = in + output * feedBack;
	if (++m_counter >= WAVE_GUIDE_SAMPLES) m_counter -= WAVE_GUIDE_SAMPLES;

	// return output
	return output;
}
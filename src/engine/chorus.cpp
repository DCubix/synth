#include "chorus.h"

f32 Chorus::process(f32 in, f32 sampleRate, u32 channel) {
	f32 weight = 1.0f;
	f32 result = in;
	f32 out = 0.0f;

	m_phaseOff = 0.0f;

	f32 d = delay * 0.001f;
	f32 w = width * 0.001f;

	f32 phase = m_phase.value();
	for (u32 voice = 0; voice < numVoices - 1; voice++) {
		if (numVoices > 2) {
			weight = f32(voice) / f32(numVoices - 2);
			if (channel != 0) weight = 1.0f - weight;
		} else {
			weight = 1.0f;
		}

		f32 lfo = ::sinf(phase + m_phaseOff) * 0.5f + 0.5f;
		f32 localDelayTime = (d + w * lfo) * sampleRate;

		out = m_wg.process(in, 0.0f, localDelayTime);

		if		(numVoices == 2) result = (channel == 0) ? in : out * depth;
		else					 result += out * depth * weight;

		if		(numVoices == 3) m_phaseOff += 0.25f;
		else if (numVoices > 3)  m_phaseOff += 1.0f / f32(numVoices - 1);
	}
	m_phase.advance(freq, sampleRate);

	return result;
}
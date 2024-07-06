#include "music.hh"

#include <iostream>
#include <functional>
#include <numbers>
#include <bit>
#include <cstdint>
#include <cmath>
#include "notes.hh"

static constexpr uint64_t sr = 48000;

class Samples
{
public:
	Samples();
	~Samples();
	float &operator[](uint64_t i);
	const float operator[](uint64_t i) const;
	constexpr uint64_t length() const;
private:
	float *s;
	uint64_t real_size;
	uint64_t size;
	void accomodate(uint64_t i);
};

Samples::Samples()
{
	real_size = 65536;
	size = 0;
	s = new float[real_size]{ 0 };
}

Samples::~Samples()
{
	delete[] s;
}

void Samples::accomodate(uint64_t i)
{
	if (i >= real_size)
	{
		real_size = std::max(i + 1, real_size * 2);
		float *new_s = new float[real_size]{ 0 };
		for (uint64_t j = 0; j < size; j++)
			new_s[j] = s[j];
		delete[] s;
		s = new_s;
	}
	size = i + 1;
}

float &Samples::operator[](uint64_t i)
{
	accomodate(i);
	return s[i];
}

const float Samples::operator[](uint64_t i) const
{
	if (i < size) return s[i];
	return 0.0f;
}

constexpr uint64_t Samples::length() const
{
	return size;
}

static void dump(const Samples &s)
{
	for (uint64_t i = 0; i < s.length(); i++)
	{
		int16_t v = std::max(std::min(s[i], 1.0f), -1.0f) * 32767.0f;
		std::cout.put(v & 255);
		std::cout.put(v >> 8);
	}
}

void kick(Samples &s, float t)
{
	constexpr float decay = -36.0;
	constexpr float floor_freq = 55.0;
	constexpr float ceil_freq = 220.0;
	constexpr float boundary = (std::log(floor_freq) - std::log(ceil_freq)) / decay;
	constexpr float env_boundary = 0.5;
	constexpr float saturation = 0.2; // the lower the value, the more saturated. 0.5 is no saturation, 0.0 is max saturation without clipping
	const uint64_t st = t * sr;
	for (uint64_t i = 0; ; i++)
	{
		float t = float(i) / sr;
		// $$\text{ceil_freq, floor_freq, time}\ge0$$$$\text{decay}\lt0$$$$\text{phase}=\int_0^\text{time}max(\text{ceil_freq}\cdot e^{\text{decay}\cdot t},\text{floor_freq})dt$$$$=\begin{cases}\int_0^{time}\text{ceil_freq}\cdot e^{\text{decay}\cdot t}dt&\text{time}\lt\frac{\ln(\text{floor_freq})-\ln(\text{ceil_freq})}{\text{decay}}\\\int_0^{\frac{\ln(\text{floor_freq})-\ln(\text{ceil_freq})}{\text{decay}}}\text{ceil_freq}\cdot e^{\text{decay}\cdot t}dt+\int_{\frac{\ln(\text{floor_freq})-\ln(\text{ceil_freq})}{\text{decay}}}^{time}\text{floor_freq}dt&\text{time}\ge\frac{\ln(\text{floor_freq})-\ln(\text{ceil_freq})}{\text{decay}}\end{cases}$$$$=\begin{cases}\text{ceil_freq}\cdot\frac{e^{\text{decay}\cdot \text{time}}-1}{\text{decay}}&\text{time}\lt\frac{\ln(\text{floor_freq})-\ln(\text{ceil_freq})}{\text{decay}}\\\frac{\text{floor_freq}-\text{ceil_freq}}{\text{decay}}+\text{floor_freq}\cdot(\text{time}-\frac{\ln(\text{floor_freq})-\ln(\text{ceil_freq})}{\text{decay}})&\text{time}\ge\frac{\ln(\text{floor_freq})-\ln(\text{ceil_freq})}{\text{decay}}\end{cases}$$
		float phase1 = ceil_freq / decay * (exp(decay * t) - 1.0f);
		float phase2 = (floor_freq - ceil_freq) / decay + floor_freq * (t - boundary);
		float phase = t < boundary ? phase1 : phase2;
		float body = sin(6.2831853f * phase);
		// $$f(x)=c_1x+c_2x^3$$$$\text{find }c_1,c_2\text{ such that for a value }n\text{, these conditions are satisfied:}$$$$\begin{cases}f(1)=1\\f'(1)=n\end{cases}$$$$\iff\begin{cases}c_1+c_2=1\\c_1+3c_2=n\end{cases}$$$$\iff\begin{cases}c_1=\frac{3}{2}-\frac{n}{2}\\c_2=\frac{n}{2}-\frac{1}{2}\end{cases}$$
		// saturation = n/2
		body = (1.5f - saturation) * body + (saturation - 0.5f) * body*body*body;
		float e = 1.0f - exp(8.0f * (t - env_boundary));
		if (e <= 0) break;
		s[i+st] += e * body * 0.2f;
	}
}

struct BandNoise
{
	float s[8192];
	constexpr BandNoise(float band_low, float band_high, uint64_t state = 0x1234)
	{
		auto rnd = [&state]()
		{
			state ^= state >> 12;
			state ^= state << 25;
			state ^= state << 27;
			uint64_t x = state * 0x2545F4914F6CDD1DULL;
			uint32_t f = 0x3F800000 | (x & 0x007FFFF);
			return std::bit_cast<float>(f);
		};
		uint64_t band_low_idx = band_low * 8192 / sr;
		uint64_t band_high_idx = band_high * 8192 / sr;
		float band_scale = 4096.0f / (band_high_idx - band_low_idx);
		float ar[8192], ai[8192], br[8192], bi[8192];
		float ur[4096], ui[4096];
		for (uint64_t i = 0; i < 4096; i++)
			ur[i] = std::cos(std::numbers::pi * i / 4096.0),
			ui[i] = -std::sin(std::numbers::pi * i / 4096.0);
		for (uint64_t i = 0; i < 8192; i++)
			ar[i] = rnd() * 2.0f - 1.0f;
		for (uint64_t i = 0; i < 8192; i++)
			ai[i] = 0.0f;
		for (uint64_t i = 0; i < 8192; i++)
			br[i] = ar[i], bi[i] = ai[i];
		fft(ar, ai, br, bi, ur, ui, 1);
		for (uint64_t i = 0; i < 8192; i++)
		{
			uint64_t f = i;
			if (i > 4096) f = 8192 - i;
			float amp = 0.0f;
			if (f >= band_low_idx && f <= band_high_idx)
				amp = float(f - band_low_idx) / (band_high_idx - band_low_idx);
			ar[i] *= amp;
			ai[i] *= amp;
		}
		for (uint64_t i = 0; i < 8192; i++)
			br[i] = ar[i], bi[i] = ai[i];
		for (uint64_t i = 0; i < 4096; i++)
			ui[i] *= -1.0f;
		fft(ar, ai, br, bi, ur, ui, 1);
		for (uint64_t i = 0; i < 8192; i++)
			s[i] = ar[i] / 8192.0f * band_scale;
	}
	constexpr static void fft(float *ar, float *ai, float *br, float *bi, const float *ur, const float *ui, uint64_t step)
	{
		const uint64_t step2 = step << 1;
		if (step < 8192)
		{
			fft(br, bi, ar, ai, ur, ui, step2);
			fft(br + step, bi + step, ar + step, ai + step, ur, ui, step2);
			for (uint64_t i = 0; i < 8192; i += step2)
			{
				float tr = br[i + step] * ur[i >> 1] - bi[i + step] * ui[i >> 1];
				float ti = br[i + step] * ui[i >> 1] + bi[i + step] * ur[i >> 1];
				ar[i >> 1] = br[i] + tr;
				ai[i >> 1] = bi[i] + ti;
				ar[i + 8192 >> 1] = br[i] - tr;
				ai[i + 8192 >> 1] = bi[i] - ti;
			}
		}
	}
};

void hat(Samples &s, float t)
{
	constexpr BandNoise h(1000, 8000, 0x696969);
	const uint64_t st = t * sr;
	for (uint64_t i = 0; i < 8192; i++)
	{
		float t = float(i) / 8192.0f;
		float e = 1.0f - 2.0f * t + t*t;
		s[st + i] += h.s[i] * e;
	}
}

void tambourine(Samples &s, float t)
{
	constexpr BandNoise h(4000, 8000, 0x420420);
	const uint64_t st = t * sr;
	for (uint64_t i = 0; i < 8192; i++)
	{
		float t = float(i) / 8192.0f;
		float e = 1.0f - 2.0f * t + t*t;
		s[st + i] += h.s[i] * e;
	}
}

void music()
{
	Samples s;
	std::array<std::function<void(Samples &, float)>, 4> instr{ kick, hat, tambourine, hat };
	for (uint64_t i = 0; i < note_page_amount; i++)
		for (uint64_t j = 0; j < 32; j++)
			if (note_pages[i] >> 31-j & 1)
				instr[j&3](s, float(i) + float(j/4) / 8.0f);
	dump(s);
}

#include "bg.hh"

#include <fstream>
#include <array>
#include <cstdint>
#include <cmath>

static constexpr uint64_t W = 1280, H = 720;

float ss(float a, float b, float x)
{
	x = std::max(std::min((x - a) / (b - a), 1.0f), 0.0f);
	return x*x * std::fma(-2, x, 3);
}

void bg()
{
	std::array<uint8_t, W*H> img;
	std::array<float, 4> m{1.0f, 0.0f, 0.0f, 1.0f},
		mi{1.0f,0.0f,0.0f,1.0f};
	for (uint64_t i = 0; i < W*H; i++)
	{
		float ux = (float(i % W) * 2.0f - W) / H;
		float uy = (H - float(i / W) * 2.0f) / H;
		ux = ux * 10.0f;
		uy = uy * 10.0f;
		float tx = ux * mi[0] + uy * mi[1] - 1.0f;
		float ty = ux * mi[2] + uy * mi[3] - 1.0f;
		float cx = tx;
		float cy = ty;
		int ix = std::round(tx * 0.5f);
		int iy = std::round(ty * 0.5f);
		tx -= ix * 2.0f;
		ty -= iy * 2.0f;
		ux = tx * m[0] + ty * m[1];
		uy = tx * m[2] + ty * m[3];
		float d = std::hypot(ux, uy);
		float col = ss(0.9f, 0.8f, d);
		col *= ss(0.65f, 0.75f, d);
		col += ss(0.6f, 0.5f, d);
		col *= ss(-5.01f, -4.99f, cx);
		col *= ss(3.01f, 2.99f, cx);
		col *= ss(7.01f, 6.99f, cy);
		col *= ss(-9.01f, -8.99f, cy);
		ix = 1 - ix;
		iy += 12;
		col *= iy >> ix & 1;
		img[i] = col * 255.0f;
	}
	std::ofstream out("out.pbm", std::ios::binary);
	out << "P5\n" << W << ' ' << H << "\n255\n";
	out.write(reinterpret_cast<const char *>(img.data()), W*H);
	out.close();
}

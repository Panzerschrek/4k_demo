#pragma once
#include <cstdint>

// Linker requres this for any program with floating point calculations.
extern "C" int _fltused = 0;

namespace Math
{

constexpr float tau = 3.1415926535f * 2.0f;

inline float Cos(float x)
{
	if (x < 0.0f)
		x = -x;

	const float x_scaled = x / tau;
	const float z = tau * (float(x_scaled) - float(int(x_scaled)));

	const float minus_z2 = -z * z;
	float w = 1.0f;
	float res = 1.0f;

	for (int32_t i = 2; i <= 20; i += 2)
	{
		w *= minus_z2 / float(i * (i - 1));
		res += w;
	}

	return res;
}

inline float Sin(const float x)
{
	return Cos(x - tau / 4.0f);
}

inline float Log(const float x)
{
	// TODO - fix this. It's too inprecise for large numbers ( > 10 ).

	const float z = (x - 1.0f) / (x + 1.0f);
	const float z2 = z * z;
	float w = z;
	float res = 0.0f;

	for (int32_t i = 1; i <= 13; i += 2)
	{
		res += w / float(i);
		w *= z2;
	}

	return 2.0f * res;
}

} // namespace Math

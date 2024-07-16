#include <complex>
#include <emmintrin.h>
#include "window.hpp"

extern "C" int _fltused = 0;

float CosPositive(const float x)
{
	const float period = 3.1415926525f * 2.0f;

	const float x_scaled = x / period;
	const float z = period * (float(x_scaled) - float(int(x_scaled)));

	const float minus_z2 = -z * z;
	float w = 1.0f;
	float res = 1.0f;

	for (int32_t i = 2; i < 20; i+= 2)
	{
		w *= minus_z2 / float(i * (i - 1));
		res += w;
	}

	return res;
}

float Cos(const float x)
{
	if (x >= 0.0f)
		return CosPositive(x);
	else
		return CosPositive(-x);
}

float Log(const float x)
{
	const float z = (x - 1.0f) / (x + 1.0f);
	const float z2 = z * z;
	float w = z;
	float res = 0.0f;

	for (int32_t i = 1; i <= 13; i += 2)
	{
		res += w;
		w *= z2 / float(i);
	}

	return 2.0f * res;
}

int main()
{
	const uint32_t width = 512;
	const uint32_t height = 512;
	const uint32_t iterations = 96;

	DrawableWindow window("4k_mandelbrot", width, height);

	LARGE_INTEGER start_ticks;
	QueryPerformanceCounter(&start_ticks);

	LARGE_INTEGER ticks_per_second;
	QueryPerformanceFrequency(&ticks_per_second);

	while(true)
	{
		window.ProcessMessages();

		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		const float time = 0.25f * float(uint32_t(now.QuadPart - start_ticks.QuadPart)) / float(uint32_t(ticks_per_second.QuadPart));

		using Complex = std::complex<float>;

		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				DrawableWindow::PixelType& dst_pixel = window.GetPixels()[x + y * window.GetWidth()];

				Complex num(
					float(x) * (3.5f / float(width )) - 2.25f,
					float(y) * (3.5f / float(height)) - 1.75f);
		
				float c = 1.0f;
				if(std::norm(num) <= 4.0f)
				{
					Complex z(0.0f, 0.0f);
					for(uint32_t i = 0; i < iterations; ++i)
					{
						z = num + z * z;
						const float d = std::norm(z);
						if(d >= 4.0f)
							break;

						c += 4.0f - d;
					}
				}
	
				const float color_factor = Log(c) * 0.25f;

				const int32_t components[3]
				{
					int32_t((Cos(time * 1.0f + color_factor * 7.0f) * 0.5f + 0.5f) * 254.9f),
					int32_t((Cos(time * 1.5f + color_factor * 8.0f) * 0.5f + 0.5f) * 254.9f),
					int32_t((Cos(time * 2.0f + color_factor * 9.0f) * 0.5f + 0.5f) * 254.9f),
				};

				dst_pixel = DrawableWindow::PixelType(components[0] | (components[1] << 8) | (components[2] << 16));
			}
		}

		window.Blit();
	}
}

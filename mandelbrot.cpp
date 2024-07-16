#include <complex>
#include <emmintrin.h>
#include "window.hpp"

extern "C" int _fltused = 0;

float CosPositive(const float x)
{
	const float period = 3.1415926525f * 2.0f;

	const float x_scaled = x / period;
	const float z = period * (float(x_scaled) - float(int(x_scaled)));

	const float z2 = z * z;
	const float z4 = z2 * z2;
	const float z6 = z4 * z2;
	const float z8 = z4 * z4;
	const float z10 = z6 * z4;

	return 1.0f - z2 / 2.0f + z4 / 24.0f - z6 / 720.0f + z8 / 40320.0f - z10 / 3628800.0f;
}

float Cos(const float x)
{
	return CosPositive(std::abs(x));
}

float Log(const float x)
{
	const float z = (x - 1.0f) / (x + 1.0f);

	const float z2 = z * z;
	const float z3 = z * z2;
	const float z5 = z3 * z2;
	const float z7 = z5 * z2;
	const float z9 = z7 * z2;
	const float z11 = z9 * z2;
	const float z13 = z11 * z2;
	const float z15 = z13 * z2;

	return 2.0f * (z + z3 / 3.0f + z5 / 5.0f + z7 / 7.0f + z9 / 9.0f + z11 / 11.0f + z13 / 13.0f + z15 / 15.0f);
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
		const float time = float(uint32_t(now.QuadPart - start_ticks.QuadPart)) / float(uint32_t(ticks_per_second.QuadPart));

		using Complex = std::complex<float>;

		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				DrawableWindow::PixelType& dst_pixel = window.GetPixels()[x + y * window.GetWidth()];

				Complex num(
					1.0f - float(x) * (3.5f / float(width )),
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

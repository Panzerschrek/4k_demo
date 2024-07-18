#include <complex>
#include <utility>
#include "math.hpp"
#include "window.hpp"

#ifdef __MINGW32__
extern "C" int __main()
#else
int main()
#endif
{
	const uint32_t width = 800;
	const uint32_t height = 600;
	const uint32_t iterations = 128;
	const uint32_t scale = std::min(width, height);

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
					float(x) * (2.5f / float(scale)) - 2.1f,
					float(y) * (2.5f / float(scale)) - 1.25f);
		
				float c = 1.0f;
				Complex z(0.0f, 0.0f);
				for(uint32_t i = 0; i < iterations; ++i)
				{
					z = num + z * z;
					const float d = std::norm(z);
					if(d >= 4.0f)
						break;

					c += 4.0f - d;
				}
	
				const float color_factor = Math::Log(c);

				constexpr float half_range = (255.0f - 0.1f) / 2.0f;
				const int32_t components[3]
				{
					int32_t(Math::Cos(time * 1.0f + color_factor * 1.75f) * half_range + half_range),
					int32_t(Math::Cos(time * 1.5f + color_factor * 2.0f) * half_range + half_range),
					int32_t(Math::Cos(time * 2.0f + color_factor * 2.25f) * half_range + half_range),
				};

				dst_pixel = DrawableWindow::PixelType(components[0] | (components[1] << 8) | (components[2] << 16));
			}
		}

		window.Blit();
	}
}

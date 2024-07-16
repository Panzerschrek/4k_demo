#include <complex>
#include "window.hpp"

extern "C" int _fltused = 0;

int main()
{
	const uint32_t width = 512;
	const uint32_t height = 512;

	DrawableWindow window("4k_mandelbrot", width, height);

	uint32_t iteration = 0;

	while(true)
	{
		window.ProcessMessages();

		using Complex = std::complex<float>;

		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				DrawableWindow::PixelType& dst_pixel = window.GetPixels()[x + y * window.GetWidth()];

				Complex num(
					float(y) * (2.0f / float(width )) - 1.0f,
					float(x) * (2.0f / float(height)) - 1.0f);
		
				float c = 1.0f;
				if(std::norm(num) <= 4.0f)
				{
					Complex z(0.0f, 0.0f);
					for(uint32_t i = 0; i < 32u; ++i)
					{
						z = num + z * z;
						const float d = std::norm(z);
						if (d >= 4.0f)
							break;

						c += 4.0f - d;
					}

					dst_pixel = DrawableWindow::PixelType(c);
				}
				else
				{
					dst_pixel = 0;
				}
			}
		}

		window.Blit();

		Sleep(16);

		++iteration;
	}
}

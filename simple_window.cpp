#include "window.hpp"

#ifdef __MINGW32__
extern "C" int __main()
#else
int main()
#endif
{
	DrawableWindow window("4k_demo", 640, 480);

	uint32_t iteration = 0;

	while(true)
	{
		window.ProcessMessages();

		for(uint32_t y= 0; y < window.GetHeight(); ++y)
			for(uint32_t x = 0; x < window.GetWidth(); ++x)
				window.GetPixels()[x + y * window.GetWidth()] =
					(x * 255u / window.GetWidth()) |
					((y * 255u / window.GetHeight()) << 8) |
					(((iteration << 1) & 255) << 16);

		window.Blit();

		Sleep(16);

		++iteration;
	}
}

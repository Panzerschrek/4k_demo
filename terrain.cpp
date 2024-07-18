#include "window.hpp"

static int32_t Noise2(int32_t x, int32_t y, int32_t seed)
{
	const int X_NOISE_GEN = 1619;
	const int Y_NOISE_GEN = 31337;
	const int Z_NOISE_GEN = 6971;
	const int SEED_NOISE_GEN = 1013;

	int n = (
		X_NOISE_GEN * x +
		Y_NOISE_GEN * y +
		Z_NOISE_GEN * 0 +
		SEED_NOISE_GEN * seed)
		& 0x7fffffff;

	n = (n >> 13) ^ n;
	return ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) >> 15;
}

#pragma 
#ifdef DEBUG
int WINAPI WinMain(HINSTANCE , HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	constexpr uint32_t hightmap_size_log2 = 9;
	constexpr uint32_t hightmap_size = 1 << hightmap_size_log2;
	// For now allocate memory from heap.
	// Using static array increases executable size.
	// Using stack array requires stack checking function.
	uint8_t* hightmap = reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr, hightmap_size * hightmap_size, MEM_COMMIT, PAGE_READWRITE));

	for(uint32_t y = 0; y < hightmap_size; ++y)
	for(uint32_t x = 0; x < hightmap_size; ++x)
		hightmap[ x + (y << hightmap_size_log2)] = Noise2(x, y, 0);
	
	DrawableWindow window("4k_terrain", hightmap_size, hightmap_size);

	uint32_t iteration = 0;

	while(true)
	{
		window.ProcessMessages();

		for(uint32_t y= 0; y < window.GetHeight(); ++y)
			for(uint32_t x = 0; x < window.GetWidth(); ++x)
			{
				const uint8_t h = hightmap[x + (y << hightmap_size_log2)];

				window.GetPixels()[x + y * window.GetWidth()] = h | (h << 8) | (h << 16);
			}

		window.Blit();

		Sleep(16);

		++iteration;
	}
}

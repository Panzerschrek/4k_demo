#include <utility>
#include "math.hpp"
#include "window.hpp"

static int32_t Noise2(const int32_t x, const int32_t y, const int32_t seed)
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

static uint32_t InterpolatedNoise(const uint32_t x, const uint32_t y, const uint32_t size_log2, const uint32_t k)
{
	const uint32_t step = 1 << k;
	const uint32_t mask = ((1 << size_log2) >> k) - 1; // This makes noise tileable.

	const uint32_t X = x >> k;
	const uint32_t Y = y >> k;

	const uint32_t noise[4] =
	{
		uint32_t(Noise2((X    ) & mask, (Y    ) & mask, 0)),
		uint32_t(Noise2((X + 1) & mask, (Y    ) & mask, 0)),
		uint32_t(Noise2((X + 1) & mask, (Y + 1) & mask, 0)),
		uint32_t(Noise2((X    ) & mask, (Y + 1) & mask, 0))
	};

	const uint32_t dx = x - (X << k);
	const uint32_t dy = y - (Y << k);

	const uint32_t interp_x[2] =
	{
		dy * noise[3] + (step - dy) * noise[0],
		dy * noise[2] + (step - dy) * noise[1],
	};
	return uint32_t((interp_x[1] * dx + interp_x[0] * (step - dx)) >> (k + k));
}

#pragma 
#ifdef DEBUG
int WINAPI WinMain(HINSTANCE , HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	constexpr uint32_t hightmap_size_log2 = 10;
	constexpr uint32_t hightmap_size = 1 << hightmap_size_log2;
	constexpr uint32_t hightmap_size_mask = hightmap_size - 1;

	// For now allocate memory from heap.
	// Using static array increases executable size.
	// Using stack array requires stack checking function.
	uint8_t* hightmap = reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr, hightmap_size * hightmap_size, MEM_COMMIT, PAGE_READWRITE));

	for(uint32_t y = 0; y < hightmap_size; ++y)
	for(uint32_t x = 0; x < hightmap_size; ++x)
	{
		constexpr uint32_t min_octave = 1;
		constexpr uint32_t max_octave = 6;

		uint32_t r = 0;
		for(uint32_t i = min_octave; i <= max_octave; ++i)
			r += InterpolatedNoise(x, y, hightmap_size_log2, i) >> (max_octave  - i);

		hightmap[ x + (y << hightmap_size_log2)] = r >> 9;
	}
	
	DrawableWindow window("4k_terrain", 640, 480);

	uint32_t shift = 0;

	while(true)
	{
		window.ProcessMessages();

		/*
		for(uint32_t y= 0; y < window.GetHeight(); ++y)
			for(uint32_t x = 0; x < window.GetWidth(); ++x)
			{
				const uint8_t h = hightmap[((x + shift * 2u) & hightmap_size_mask) + (((y + shift) & hightmap_size_mask) << hightmap_size_log2)];

				window.GetPixels()[x + y * window.GetWidth()] = h | (h << 8) | (h << 16);
			}*/

		ZeroMemoryInline(window.GetPixels(), window.GetWidth() * window.GetHeight() * sizeof(DrawableWindow::PixelType));

		const float cam_position[3]{float(hightmap_size) / 2.0f, 0.0f, 120.0f};
		const float additional_y_shift = 0.5f;

		// Process columns.
		for(uint32_t x = 0; x < window.GetWidth(); ++x)
		{
			const float ray_x = float(x) * (2.0f / float(window.GetWidth())) - 1.0f;

			const int32_t max_y = int32_t(window.GetHeight()) - 1;
			int32_t prev_y = window.GetHeight();
			for(float depth = 1.0f; depth < float(hightmap_size * 3 / 2); depth *= 1.008f)
			{
				const float terrain_pos[2]{ ray_x * depth, depth };

				const uint8_t h =
					hightmap[
						(int32_t(terrain_pos[0]) & hightmap_size_mask) +
						((int32_t(terrain_pos[1]) & hightmap_size_mask) << hightmap_size_log2)];

				const float h_scaled = float(h) / 2.0f;

				const float h_relative_to_camera = h_scaled - cam_position[2];

				const float screen_y = h_relative_to_camera / depth;
				const int32_t y = int32_t((1.0f - screen_y - additional_y_shift) * 0.5f * float(window.GetHeight()));

				const int32_t min_y= std::max(y, 0);
				for(int32_t dst_y = std::min(prev_y - 1, max_y); dst_y >= min_y; --dst_y)
					window.GetPixels()[x + uint32_t(dst_y) * window.GetWidth()] = h | (h << 8) | (h << 16);

				prev_y = min_y;
			}

			for(int32_t dst_y = std::min(prev_y - 1, max_y); dst_y >= 0; --dst_y)
				window.GetPixels()[x + uint32_t(dst_y) * window.GetWidth()] = 0x00000000;
		}

		window.Blit();

		Sleep(16);

		++shift;
	}
}

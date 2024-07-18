#include <array>
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

inline std::array<float, 3> GetTerrainColor(float h)
{
	constexpr uint32_t num_colors = 6;
	static constexpr float borders[num_colors - 1]= { 80.0f, 90.0f, 110.0f, 145.0f, 160.0f };
	static constexpr float colors[num_colors][3]
	{
		{ 170.0f, 110.0f, 110.0f },
		{ 60.0f, 170.0f, 170.0f },
		{ 70.0f, 190.0f, 70.0f },
		{ 80.0f, 128.0f, 80.0f },
		{ 110.0f, 110.0f, 110.0f },
		{ 255.0f, 255.0f, 255.0f },
	};

	const float half_border = 7.0f;
	
	for(uint32_t i = 0; i < num_colors - 1; ++i)
	{
		if(h <= borders[i])
		{
			const float up_dist = borders[i] - h;
			if(up_dist < half_border)
			{
				const float k = up_dist / half_border;
				const float one_minus_k = 1.0f - k;
				return { colors[i][0] * k + colors[i+1][0] * one_minus_k, colors[i][1] * k + colors[i + 1][1] * one_minus_k, colors[i][2] * k + colors[i + 1][2] * one_minus_k };
			}
			else
				return {colors[i][0], colors[i][1], colors[i][2]};
		}
	}

	const auto& last_color = colors[num_colors - 1];
	return { last_color[0], last_color[1], last_color[2] };
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
	uint16_t* hightmap = reinterpret_cast<uint16_t*>(VirtualAlloc(nullptr, hightmap_size * hightmap_size * sizeof(uint16_t), MEM_COMMIT, PAGE_READWRITE));

	for(uint32_t y = 0; y < hightmap_size; ++y)
	for(uint32_t x = 0; x < hightmap_size; ++x)
	{
		constexpr uint32_t min_octave = 1;
		constexpr uint32_t max_octave = 7;

		uint32_t r = 0;
		for(uint32_t i = min_octave; i <= max_octave; ++i)
			r += InterpolatedNoise(x, y, hightmap_size_log2, i) >> (max_octave  - i);

		hightmap[ x + (y << hightmap_size_log2)] = r >> 1;
		// TODO - clamp to water level?
	}
	
	DrawableWindow window("4k_terrain", 1024, 768);

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

		ZeroMemoryInline(window.GetPixels(), window.GetWidth() * window.GetHeight() * sizeof(DrawableWindow::PixelType));

		const float move_speed = 15.0f;
		const float cam_position[3]{0.0f, time * move_speed, 210.0f};
		const float additional_y_shift = 0.5f;

		const float screen_scale = 0.5f * float(std::max(window.GetWidth(), window.GetHeight()));

		static constexpr float color_fog[3]{ 240.0f, 200.0f, 200.0f };

		const float max_depth = float(hightmap_size * 2);

		const float cam_angle = time * (-0.06f);
		const float cam_angle_cos = Math::Cos(cam_angle);
		const float cam_angle_sin = Math::Sin(cam_angle);

		// Process columns.
		for(uint32_t x = 0; x < window.GetWidth(); ++x)
		{
			const float ray_x = (float(x) - float(window.GetWidth()) * 0.5f) / screen_scale;

			const int32_t max_y = int32_t(window.GetHeight()) - 1;
			int32_t prev_y = window.GetHeight();
			// Increase depth exponentialy.
			for(float depth = 24.0f; depth < max_depth; depth *= 1.004f)
			{
				const float ray_vec[2]{ ray_x * depth, depth };
				const float ray_vec_rotated[2]{ ray_vec[0] * cam_angle_cos - ray_vec[1] * cam_angle_sin, ray_vec[0] * cam_angle_sin + ray_vec[1] * cam_angle_cos };
				const float terrain_pos[2]{ ray_vec_rotated[0] + cam_position[0], ray_vec_rotated[1] + cam_position[1] };

				const uint16_t h =
					hightmap[
						(int32_t(terrain_pos[0]) & hightmap_size_mask) +
						((int32_t(terrain_pos[1]) & hightmap_size_mask) << hightmap_size_log2)];

				// TODO - perform interpolation.
				const float h_scaled = float(h) * (0.75f / 256.0f);

				const float h_relative_to_camera = h_scaled - cam_position[2];

				const float screen_y = h_relative_to_camera / depth;
				const int32_t y = int32_t((1.0f - screen_y - additional_y_shift) * screen_scale);

				const auto own_color = GetTerrainColor(float(h) / 256.0f);

				// TODO - add simple sun lighting.
				
				const float fog_factor = depth / max_depth;
				const float one_minus_fog_factor = 1.0f - fog_factor;

				DrawableWindow::PixelType color = 0x000000;
				for(uint32_t j = 0; j < 3; ++j)
				{
					float color_mixed = own_color[j] * one_minus_fog_factor + color_fog[j] * fog_factor;
					color |= uint32_t(int32_t(color_mixed) << (j << 3));
				}

				const int32_t min_y= std::max(y, 0);
				for(int32_t dst_y = std::min(prev_y - 1, max_y); dst_y >= min_y; --dst_y)
					window.GetPixels()[x + uint32_t(dst_y) * window.GetWidth()] = color;

				prev_y = min_y;
			}

			// Fill remaining sky with fog.
			{
				constexpr DrawableWindow::PixelType fog_color_int = uint32_t(color_fog[0]) | (uint32_t(color_fog[1]) << 8) | (uint32_t(color_fog[2]) << 16);
				for(int32_t dst_y = std::min(prev_y - 1, max_y); dst_y >= 0; --dst_y)
					window.GetPixels()[x + uint32_t(dst_y) * window.GetWidth()] = fog_color_int;
			}
		}

		window.Blit();

		Sleep(5);
	}
}

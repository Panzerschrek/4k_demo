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

static inline uint32_t InterpolatedNoise(const uint32_t x, const uint32_t y, const uint32_t size_log2, const uint32_t k)
{
	const uint32_t step = 1 << k;
	const uint32_t mask = ((1 << size_log2) >> k) - 1; // This makes noise tileable.

	const uint32_t X = x >> k;
	const uint32_t Y = y >> k;

	const uint32_t noise[4] =
	{
		uint32_t(Noise2((X)&mask, (Y)&mask, 0)),
		uint32_t(Noise2((X + 1) & mask, (Y)&mask, 0)),
		uint32_t(Noise2((X + 1) & mask, (Y + 1) & mask, 0)),
		uint32_t(Noise2((X)&mask, (Y + 1) & mask, 0))
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

constexpr uint32_t cloud_texture_size_log2 = 10;
constexpr uint32_t cloud_texture_size = 1 << cloud_texture_size_log2;
constexpr uint32_t cloud_texture_size_mask = cloud_texture_size - 1;

constexpr uint32_t window_width= 800;
constexpr uint32_t window_height= 600;

using ColorHDR= std::array<float, 3>;

struct DemoData
{
	float clouds_texture[cloud_texture_size * cloud_texture_size];
	ColorHDR colors_temp_buffers[2][window_width * window_height];
};

#ifdef DEBUG
int WINAPI WinMain(HINSTANCE , HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	// For now allocate memory from heap.
	// Using static array increases executable size.
	// Using stack array requires stack checking function.
	const auto demo_data = reinterpret_cast<DemoData*>(VirtualAlloc(nullptr, sizeof(DemoData), MEM_COMMIT, PAGE_READWRITE));

	for(uint32_t y = 0; y < cloud_texture_size; ++y)
	for(uint32_t x = 0; x < cloud_texture_size; ++x)
	{
		constexpr uint32_t min_octave = 2;
		constexpr uint32_t max_octave = 6;

		uint32_t r = 0;
		for(uint32_t i = min_octave; i <= max_octave; ++i)
			r += InterpolatedNoise(x, y, cloud_texture_size_log2, i) >> (max_octave - i);

		const uint32_t address = x + (y << cloud_texture_size_log2);

		const float alpha= float(r) / (512u * 256u);

		const float border_low= 0.55f;
		const float border_height= 0.7f;

		float alpha_corrected;
		if( alpha <= border_low)
			alpha_corrected = 0.0f;
		else if( alpha >= border_height)
			alpha_corrected= 1.0f;
		else
			alpha_corrected= (alpha - border_low) / (border_height - border_low);
		
		demo_data->clouds_texture[address]= alpha_corrected;
	}

	DrawableWindow window("4k_ocean", window_width, window_height);

	static constexpr ColorHDR sky_color{ 8.4f, 2.5f, 2.5f };
	static constexpr ColorHDR clouds_color{ 8.4f, 8.4f, 8.4f };
	static constexpr ColorHDR sun_color{ 10.0f, 20.0f, 90.0f };
	static constexpr ColorHDR horizon_color{ 1.0f, 4.0f, 20.0f };

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

		const float move_speed = 40.0f;

		constexpr float camera_height= 256.0f;
		const float camera_distance= time * move_speed;

		constexpr uint32_t sun_radius= 24;
		constexpr uint32_t sun_center[2]{ window_width / 2u, window_height / 2u - sun_radius * 3u / 2u };

		// Draw sky.
		constexpr uint32_t horizon_offset= 10u;
		constexpr auto clouds_end_y= window_height / 2u - horizon_offset;
		for(uint32_t y= 0; y < clouds_end_y; ++y)
		{
			const float y_f= float(int32_t(y));

			const float line_distance= camera_height * float(window_height / 2.0f) / (float(window_height / 2u) - y_f);
			const float line_scale= line_distance / float(window_width / 2u);

			float horizon_factor= 1.0f - (float(window_height / 2u - horizon_offset) - y_f) / float(window_height / 2u - horizon_offset);
			horizon_factor= horizon_factor * horizon_factor;

			const float one_minus_horizon_factor= 1.0f - horizon_factor;

			const uint32_t tex_v= int32_t(line_distance + camera_distance);

			const auto src_line_texels= demo_data->clouds_texture + (tex_v & uint32_t(cloud_texture_size_mask)) * cloud_texture_size;

			const auto dst_line = demo_data->colors_temp_buffers[0] + y * window_width;

			for(uint32_t x = 0; x < window_width; ++x)
			{
				const int32_t tex_v= int32_t(line_scale * (float(x) - float(window_width / 2u))) & cloud_texture_size_mask;

				const int32_t sun_delta[2]{ int32_t(x) - int32_t(sun_center[0]), int32_t(y) - int32_t(sun_center[1]) };
				
				const ColorHDR* own_color;
				if(sun_delta[0] * sun_delta[0] + sun_delta[1] * sun_delta[1] <= sun_radius * sun_radius)
					own_color= &sun_color;
				else
					own_color= &sky_color;

				const float alpha= src_line_texels[tex_v];
				const float one_minus_alpha = 1.0f - alpha;

				for (uint32_t j = 0; j < 3; ++j)
				{
					const float color_mixed = (*own_color)[j] * one_minus_alpha + clouds_color[j] * alpha;
					const float color_horizon_mixed= one_minus_horizon_factor * color_mixed + horizon_color[j] * horizon_factor;
					dst_line[x][j]= color_horizon_mixed;
				}
			}
		}

		constexpr uint32_t water_y_start = window_height / 2u + horizon_offset;

		// Copy sky to water with horizontal blur.
		for(uint32_t y = water_y_start; y < window_height; ++y)
		{
			// Make reflection at the screen bottom a little bit darker.
			const float horizon_factor = float(int32_t(y)) * ( -1.0f / float(window_height) ) + 1.5f;
			const float half_horizon_factor= 0.5f * horizon_factor;

			const auto src_line = demo_data->colors_temp_buffers[0] + (window_height - 1u - y) * window_width;
			const auto dst_line = demo_data->colors_temp_buffers[1] + y * window_width;

			constexpr float blur_factor= 0.9f;
			constexpr float blur_facrtor_minus_one= 1.0f - blur_factor;

			// Blur left to right.
			ColorHDR blurreed_color= src_line[0];
			for(uint32_t x = 0; x < window_width; ++x)
			{
				for(uint32_t j= 0; j < 3; ++j)
				{
					blurreed_color[j]= blurreed_color[j] * blur_factor + src_line[x][j] * blur_facrtor_minus_one;
					dst_line[x][j] = blurreed_color[j] * half_horizon_factor;
				}
			}

			// Blur right to left and add to previous blur.
			blurreed_color= src_line[window_width - 1];
			for(uint32_t x = 0; x < window_width; ++x)
			{
				const uint32_t x1= window_width - 1u - x;
				for (uint32_t j = 0; j < 3; ++j)
				{
					blurreed_color[j] = blurreed_color[j] * blur_factor + src_line[x1][j] * blur_facrtor_minus_one;
					dst_line[x1][j] += blurreed_color[j] * half_horizon_factor;
				}
			}
		}

		// Perform vertical blur for water.
		for(uint32_t x = 0; x < window_width; ++x)
		{
			const auto src_column = demo_data->colors_temp_buffers[1] + x;
			const auto dst_column = demo_data->colors_temp_buffers[0] + x;

			constexpr float blur_factor = 0.965f;
			constexpr float blur_facrtor_minus_one = 1.0f - blur_factor;
			constexpr float top_to_bottom_weight= 0.75f;
			constexpr float bottom_to_top_weight= 1.0f - top_to_bottom_weight;

			// Blur top to bottom.
			ColorHDR blurreed_color = src_column[water_y_start * window_width];
			for(uint32_t y = water_y_start; y < window_height; ++y)
			{
				const uint32_t y_offset= y * window_width;
				for (uint32_t j = 0; j < 3; ++j)
				{
					blurreed_color[j] = blurreed_color[j] * blur_factor + src_column[y_offset][j] * blur_facrtor_minus_one;
					dst_column[y_offset][j] = blurreed_color[j] * top_to_bottom_weight;
				}
			}
			
			// Blur bottom to top and add to previous blur.
			blurreed_color = src_column[(window_height - 1) * window_width];
			for(uint32_t y = window_height - 1; y >= water_y_start; --y)
			{
				const uint32_t y_offset = y * window_width;
				for (uint32_t j = 0; j < 3; ++j)
				{
					blurreed_color[j] = blurreed_color[j] * blur_factor + src_column[y_offset][j] * blur_facrtor_minus_one;
					dst_column[y_offset][j] += blurreed_color[j] * bottom_to_top_weight;
				}
			}
		}

		// Fill horizon span.
		for(uint32_t y= clouds_end_y; y < water_y_start; ++y)
		{
			const auto dst_line= demo_data->colors_temp_buffers[0] + y * window_width;
			for(uint32_t x = 0; x < window_width; ++x)
				dst_line[x]= horizon_color;
		}

		for(uint32_t y = 0; y < window_height; ++y)
		{
			const auto src_line= demo_data->colors_temp_buffers[0] + y * window_width;
			const auto dst_line= window.GetPixels() + y * window_width;
			for(uint32_t x = 0; x < window_width; ++x)
			{
				DrawableWindow::PixelType color = 0x000000;
				for (uint32_t j = 0; j < 3; ++j)
				{
					const float c= src_line[x][j];
					const float color_tonemapped= 255.0f * c / (c + 1.0f);
					color |= uint32_t(int32_t(color_tonemapped) << (j << 3));
				}

				dst_line[x]= color;
			}
		}

		window.Blit();
	}
}

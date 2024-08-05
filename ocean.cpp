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

constexpr uint32_t window_width= 768;
constexpr uint32_t window_height= 768;

struct DemoData
{
	float clouds_texture[cloud_texture_size * cloud_texture_size];
	std::array<float, 3> colors_temp_buffer[window_width * window_height];
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

	static constexpr float sky_color[3]{ 240.0f, 200.0f, 200.0f };
	static constexpr float clouds_color[3]{ 240.0f, 240.0f, 240.0f };
	static constexpr float sun_color[3]{ 200.0f, 240.0f, 255.0f };

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

		const float height= 256.0f;
		const float distance= time * move_speed;

		const uint32_t sun_radius= 32;
		const uint32_t sun_center[2]{ window.GetWidth() / 2u, window.GetHeight() / 2u - 2 * sun_radius };

		const auto clouds_end_y= window.GetHeight() / 2u - 10;
		for(uint32_t y= 0; y < clouds_end_y; ++y)
		{
			const float line_distance= float(height) * float(window.GetHeight() / 2.0f) / (float(window.GetHeight() / 2u) - float(y));
			const float line_scale= line_distance / float(window.GetWidth() / 2u);

			const uint32_t tex_v= uint32_t(line_distance + distance);

			const auto src_line_texels= demo_data->clouds_texture + (tex_v & uint32_t(cloud_texture_size_mask)) * cloud_texture_size;

			const auto dst_line = demo_data->colors_temp_buffer + y * window_width;

			for(uint32_t x = 0; x < window.GetWidth(); ++x)
			{
				const int32_t tex_v= int32_t(line_scale * (float(x) - float(window.GetHeight() / 2u))) & cloud_texture_size_mask;

				const int32_t sun_delta[2]{ int32_t(x) - int32_t(sun_center[0]), int32_t(y) - int32_t(sun_center[1]) };
				
				const float* own_color;
				if(sun_delta[0] * sun_delta[0] + sun_delta[1] * sun_delta[1] <= sun_radius * sun_radius)
					own_color= sun_color;
				else
					own_color= sky_color;

				const float alpha= src_line_texels[tex_v];
				const float one_minus_alpha = 1.0f - alpha;

				DrawableWindow::PixelType color = 0x000000;
				for (uint32_t j = 0; j < 3; ++j)
				{
					const float color_mixed = own_color[j] * one_minus_alpha + clouds_color[j] * alpha;
					dst_line[x][j]= color_mixed;
					color |= uint32_t(int32_t(color_mixed) << (j << 3));
				}
			}
		}

		for(uint32_t y = window.GetHeight() / 2u + 10; y < window.GetHeight(); ++y)
		{
			const auto src_y = window_height - 1 - y;
			const auto dst_line = demo_data->colors_temp_buffer + y * window_width;
			for(uint32_t x = 0; x < window.GetWidth(); ++x)
			{
				std::array<float, 3> avg_color{0.0f, 0.0f, 0.0f};
				for(int32_t dy= -10; dy <= 10; ++dy)
				{
					const int32_t src_blur_y= std::max(0, std::min(int32_t(src_y) + dy, int32_t(clouds_end_y) - 1));
					const auto& src_color= demo_data->colors_temp_buffer[x + src_blur_y * int32_t(window_width)];

					for(uint32_t j= 0; j < 3; ++j)
						avg_color[j]+= src_color[j];
				}
				for(uint32_t j = 0; j < 3; ++j)
					dst_line[x][j]= avg_color[j] / 21.0f;
			}
		}

		for(uint32_t y = 0; y < window.GetHeight(); ++y)
		{
			const auto src_line= demo_data->colors_temp_buffer + y * window_width;
			const auto dst_line= window.GetPixels() + y * window.GetWidth();
			for(uint32_t x = 0; x < window.GetWidth(); ++x)
			{
				DrawableWindow::PixelType color = 0x000000;
				for (uint32_t j = 0; j < 3; ++j)
				{
					color |= uint32_t(int32_t(src_line[x][j]) << (j << 3));
				}

				dst_line[x]= color;
			}
		}

		window.Blit();

		Sleep(16);

	}
}

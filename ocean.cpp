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

struct CloudsData
{
	float alpha[cloud_texture_size * cloud_texture_size];
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
	const auto clouds_data = reinterpret_cast<CloudsData*>(VirtualAlloc(nullptr, sizeof(CloudsData), MEM_COMMIT, PAGE_READWRITE));

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
		
		clouds_data->alpha[address]= alpha_corrected;
	}

	DrawableWindow window("4k_ocean", 768, 768);

	static constexpr float sky_fog[3]{ 240.0f, 200.0f, 200.0f };
	static constexpr float clouds_color[3]{ 240.0f, 240.0f, 240.0f };

	while(true)
	{
		window.ProcessMessages();

		const float height= 256.0f;

		for(uint32_t y= 0; y < window.GetHeight() / 2u - 20; ++y)
		{
			const float tex_y= float(height) * float(window.GetHeight() / 2.0f) / (float(window.GetHeight() / 2u) - float(y));
			const float line_scale= tex_y / float(window.GetWidth() / 2u);

			const uint32_t tex_v= uint32_t(tex_y);

			const auto src_line_texels= clouds_data->alpha + (tex_v & uint32_t(cloud_texture_size_mask)) * cloud_texture_size;

			for(uint32_t x = 0; x < window.GetWidth(); ++x)
			{
				const int32_t tex_v= int32_t(line_scale * (float(x) - float(window.GetHeight() / 2u))) & cloud_texture_size_mask;

				const float alpha= src_line_texels[tex_v];
				const float one_minus_alpha = 1.0f - alpha;

				DrawableWindow::PixelType color = 0x000000;
				for (uint32_t j = 0; j < 3; ++j)
				{
					float color_mixed = sky_fog[j] * one_minus_alpha + clouds_color[j] * alpha;
					color |= uint32_t(int32_t(color_mixed) << (j << 3));
				}

				window.GetPixels()[x + y * window.GetWidth()] = color;
			}
		}

		window.Blit();

		Sleep(16);

	}
}

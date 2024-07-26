#include <limits>
#include "math.hpp"
#include "window.hpp"

constexpr uint32_t sampling_frequency = 22050;
using SampleType = int16_t;

// Number of buffers and chunk size are tuned for minimal latency without sound cracks.
constexpr uint32_t chunk_size = 512;
constexpr uint32_t num_headers = 4;

using BufferData = SampleType[num_headers][chunk_size];

static constexpr const WAVEFORMATEX wfx
{
	WAVE_FORMAT_PCM, // Format tag.
	1, // Number of channels.
	sampling_frequency, // Sample rate
	sampling_frequency * sizeof(SampleType), // Bytes per second.
	alignof(SampleType), // Alignment of block.
	8 * sizeof(SampleType), // Bits per sample.
	0 // Extra size.
};

// Assuming no circle is outside window borders.
static void DrawCircle(
	DrawableWindow& window,
	const int32_t center_x, const int32_t center_y,
	const int32_t radius,
	const DrawableWindow::PixelType color)
{
	const uint32_t w = window.GetWidth();
	const auto pixels = window.GetPixels();
	for(int32_t dy = -radius; dy <= radius; ++dy)
	for(int32_t dx = -radius; dx <= radius; ++dx)
	{
		if(dx * dx + dy * dy > radius * radius)
			continue;

		pixels[uint32_t(center_x + dx) + uint32_t(center_y + dy) * w] = color;
	}
}

constexpr float c_beat_period = 64.0f; // TODO - make it bigger.
constexpr float c_beat_fraction = 1.0f / 8.0f;
constexpr int32_t c_start_beat = 2;
constexpr int32_t c_end_beat = 9;

constexpr float c_base_freq = 220.0f;
constexpr float c_sample_scale = 0.333f / float(1 + c_end_beat - c_start_beat);

#ifdef DEBUG
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	DrawableWindow window("4k_sound", 800, 600);

	// For now allocate memory from heap.
	// Using static array increases executable size.
	// Using stack array requires stack checking function.
	auto& buffer_data = *reinterpret_cast<BufferData*>(VirtualAlloc(nullptr, sizeof(BufferData), MEM_COMMIT, PAGE_READWRITE));

	HWAVEOUT wave_out = 0;

	waveOutOpen(&wave_out, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

	WAVEHDR headers[num_headers];
	for(uint32_t i = 0; i < num_headers; ++i)
	{
		headers[i].dwFlags = WHDR_DONE;
		headers[i].dwBufferLength = chunk_size * sizeof(SampleType);
	}

	const uint32_t screen_area = window.GetWidth() * window.GetHeight();

	uint32_t block_index_linear = 0;
	uint32_t t = 0;
	while(true)
	{
		const uint32_t block_index = block_index_linear % num_headers;
		WAVEHDR& header = headers[block_index];
		// Fill any buffers that are "done"
		if((header.dwFlags & WHDR_DONE) != 0)
		{
			// Clear previous usage.
			waveOutUnprepareHeader(wave_out, &header, sizeof(WAVEHDR));

			header.dwFlags = 0;
			header.lpData = reinterpret_cast<LPSTR>(buffer_data[block_index]);

			// Calculate a new chunk of data.
			for(uint32_t i = 0; i < chunk_size; ++i)
			{
				const float w = float(t) * (c_base_freq / float(sampling_frequency));
				const float p = w * Math::tau;

				float result = 0.0f;

				// See https://www.youtube.com/watch?v=Qyn64b4LNJ0.
				for (int32_t beat_n = c_start_beat; beat_n <= c_end_beat; ++beat_n)
				{
					const float current_period = c_beat_period * float(beat_n);
					const float current_beat_w = w / current_period;
					const float beat_factor = current_beat_w - float(int32_t(current_beat_w));
					if (beat_factor < c_beat_fraction)
					{
						float beat_p = p * float(beat_n);
						result += Math::Sin(beat_p) + Math::Sin(beat_p * 2.0f) * 0.5f + Math::Sin(beat_p * 3.0f) * 0.3333f;
					}
				}

				constexpr float amplitude = float(std::numeric_limits<SampleType>::max()) / 16.0f; // Not too loud.
				buffer_data[block_index][i] = SampleType(int32_t(result * amplitude));

				++t;
			}
			
			waveOutPrepareHeader(wave_out, &header, sizeof(WAVEHDR));
			waveOutWrite(wave_out, &header, sizeof(WAVEHDR));

			++block_index_linear;

			// Process messages and update window only after successfull sound output.
			if(block_index_linear % 2 == 0)
			{
				window.ProcessMessages();

				// Draw.
				ZeroMemoryInline(window.GetPixels(), window.GetWidth() * window.GetHeight() * sizeof(DrawableWindow::PixelType));

				const uint32_t center_x = window.GetWidth() / 2u;
				const uint32_t center_y = window.GetHeight() / 2u;
				// Sun.
				DrawCircle(window, center_x, center_y, 16, 0x00FFFF00);

				for(int32_t beat_n = c_start_beat; beat_n <= c_end_beat; ++beat_n)
				{
					const float orbit_radius = float(beat_n) * 30;
					const int32_t planet_radius = 6;

					const float phase = float(t) * (Math::tau / float(sampling_frequency) * c_base_freq / c_beat_period) / float(beat_n);
					const int32_t dx = int32_t(Math::Cos(phase) * orbit_radius);
					const int32_t dy = int32_t(Math::Sin(phase) * orbit_radius);

					DrawCircle(window, int32_t(center_x) + dx, int32_t(center_y) + dy, planet_radius, 0x00FFFF00);
				}

				window.Blit();
			}
		}
		else
			Sleep(1);
	}
}

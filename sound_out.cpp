#include <limits>
#include "math.hpp"
#include "window.hpp"

constexpr uint32_t sampling_frequency = 22050;
using SampleType = int16_t;

// Make this size at least a good fraction of the sample rate.
constexpr uint32_t chunk_size = 1024;
// Use double buffering.
constexpr uint32_t num_headers = 3;

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

#ifdef DEBUG
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	DrawableWindow window("4k_sound", 320, 200);

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
			header.dwFlags = 0;
			header.lpData = reinterpret_cast<LPSTR>(buffer_data[block_index]);
			// Calculate a new chunk of data
			for(uint32_t i = 0; i < chunk_size; ++i)
			{
				constexpr float amplitude = float(std::numeric_limits<SampleType>::max()) * 0.95f;
				buffer_data[block_index][i] = SampleType(int32_t(Math::Cos(float(t) * 0.15f) * amplitude));
				window.GetPixels()[t % screen_area] = buffer_data[block_index][i];
				++t;
			}
			
			waveOutPrepareHeader(wave_out, &header, sizeof(WAVEHDR));
			waveOutWrite(wave_out, &header, sizeof(WAVEHDR));
			waveOutUnprepareHeader(wave_out, &header, sizeof(WAVEHDR));

			++block_index_linear;
		}
		else
			Sleep(1);

		if(block_index_linear % 3 == 0)
		{
			window.ProcessMessages();
			window.Blit();
		}
	}
}

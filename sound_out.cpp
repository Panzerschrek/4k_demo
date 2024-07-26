#include "math.hpp"
#include "window.hpp"

constexpr uint32_t sampling_frequency = 22050;
using SampleType = int8_t;

// Make this size at least a good fraction of the sample rate.
constexpr uint32_t chunk_size = 1024;
// Use double buffering.
constexpr uint32_t num_headers = 3;

using BufferData = SampleType[num_headers][chunk_size];

#ifdef DEBUG
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	DrawableWindow window("4k_sound", 320, 200);

	uint32_t iteration = 0;

	HWAVEOUT wave_out = 0;
	WAVEFORMATEX wfx{ WAVE_FORMAT_PCM, 1, sampling_frequency, sampling_frequency * sizeof(SampleType), alignof(SampleType), 8 * sizeof(SampleType), 0 };
	waveOutOpen(&wave_out, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

	WAVEHDR headers[num_headers];
	
	// For now allocate memory from heap.
	// Using static array increases executable size.
	// Using stack array requires stack checking function.
	auto& buffer_data = *reinterpret_cast<BufferData*>(VirtualAlloc(nullptr, sizeof(BufferData), MEM_COMMIT, PAGE_READWRITE));

	for(uint32_t i = 0; i < num_headers; ++i)
	{
		headers[i].dwFlags = WHDR_DONE;
		headers[i].dwBufferLength = chunk_size * sizeof(SampleType);
	}

	uint32_t block_index = 0;
	// This demonstrates sending data as chunks into the output headers
	// Note no callback is needed because we test the dwFlags for WHDR_DONE
	uint32_t t = 0;
	while(true)
	{
		WAVEHDR& header = headers[block_index];
		// Fill any buffers that are "done"
		if((header.dwFlags & WHDR_DONE) != 0)
		{
			header.dwFlags = 0;
			header.lpData = reinterpret_cast<LPSTR>(buffer_data[block_index]);
			// Calculate a new chunk of data
			for(uint32_t i = 0; i < chunk_size; ++i)
			{
				buffer_data[block_index][i] = SampleType(int32_t(Math::Cos(float(t) * 0.05f) * 64.0f));
				++t;
			}
			
			waveOutPrepareHeader(wave_out, &header, sizeof(WAVEHDR));
			waveOutWrite(wave_out, &header, sizeof(WAVEHDR));
			waveOutUnprepareHeader(wave_out, &header, sizeof(WAVEHDR));

			block_index = (block_index + 1) % num_headers;
		}
		else
			Sleep(1);

		// Draw.
		if(iteration % 32u == 0 )
		{
			window.ProcessMessages();

			for (uint32_t y = 0; y < window.GetHeight(); ++y)
				for (uint32_t x = 0; x < window.GetWidth(); ++x)
					window.GetPixels()[x + y * window.GetWidth()] =
					(x * 255u / window.GetWidth()) |
					((y * 255u / window.GetHeight()) << 8) |
					(((iteration << 1) & 255) << 16);

			window.Blit();
		}
	}

	 // waveOutClose(wave_out);
}

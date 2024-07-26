#include "window.hpp"


#ifdef DEBUG
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	DrawableWindow window("4k_sound", 640, 480);

	uint32_t iteration = 0;

	HWAVEOUT wave_out = 0;
	WAVEFORMATEX wfx{ WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&wave_out, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

	// Make this size at least a good fraction of the sample rate.
	constexpr uint32_t chunk_size = 512;
	// Use double buffering.
	constexpr uint32_t num_headers = 2;
	WAVEHDR headers[num_headers];
	char buffer_data[num_headers][chunk_size];

	// Setup all buffers to be "done" so they are filled the first time around.
	for(uint32_t i = 0; i < num_headers; ++i)
		headers[i].dwFlags = WHDR_DONE;

	uint32_t block_index = 0;
	// This demonstrates sending data as chunks into the output headers
	// Note no callback is needed because we test the dwFlags for WHDR_DONE
	DWORD t = 0;
	while(true)
	{
		WAVEHDR* const header = headers + block_index;
		// Fill any buffers that are "done"
		if((header->dwFlags & WHDR_DONE) != 0)
		{
			header->dwFlags = 0;
			header->lpData = buffer_data[block_index];
			// Calculate a new chunk of data
			for(uint32_t i = 0; i < chunk_size; ++i)
			{
				// See http://goo.gl/hQdTi
				buffer_data[block_index][i] = static_cast<char>((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);
				t++;
			}
			header->dwBufferLength = chunk_size;
			waveOutPrepareHeader(wave_out, header, sizeof(WAVEHDR));
			waveOutWrite(wave_out, header, sizeof(WAVEHDR));
			waveOutUnprepareHeader(wave_out, header, sizeof(WAVEHDR));
		}

		// Test the next buffer
		++block_index;
		if(block_index == num_headers)
		{
			// If we have tested all buffers, then sleep for a very short while to avoid blocking the whole thread
			block_index = 0;
			Sleep(10);
		}

		// Draw.
		window.ProcessMessages();

		for (uint32_t y = 0; y < window.GetHeight(); ++y)
			for (uint32_t x = 0; x < window.GetWidth(); ++x)
				window.GetPixels()[x + y * window.GetWidth()] =
				(x * 255u / window.GetWidth()) |
				((y * 255u / window.GetHeight()) << 8) |
				(((iteration << 1) & 255) << 16);

		window.Blit();
	}

	waveOutClose(wave_out);
}

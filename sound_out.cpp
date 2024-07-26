#include "window.hpp"


#ifdef DEBUG
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
	DrawableWindow window("4k_sound", 640, 480);

	uint32_t iteration = 0;

	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

	// Make this size at least a good fraction of the sample rate.
	const DWORD chunkSize = 500;
	// It is good to have at least two buffers. One buffer causes crackles in the sound output.
	const DWORD numHeaders = 2;
	WAVEHDR headers[numHeaders];
	char bufferData[numHeaders][chunkSize];

	// Setup all buffers to be "done" so they are filled the first time around
	for (int i = 0; i < numHeaders; i++)
	{
		headers[i].dwFlags = WHDR_DONE;
	}

	DWORD blockIndex = 0;
	// This demonstrates sending data as chunks into the output headers
	// Note no callback is needed because we test the dwFlags for WHDR_DONE
	DWORD t = 0;
	while (true)
	{
		WAVEHDR* header = headers + blockIndex;
		// Fill any buffers that are "done"
		if (header->dwFlags & WHDR_DONE)
		{
			header->dwFlags = 0;
			header->lpData = bufferData[blockIndex];
			// Calculate a new chunk of data
			for (int i = 0; i < chunkSize; i++)
			{
				// See http://goo.gl/hQdTi
				bufferData[blockIndex][i] = static_cast<char>((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);
				t++;
			}
			header->dwBufferLength = chunkSize;
			waveOutPrepareHeader(hWaveOut, header, sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, header, sizeof(WAVEHDR));
			waveOutUnprepareHeader(hWaveOut, header, sizeof(WAVEHDR));
		}

		// Test the next buffer
		blockIndex++;
		if (blockIndex == numHeaders)
		{
			// If we have tested all buffers, then sleep for a very short while to avoid blocking the whole thread
			blockIndex = 0;
			Sleep(10);
		}

		window.ProcessMessages();

		for (uint32_t y = 0; y < window.GetHeight(); ++y)
			for (uint32_t x = 0; x < window.GetWidth(); ++x)
				window.GetPixels()[x + y * window.GetWidth()] =
				(x * 255u / window.GetWidth()) |
				((y * 255u / window.GetHeight()) << 8) |
				(((iteration << 1) & 255) << 16);

		window.Blit();

		Sleep(16);
	}

	waveOutClose(hWaveOut);
}

#include "window.hpp"

enum class TetrisBlock : uint8_t
{
	Empty,
	I,
	J,
	L,
	O,
	S,
	T,
	Z,
};

constexpr uint32_t g_tetris_field_width  = 10;
constexpr uint32_t g_tetris_field_height = 20;

constexpr uint32_t g_cell_size = 20;

static void DrawFieldBorders(DrawableWindow& window)
{
	const DrawableWindow::PixelType border_color = 0xFFFFFFFF;

	for(uint32_t x = 0; x < g_cell_size * g_tetris_field_width; ++x)
	{
		const uint32_t y0 = 0;
		const uint32_t y1 = g_cell_size * g_tetris_field_height + 2;
		window.GetPixels()[x + y0 * window.GetWidth()] = border_color;
		window.GetPixels()[x + y1 * window.GetWidth()] = border_color;
	}

	for (uint32_t y = 0; y < g_cell_size * g_tetris_field_height; ++y)
	{
		const uint32_t x0 = 0;
		const uint32_t x1 = g_cell_size * g_tetris_field_width + 2;
		window.GetPixels()[x0 + y * window.GetWidth()] = border_color;
		window.GetPixels()[x1 + y * window.GetWidth()] = border_color;
	}
}


static void DrawQuad(
	DrawableWindow& window,
	const uint32_t cell_x, const uint32_t cell_y,
	const DrawableWindow::PixelType color)
{
	const uint32_t x = 2 + cell_x * g_cell_size;
	const uint32_t y = 2 + (g_tetris_field_height - 1) * g_cell_size - cell_y * g_cell_size;

	for(uint32_t dy = 1; dy < g_cell_size - 1; ++dy)
	for(uint32_t dx = 1; dx < g_cell_size - 1; ++dx)
		window.GetPixels()[x + dx + (y + dy) * window.GetWidth()] = color;
}

int main()
{
	DrawableWindow window("4k_tetris", 640, 480);

	TetrisBlock field[g_tetris_field_width * g_tetris_field_height];

	field[5] = TetrisBlock::I;

	while(true)
	{
		window.ProcessMessages();

		for(uint32_t i = 0; i < window.GetWidth() * window.GetHeight(); ++i)
			window.GetPixels()[i] = 0x00000000;

		DrawFieldBorders(window);

		for(uint32_t y = 0; y < g_tetris_field_height; ++y)
		for(uint32_t x = 0; x < g_tetris_field_width; ++x)
		{
			const TetrisBlock block = field[x + y * g_tetris_field_width];
			if(block != TetrisBlock::Empty)
				DrawQuad(window, x, y, 0xFF00FF00);
		}

		window.Blit();

		Sleep(16);
	}
}

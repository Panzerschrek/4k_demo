#include <array>
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

constexpr uint32_t g_tetris_piece_num_blocks = 4;
constexpr uint32_t g_tetris_num_piece_types = 7;

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

using TetrisPieceBlock = std::array<int32_t, 2>;
using TetrisPieceBlocks = std::array<TetrisPieceBlock, g_tetris_piece_num_blocks>;

struct TetrisPiece
{
	TetrisBlock type = TetrisBlock::I;
	// Signerd coordinate to allow apperiance form screen top.
	TetrisPieceBlocks blocks;
};

TetrisPieceBlocks RotateTetrisPieceBlocks(const TetrisPiece& piece)
{
	if (piece.type == TetrisBlock::O)
		return piece.blocks;

	const auto center = piece.blocks[2];

	std::array<std::array<int32_t, 2>, 4> blocks_transformed;
	for (size_t i = 0; i < 4; ++i)
	{
		const TetrisPieceBlock& block = piece.blocks[i];
		const int32_t rel_x = block[0] - center[0];
		const int32_t rel_y = block[1] - center[1];
		const int32_t new_x = center[0] + rel_y;
		const int32_t new_y = center[1] - rel_x;

		blocks_transformed[i] = { new_x, new_y };
	}

	return blocks_transformed;
}

constexpr std::array<TetrisPieceBlocks, g_tetris_num_piece_types> g_tetris_pieces_blocks =
{ {
	{{ { 4, -4}, {4, -3}, {4, -2}, {4, -1} }}, // I
	{{ { 4, -1}, {5, -1}, {5, -2}, {5, -3} }}, // J
	{{ { 5, -1}, {4, -1}, {4, -2}, {4, -3} }}, // L
	{{ { 4, -2}, {5, -2}, {4, -1}, {5, -1} }}, // O
	{{ { 4, -1}, {5, -1}, {5, -2}, {6, -2} }}, // S
	{{ { 4, -1}, {6, -1}, {5, -1}, {5, -2} }}, // T
	{{ { 5, -1}, {6, -1}, {5, -2}, {4, -2} }}, // Z
} };


int main()
{
	DrawableWindow window("4k_tetris", 640, 480);

	TetrisBlock field[g_tetris_field_width * g_tetris_field_height];

	TetrisPiece active_piece{ TetrisBlock::I, g_tetris_pieces_blocks[0] };

	field[5] = TetrisBlock::I;

	for (auto& piece_block : active_piece.blocks)
	{
		piece_block[1] += 3;
	}

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

		for(const auto& piece_block : active_piece.blocks)
		{
			if (piece_block[0] >= 0 && piece_block[0] < int32_t(g_tetris_field_width) &&
				piece_block[1] >= 0 && piece_block[1] < int32_t(g_tetris_field_height))
			{
				DrawQuad(window, piece_block[0], piece_block[1], 0xFF00FF00);
			}
		}

		window.Blit();

		Sleep(16);
	}
}

#include <array>
#include <optional>
#include "math.hpp"
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
	const uint32_t y = 2 + cell_y * g_cell_size;

	for(uint32_t dy = 1; dy < g_cell_size - 1; ++dy)
	for(uint32_t dx = 1; dx < g_cell_size - 1; ++dx)
		window.GetPixels()[x + dx + (y + dy) * window.GetWidth()] = color;
}

using TetrisPieceBlock = std::array<int8_t, 2>;
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

	std::array<std::array<int8_t, 2>, 4> blocks_transformed;
	for (size_t i = 0; i < 4; ++i)
	{
		const TetrisPieceBlock& block = piece.blocks[i];
		const int32_t rel_x = block[0] - center[0];
		const int32_t rel_y = block[1] - center[1];
		const int32_t new_x = center[0] + rel_y;
		const int32_t new_y = center[1] - rel_x;

		blocks_transformed[i] = { int8_t(new_x), int8_t(new_y) };
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

constexpr std::array<DrawableWindow::PixelType, g_tetris_num_piece_types> g_piece_colors =
{
	0x00FF0000, // I
	0x00F0F0F0, // J
	0x00FF00FF, // L
	0x000000FF, // O
	0x0000FF00, // S
	0x00A0A000, // T
	0x0000FFFF, // Z
};

static bool has_move_left = false;
static bool has_move_right = false;
static bool has_move_down = false;
static bool has_rotate = false;

static LRESULT CALLBACK TetrisWindowProc(const HWND hwnd, const UINT msg, const WPARAM w_param, const LPARAM l_param)
{
	if(msg == WM_KEYDOWN)
	{
		switch(w_param)
		{
		case VK_LEFT:
			has_move_left= true;
			break;
		case VK_RIGHT:
			has_move_right= true;
			break;
		case VK_UP:
			has_rotate= true;
			break;
		case VK_DOWN:
			has_move_down = true;
			break;
		};
	}

	return DrawableWindow::WindowProc(hwnd, msg, w_param, l_param);
}

int main()
{
	LARGE_INTEGER start_ticks;
	QueryPerformanceCounter(&start_ticks);

	LARGE_INTEGER ticks_per_second;
	QueryPerformanceFrequency(&ticks_per_second);

	const float ticks_frequency = 4.0f;
	float prev_tick_time = 0.0f;

	DrawableWindow window("4k_tetris", 640, 480, TetrisWindowProc);

	// TODO - zero field properly.
	TetrisBlock field[g_tetris_field_width * g_tetris_field_height];
	for (uint32_t i = 0; i < g_tetris_field_width; ++i)
		field[i + (g_tetris_field_height - 1) * g_tetris_field_width] = TetrisBlock(i % uint32_t(g_tetris_num_piece_types + 1));

	std::optional<TetrisPiece> active_piece = TetrisPiece{ TetrisBlock::I, g_tetris_pieces_blocks[0] };
	TetrisBlock next_piece_type = TetrisBlock::O;

	while(true)
	{
		// Proces input.
		has_move_left = false;
		has_move_right = false;
		has_move_down = false;
		has_rotate = false;
		window.ProcessMessages();

		// Query time.
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		const float current_time = ticks_frequency * float(uint32_t(now.QuadPart - start_ticks.QuadPart)) / float(uint32_t(ticks_per_second.QuadPart));

		if(int(prev_tick_time) < int(current_time))
		{
			prev_tick_time = current_time;

			// It's time to move active piece down.

			if(active_piece == std::nullopt)
			{
				// No active piece - try to spawn new piece.
				TetrisPiece next_active_piece;
				next_active_piece.type = next_piece_type;
				next_active_piece.blocks = g_tetris_pieces_blocks[uint32_t(next_active_piece.type) - uint32_t(TetrisBlock::I)];

				bool can_move = true;
				for(const TetrisPieceBlock& piece_block : next_active_piece.blocks)
				{
					if(piece_block[1] == int32_t(g_tetris_field_height - 1))
						can_move = false;

					const auto next_x = piece_block[0];
					const auto next_y = piece_block[1] + 1;
					if (next_x >= 0 && next_x < int32_t(g_tetris_field_width) &&
						next_y >= 0 && next_y < int32_t(g_tetris_field_height) &&
						field[uint32_t(next_x) + uint32_t(next_y) * g_tetris_field_width] != TetrisBlock::Empty)
						can_move = false;
				}

				if (can_move)
					active_piece = next_active_piece;
				else
				{
					// game_over_ = true;
				}
			}
			else
			{
				bool can_move = true;
				for(const TetrisPieceBlock& piece_block : active_piece->blocks)
				{
					if(piece_block[1] == int32_t(g_tetris_field_height - 1))
						can_move = false;

					const auto next_x = piece_block[0];
					const auto next_y = piece_block[1] + 1;
					if (next_x >= 0 && next_x < int32_t(g_tetris_field_width) &&
						next_y >= 0 && next_y < int32_t(g_tetris_field_height) &&
						field[uint32_t(next_x) + uint32_t(next_y) * g_tetris_field_width] != TetrisBlock::Empty)
						can_move = false;
				}

				if(can_move)
				{
					for(auto& piece_block : active_piece->blocks)
						piece_block[1] += 1;
				}
				else
				{
					// Put piece into field.
					for(const TetrisPieceBlock& piece_block : active_piece->blocks)
					{
						if (piece_block[1] < 0)
						{
							// HACK! prevent overflow.
							// game_over_ = true;
							break;
						}
						field[uint32_t(piece_block[0]) + uint32_t(piece_block[1]) * g_tetris_field_width] = active_piece->type;
					}

					// TryRemoveLines();

					active_piece = std::nullopt;
				}
			}
		}

		// Process logic.
		if(active_piece != std::nullopt)
		{
			const auto try_side_move_piece =
				[&](const int32_t delta)
			{
				bool can_move = true;
				for(const TetrisPieceBlock& piece_block : active_piece->blocks)
				{
					const auto next_x = piece_block[0] + delta;
					const auto next_y = piece_block[1];
					if (next_x < 0 || next_x >= int32_t(g_tetris_field_width) ||
						(next_y >= 0 && next_y < int32_t(g_tetris_field_height) && field[uint32_t(next_x) + uint32_t(next_y) * g_tetris_field_width] != TetrisBlock::Empty))
						can_move = false;
				}

				if(can_move)
					for(auto& piece_block : active_piece->blocks)
						piece_block[0] += delta;
			};

			if(has_move_left)
				try_side_move_piece(-1);
			if(has_move_right)
				try_side_move_piece(1);

			if(has_move_down)
			{
				bool can_move = true;
				for(const TetrisPieceBlock& piece_block : active_piece->blocks)
				{
					const auto next_x = piece_block[0];
					const auto next_y = piece_block[1] + 1;
					if (next_y >= int32_t(g_tetris_field_height) ||
						(next_y >= 0 && field[uint32_t(next_x) + uint32_t(next_y) * g_tetris_field_width] != TetrisBlock::Empty))
						can_move = false;
				}

				if(can_move)
				{
					for(auto& piece_block : active_piece->blocks)
						piece_block[1] += 1;
				}
			}

			if(has_rotate)
			{
				const TetrisPieceBlocks blocks_rotated = RotateTetrisPieceBlocks(*active_piece);

				bool can_rotate = true;
				for(const TetrisPieceBlock& block : blocks_rotated)
				{
					if (block[0] < 0 || block[0] >= int32_t(g_tetris_field_width) ||
						block[1] >= int32_t(g_tetris_field_height) ||
						(block[1] >= 0 && field[uint32_t(block[0]) + uint32_t(block[1]) * g_tetris_field_width] != TetrisBlock::Empty))
					{
						can_rotate = false;
					}
				}

				if(can_rotate)
					active_piece->blocks = blocks_rotated;
			}
		}

		// Draw.

		for(uint32_t i = 0; i < window.GetWidth() * window.GetHeight(); ++i)
			window.GetPixels()[i] = 0x00000000;

		DrawFieldBorders(window);

		for(uint32_t y = 0; y < g_tetris_field_height; ++y)
		for(uint32_t x = 0; x < g_tetris_field_width; ++x)
		{
			const TetrisBlock block = field[x + y * g_tetris_field_width];
			if(block != TetrisBlock::Empty)
				DrawQuad(window, x, y, g_piece_colors[size_t(block) - 1]);
		}

		if(active_piece != std::nullopt)
		{
			for (const auto& piece_block : active_piece->blocks)
			{
				if (piece_block[0] >= 0 && piece_block[0] < int32_t(g_tetris_field_width) &&
					piece_block[1] >= 0 && piece_block[1] < int32_t(g_tetris_field_height))
				{
					DrawQuad(window, piece_block[0], piece_block[1], g_piece_colors[size_t(active_piece->type) - 1]);
				}
			}
		}

		window.Blit();

		Sleep(16);
	}
}

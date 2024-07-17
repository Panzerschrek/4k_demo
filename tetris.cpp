#include <array>
#include <optional>
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

constexpr uint32_t g_field_width  = 10;
constexpr uint32_t g_field_height = 20;

constexpr uint32_t g_piece_num_blocks = 4;
constexpr uint32_t g_num_piece_types = 7;

constexpr uint32_t g_cell_size = 24;

constexpr uint32_t g_border_width = 4;
constexpr uint32_t g_inner_padding = 2;

constexpr uint32_t g_window_width = g_cell_size * (g_field_width + 7) + g_border_width * 2 + g_inner_padding * 2;
constexpr uint32_t g_window_height = g_cell_size * g_field_height + g_border_width * 2 + g_inner_padding * 2;

static void DrawFieldBorders(DrawableWindow& window)
{
	const DrawableWindow::PixelType border_color = 0x00D0D0D0;

	DrawableWindow::PixelType* const pixels = window.GetPixels();
	const uint32_t window_width = window.GetWidth();

	for(uint32_t x = 0; x < g_cell_size * g_field_width + g_border_width * 2 + g_inner_padding * 2; ++x)
	{
		const uint32_t y0 = 0;
		const uint32_t y1 = g_cell_size * g_field_height + g_border_width + g_inner_padding * 2;
		for(uint32_t d = 0; d < g_border_width; ++d)
		{
			pixels[x + (y0 + d) * window_width] = border_color;
			pixels[x + (y1 + d) * window_width] = border_color;
		}
	}

	for(uint32_t y = 0; y < g_cell_size * g_field_height + g_border_width * 2 + g_inner_padding * 2; ++y)
	{
		const uint32_t x0 = 0;
		const uint32_t x1 = g_cell_size * g_field_width + g_border_width + g_inner_padding * 2;
		const auto dst_line = pixels + y * window_width;
		for(uint32_t d = 0; d < g_border_width; ++d)
		{
			dst_line[d + x0] = border_color;
			dst_line[d + x1] = border_color;
		}
	}
}

static void DrawQuad(
	DrawableWindow& window,
	const uint32_t cell_x, const uint32_t cell_y,
	const DrawableWindow::PixelType color)
{
	const uint32_t x = g_border_width + g_inner_padding + cell_x * g_cell_size;
	const uint32_t y = g_border_width + g_inner_padding + cell_y * g_cell_size;

	DrawableWindow::PixelType* const pixels = window.GetPixels();
	const uint32_t window_width = window.GetWidth();

	for(uint32_t dy = 1; dy < g_cell_size - 1; ++dy)
	{
		const auto dst_line = pixels + (y + dy) * window_width;
		for(uint32_t dx = 1; dx < g_cell_size - 1; ++dx)
			dst_line[x + dx] = color;
	}
}

using PieceBlock = std::array<int8_t, 2>;
using PieceBlocks = std::array<PieceBlock, g_piece_num_blocks>;

struct Piece
{
	TetrisBlock type = TetrisBlock::I;
	// Signerd coordinate to allow apperiance form screen top.
	PieceBlocks blocks;
};

PieceBlocks RotatePieceBlocks(const Piece& piece)
{
	if(piece.type == TetrisBlock::O)
		return piece.blocks;

	const auto center = piece.blocks[2];

	std::array<std::array<int8_t, 2>, g_piece_num_blocks> blocks_transformed;
	for(size_t i = 0; i < g_piece_num_blocks; ++i)
	{
		const PieceBlock& block = piece.blocks[i];
		const int32_t rel_x = block[0] - center[0];
		const int32_t rel_y = block[1] - center[1];
		const int32_t new_x = center[0] + rel_y;
		const int32_t new_y = center[1] - rel_x;

		blocks_transformed[i] = { int8_t(new_x), int8_t(new_y) };
	}

	return blocks_transformed;
}

constexpr std::array<PieceBlocks, g_num_piece_types> g_pieces_blocks =
{ {
	{{ { 4, -4}, {4, -3}, {4, -2}, {4, -1} }}, // I
	{{ { 4, -1}, {5, -1}, {5, -2}, {5, -3} }}, // J
	{{ { 5, -1}, {4, -1}, {4, -2}, {4, -3} }}, // L
	{{ { 4, -2}, {5, -2}, {4, -1}, {5, -1} }}, // O
	{{ { 4, -1}, {5, -1}, {5, -2}, {6, -2} }}, // S
	{{ { 4, -1}, {6, -1}, {5, -1}, {5, -2} }}, // T
	{{ { 5, -1}, {6, -1}, {5, -2}, {4, -2} }}, // Z
} };

constexpr std::array<DrawableWindow::PixelType, g_num_piece_types + 1> g_piece_colors =
{
	0x00000000,
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

inline void ZeroMemoryInline(void* const mem, const size_t size)
{
	// Use this function, because functions like "ZeroMemory" are expanded into "memset", which isn't available without standard library.
	RtlSecureZeroMemory(mem, size);
}

int main()
{
	LARGE_INTEGER start_ticks;
	QueryPerformanceCounter(&start_ticks);

	LARGE_INTEGER ticks_per_second;
	QueryPerformanceFrequency(&ticks_per_second);

	const uint32_t ticks_frequency = 4;
	uint32_t prev_tick_time = 0;

	DrawableWindow window("4k_tetris", g_window_width, g_window_height, TetrisWindowProc);

	// Set color for texts.
	SetTextColor(window.GetBitmapDC(), 0xFFFFFF);
	SetBkColor(window.GetBitmapDC(), 0x00000000);

	TetrisBlock field[g_field_width * g_field_height];
	ZeroMemoryInline(field, sizeof(field));

	std::optional<Piece> active_piece;
	TetrisBlock next_piece_type = TetrisBlock(1 + uint32_t(start_ticks.LowPart) % g_num_piece_types);
	bool game_over = false;
	uint32_t score = 0;

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
		const uint32_t current_time = ticks_frequency * uint32_t(now.QuadPart - start_ticks.QuadPart) / uint32_t(ticks_per_second.QuadPart);

		if(prev_tick_time < current_time)
		{
			prev_tick_time = current_time;

			// It's time to move active piece down.
			if(!game_over)
			{
				if(active_piece == std::nullopt)
				{
					// No active piece - try to spawn new piece.
					active_piece = Piece{ next_piece_type, g_pieces_blocks[uint32_t(next_piece_type) - uint32_t(TetrisBlock::I)] };
					next_piece_type = TetrisBlock(1 + uint32_t(now.LowPart) % g_num_piece_types);
				}
				else
				{
					bool can_move = true;
					for(const PieceBlock& piece_block : active_piece->blocks)
					{
						if(piece_block[1] == int32_t(g_field_height - 1))
							can_move = false;

						const auto next_x = piece_block[0];
						const auto next_y = piece_block[1] + 1;
						if (next_x >= 0 && next_x < int32_t(g_field_width) &&
							next_y >= 0 && next_y < int32_t(g_field_height) &&
							field[uint32_t(next_x) + uint32_t(next_y) * g_field_width] != TetrisBlock::Empty)
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
						for(const PieceBlock& piece_block : active_piece->blocks)
						{
							if(piece_block[1] < 0)
								game_over = true;
							else
								field[uint32_t(piece_block[0]) + uint32_t(piece_block[1]) * g_field_width] = active_piece->type;
						}

						active_piece = std::nullopt;

						// Remove lines.
						uint32_t lines_removed = 0;
						for(uint32_t y = g_field_height - 1;;)
						{
							bool line_is_full = true;
							for(uint32_t x = 0; x < g_field_width; ++x)
								line_is_full &= field[x + y * g_field_width] != TetrisBlock::Empty;

							if(line_is_full)
							{
								++lines_removed;

								// Remove this line.
								for(uint32_t dst_y = y; ; --dst_y)
								{
									if(dst_y == 0)
									{
										for(uint32_t x = 0; x < g_field_width; ++x)
											field[x + dst_y * g_field_width] = TetrisBlock::Empty;
									}
									else
									{
										const uint32_t src_y = dst_y - 1;

										for(uint32_t x = 0; x < g_field_width; ++x)
										{
											field[x + dst_y * g_field_width] = field[x + src_y * g_field_width];
											field[x + src_y * g_field_width] = TetrisBlock::Empty;
										}
									}

									if(dst_y == 0)
										break;
								} // Shift lines after removal.
							}
							else if(y > 0)
								--y;
							else
								break;
						}

						static const uint8_t score_add_for_lines[5]{0, 1, 2, 4, 8};
						score += score_add_for_lines[lines_removed];
					}
				}
			}
		}

		// Process logic.
		if(!game_over && active_piece != std::nullopt)
		{
			const int32_t x_deltas[2]{ has_move_left ? -1 : 0, has_move_right ? 1 : 0 };
			for(const int32_t delta : x_deltas)
			{
				bool can_move = true;
				for(const PieceBlock& piece_block : active_piece->blocks)
				{
					const auto next_x = piece_block[0] + delta;
					const auto next_y = piece_block[1];
					if (next_x < 0 || next_x >= int32_t(g_field_width) ||
						(next_y >= 0 && next_y < int32_t(g_field_height) && field[uint32_t(next_x) + uint32_t(next_y) * g_field_width] != TetrisBlock::Empty))
						can_move = false;
				}

				if(can_move)
					for(auto& piece_block : active_piece->blocks)
						piece_block[0] += delta;
			}

			if(has_move_down)
			{
				bool can_move = true;
				for(const PieceBlock& piece_block : active_piece->blocks)
				{
					const auto next_x = piece_block[0];
					const auto next_y = piece_block[1] + 1;
					if (next_y >= int32_t(g_field_height) ||
						(next_y >= 0 && field[uint32_t(next_x) + uint32_t(next_y) * g_field_width] != TetrisBlock::Empty))
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
				const PieceBlocks blocks_rotated = RotatePieceBlocks(*active_piece);

				bool can_rotate = true;
				for(const PieceBlock& block : blocks_rotated)
				{
					if (block[0] < 0 || block[0] >= int32_t(g_field_width) ||
						block[1] >= int32_t(g_field_height) ||
						(block[1] >= 0 && field[uint32_t(block[0]) + uint32_t(block[1]) * g_field_width] != TetrisBlock::Empty))
						can_rotate = false;
				}

				if(can_rotate)
					active_piece->blocks = blocks_rotated;
			}
		}

		// Draw.

		ZeroMemoryInline(window.GetPixels(), window.GetWidth() * window.GetHeight() * sizeof(DrawableWindow::PixelType));

		DrawFieldBorders(window);

		for(uint32_t y = 0; y < g_field_height; ++y)
		for(uint32_t x = 0; x < g_field_width ; ++x)
			DrawQuad(window, x, y, g_piece_colors[size_t(field[x + y * g_field_width])]);

		if(active_piece != std::nullopt)
		{
			for(const auto& piece_block : active_piece->blocks)
			{
				if(piece_block[1] >= 0)
					DrawQuad(window, piece_block[0], piece_block[1], g_piece_colors[size_t(active_piece->type)]);
			}
		}

		if(!game_over)
			for(const auto& piece_block : g_pieces_blocks[uint32_t(next_piece_type) - 1])
				DrawQuad(window, piece_block[0] + g_field_width - 3, piece_block[1] + 5, g_piece_colors[size_t(next_piece_type)]);

		if(game_over)
		{
			RECT rect{ 0, 0, g_field_width * g_cell_size + (g_border_width + g_inner_padding) * 2, g_window_height };
			DrawTextA(window.GetBitmapDC(), "game over", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		if(false) // Currently didabled - adds to much code.
		{
			static char score_line[]= "Score: 000";
			score_line[9] = '0' + score % 10u;
			score_line[8] = '0' + score / 10u % 10u;
			score_line[7] = '0' + score / 100u;

			RECT rect{ g_window_width - 120, g_window_height - 40, g_window_width, g_window_height };
			DrawTextA(window.GetBitmapDC(), score_line, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		window.Blit();

		Sleep(15);
	}
}

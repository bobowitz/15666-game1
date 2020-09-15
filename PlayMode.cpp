#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"
#include "read_write_chunk.hpp"
#include "data_path.hpp"
#include <iostream>
#include <fstream>

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <ctime>

PlayMode::PlayMode()
{
	std::ifstream asset_file;
	asset_file.open(data_path("asset_data"));

	std::vector<PPU466::Palette> v_palettes;
	read_chunk(asset_file, "pals", &v_palettes);
	std::copy(v_palettes.begin(), v_palettes.begin() + ppu.palette_table.size(), ppu.palette_table.begin());

	std::vector<PPU466::Palette> v_tiles;
	read_chunk(asset_file, "tile", &v_tiles);
	std::memcpy(ppu.tile_table.data(), v_tiles.data(), sizeof(ppu.tile_table));

	asset_file.close();

	srand((unsigned int) time(NULL));

	generate_level();
}

PlayMode::~PlayMode()
{
}

void PlayMode::generate_level() {
	player.pos = glm::vec2(128.0f, 160.0f);

	boxes.clear();

	auto add_box = [this](glm::vec2 pos) {
		if (pos == player.pos) return;
		for (auto const &box : boxes) {
			if (box.pos == pos) return;
		}
		boxes.emplace_back(pos);
	};

	for (int i = 0; i < 32; i++)
	{
		glm::vec2 loc(16 * (rand() % 16), 16 * (rand() % 15));
		for (int j = 0; j < 2; j++)
		{
			add_box(loc + (float)j * glm::vec2(16.0f, 0.0f));
		}
	}
	for (int x = 0; x < 256; x += 16)
	{
		add_box(glm::vec2(x, 0.0f));
		add_box(glm::vec2(x, 224.0f));
	}
	for (int y = 16; y < 224; y += 16)
	{
		add_box(glm::vec2(0.0f, y));
		add_box(glm::vec2(240.0f, y));
	}

	level_time = 0.0f;
	num_touched = 0;

	won = false;
	won_time = 0.0f;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_KEYDOWN)
	{
		if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			std::cout << "Skipping room" << std::endl;
			generate_level();
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP)
	{
		if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.pressed = false;
			return true;
		}
	}

	return false;
}

bool collision(PlayMode::Player p, PlayMode::Box b)
{
	if (p.pos.x >= b.pos.x + b.size.x || p.pos.x + p.size.x <= b.pos.x)
		return false;
	if (p.pos.y >= b.pos.y + b.size.y || p.pos.y + p.size.y <= b.pos.y)
		return false;
	return true;
}

void PlayMode::update(float elapsed)
{
	level_time += elapsed;

	if (!won) {
		if (left.pressed)
			player.vel.x -= PLAYER_ACCEL * elapsed * elapsed;
		if (right.pressed)
			player.vel.x += PLAYER_ACCEL * elapsed * elapsed;
		if (up.pressed && player.time_since_touch <= GRACE_JUMP_TIME)
		{
			player.vel.y = JUMP_IMPULSE;
			player.time_since_touch = GRACE_JUMP_TIME + 1.0f;
		}
	}

	if (won && level_time - won_time >= WIN_FREEZE_TIME) {
		generate_level();
	}

	if (player.vel.x > MAX_X_VEL)
		player.vel.x = MAX_X_VEL;
	if (player.vel.x < -MAX_X_VEL)
		player.vel.x = -MAX_X_VEL;
	player.vel.x -= glm::sign(player.vel.x) * PLAYER_DECEL * elapsed * elapsed;
		if (glm::abs(player.vel.x) < MIN_X_VEL)
			player.vel.x = 0.0f;

	player.vel.y -= GRAVITY * elapsed * elapsed;
	player.time_since_touch += elapsed;

	auto touch = [this](Box &box) {
		box.touched_time = level_time;
		num_touched++;
		box.touched = true;
	};

	player.pos.x += player.vel.x * elapsed;
	for (Box &box : boxes)
	{
		if (collision(player, box))
		{
			if (!box.touched) touch(box);
			if (player.vel.x > 0)
			{
				player.pos.x = box.pos.x - player.size.x;
				player.vel.x = 0.0f;
			}
			if (player.vel.x < 0)
			{
				player.pos.x = box.pos.x + box.size.x;
				player.vel.x = 0.0f;
			}
		}
	}

	float delta = player.vel.y * elapsed;
	if (delta > 8.0f) delta = 8.0f;
	if (delta < -8.0f) delta = -8.0f;
	player.pos.y += delta;
	for (Box &box : boxes)
	{
		if (collision(player, box))
		{
			if (!box.touched) touch(box);
			if (player.vel.y > 0)
			{
				player.pos.y = box.pos.y - player.size.y;
				player.vel.y = 0.0f;
			}
			if (player.vel.y < 0)
			{
				player.pos.y = box.pos.y + box.size.y;
				player.vel.y = 0.0f;
				player.time_since_touch = 0.0f;
			}
		}
	}

	for (auto const box : boxes) {
		if (box.touched && level_time - box.touched_time >= BOX_TRANSITION_TIME) {
			for (auto &neighbor : boxes) {
				if (!neighbor.touched && neighbor.pos.x == box.pos.x && glm::abs(neighbor.pos.y - box.pos.y) == 16.0f) touch(neighbor);
				if (!neighbor.touched && neighbor.pos.y == box.pos.y && glm::abs(neighbor.pos.x - box.pos.x) == 16.0f) touch(neighbor);
			}
		}
	}

	if (!won && num_touched == boxes.size()) {
		won = true;
		won_time = level_time;

		std::cout << "100% gold! Moving to the next room" << std::endl;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	space.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	float pct = ((float) num_touched) / ((float) boxes.size());
	ppu.background_color = glm::u8vec4(255, 255 * (1.0 - pct), 255 * (1.0 - pct), 255);

	for (uint32_t x = 0; x < PPU466::BackgroundWidth; x++)
	{
		for (uint32_t y = 0; y < PPU466::BackgroundHeight; y++)
		{
			ppu.background[y * PPU466::BackgroundWidth + x] = (0x0 << 8) | 0xff;
		}
	}
	for (auto const &box : boxes)
	{
		uint32_t x = uint32_t(box.pos.x / 8.0f);
		uint32_t y = uint32_t(box.pos.y / 8.0f);
		uint32_t palette = 0x2 << 8;
		if (box.touched) {
			int palette_num = (int) (glm::min((level_time - box.touched_time) / BOX_TRANSITION_TIME, 1.0f) * 5 + 2);
			palette = palette_num << 8;
		}

		ppu.background[y * PPU466::BackgroundWidth + x] = palette | 0x30;
		ppu.background[y * PPU466::BackgroundWidth + (x+1)] = palette | 0x31;
		ppu.background[(y+1) * PPU466::BackgroundWidth + (x+1)] = palette | 0x21;
		ppu.background[(y+1) * PPU466::BackgroundWidth + x] = palette | 0x20;
	}

	//background scroll:
	//ppu.background_position.x = int32_t(-0.5f * player.player_at.x);
	//ppu.background_position.y = int32_t(-0.5f * player.player_at.y);

	//player sprite:
	glm::vec2 player_draw_pos = player.pos + 0.5f * (player.size - player.drawsize);
	ppu.sprites[0].x = int32_t(player_draw_pos.x);
	ppu.sprites[0].y = int32_t(player_draw_pos.y);
	ppu.sprites[0].index = (player.time_since_touch > GRACE_JUMP_TIME) ? 0x16 : 0x14;
	ppu.sprites[0].attributes = 7;

	ppu.sprites[1].x = int32_t(player_draw_pos.x + 8.0f);
	ppu.sprites[1].y = int32_t(player_draw_pos.y);
	ppu.sprites[1].index = (player.time_since_touch > GRACE_JUMP_TIME) ? 0x17 : 0x15;
	ppu.sprites[1].attributes = 7;

	ppu.sprites[2].x = int32_t(player_draw_pos.x + 8.0f);
	ppu.sprites[2].y = int32_t(player_draw_pos.y + 8.0f);
	ppu.sprites[2].index = (player.time_since_touch > GRACE_JUMP_TIME) ? 0x7 : 0x5;
	ppu.sprites[2].attributes = 7;

	ppu.sprites[3].x = int32_t(player_draw_pos.x);
	ppu.sprites[3].y = int32_t(player_draw_pos.y + 8.0f);
	ppu.sprites[3].index = (player.time_since_touch > GRACE_JUMP_TIME) ? 0x6 : 0x4;
	ppu.sprites[3].attributes = 7;

	//--- actually draw ---
	ppu.draw(drawable_size);
}

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

	for (int i = 0; i < 70; i++)
	{
		glm::vec2 loc(8 * (rand() % 28), 8 * (rand() % 30));
		for (int j = 0; j < 4; j++)
		{
			boxes.emplace_back(loc + (float)j * glm::vec2(8.0f, 0.0f));
		}
	}
	for (int x = 0; x < 256; x += 8)
	{
		boxes.emplace_back(glm::vec2(x, 0.0f));
		boxes.emplace_back(glm::vec2(x, 232.0f));
	}
	for (int y = 8; y < 232; y += 8)
	{
		boxes.emplace_back(glm::vec2(0.0f, y));
		boxes.emplace_back(glm::vec2(248.0f, y));
	}
}

PlayMode::~PlayMode()
{
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
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.downs += 1;
			down.pressed = true;
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
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.pressed = false;
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

	if (left.pressed)
		player.vel.x -= PLAYER_ACCEL * elapsed * elapsed;
	if (right.pressed)
		player.vel.x += PLAYER_ACCEL * elapsed * elapsed;
	if (player.vel.x > MAX_X_VEL)
		player.vel.x = MAX_X_VEL;
	if (player.vel.x < -MAX_X_VEL)
		player.vel.x = -MAX_X_VEL;
	player.vel.x -= glm::sign(player.vel.x) * PLAYER_DECEL * elapsed * elapsed;
	if (glm::abs(player.vel.x) < MIN_X_VEL)
		player.vel.x = 0.0f;
	if (up.pressed && player.time_since_touch <= GRACE_JUMP_TIME)
	{
		player.vel.y = JUMP_IMPULSE;
		player.time_since_touch = GRACE_JUMP_TIME + 1.0f;
	}

	player.vel.y -= GRAVITY * elapsed * elapsed;
	player.time_since_touch += elapsed;

	player.pos.x += player.vel.x * elapsed;
	for (auto const &box : boxes)
	{
		if (collision(player, box))
		{
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

	player.pos.y += player.vel.y * elapsed;
	for (auto const &box : boxes)
	{
		if (collision(player, box))
		{
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

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(255, 255, 255, 255);

	for (uint32_t x = 0; x < PPU466::BackgroundWidth; x++)
	{
		for (uint32_t y = 0; y < PPU466::BackgroundHeight; y++)
		{
			ppu.background[y * PPU466::BackgroundWidth + x] = (0x1 << 8) | 0x11;
		}
	}
	for (auto const &box : boxes)
	{
		uint32_t x = uint32_t(box.pos.x / 8.0f);
		uint32_t y = uint32_t(box.pos.y / 8.0f);
		ppu.background[y * PPU466::BackgroundWidth + x] = (0x1 << 8) | 0x10;
	}

	//background scroll:
	//ppu.background_position.x = int32_t(-0.5f * player.player_at.x);
	//ppu.background_position.y = int32_t(-0.5f * player.player_at.y);

	//player sprite:
	glm::vec2 player_draw_pos = player.pos + 0.5f * (player.size - player.drawsize);
	ppu.sprites[0].x = int32_t(player_draw_pos.x);
	ppu.sprites[0].y = int32_t(player_draw_pos.y);
	ppu.sprites[0].index = (player.time_since_touch > GRACE_JUMP_TIME) ? 1 : 0;
	ppu.sprites[0].attributes = 3;

	//--- actually draw ---
	ppu.draw(drawable_size);
}

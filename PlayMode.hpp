#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//constants:
	const float GRAVITY = 20000.0f;
	const float GRACE_JUMP_TIME = 0.05f; // allow the player to jump for a bit after leaving a platform
	const float PLAYER_DECEL = 20000.0f;
	const float PLAYER_ACCEL = 40000.0f + PLAYER_DECEL;
	const float JUMP_IMPULSE = 150.0f;
	const float MAX_X_VEL = 80.0f;
	const float MIN_X_VEL = 10.0f;

	//player data:
	struct Player
	{
		glm::vec2 pos = glm::vec2(128.0f, 128.0f);
		const glm::vec2 size = glm::vec2(6.0f, 6.0f);
		const glm::vec2 drawsize = glm::vec2(8.0f, 8.0f);
		glm::vec2 vel = glm::vec2(0.0f, 0.0f);
		float time_since_touch = 0.0f;
	};

	Player player;

	//level data:
	struct Box
	{
		Box(glm::vec2 pos_) : pos(pos_){};
		glm::vec2 pos;
		glm::vec2 size = glm::vec2(8.0f, 8.0f);
	};

	std::vector<Box> boxes;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};

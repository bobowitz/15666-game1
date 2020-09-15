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

	//helper functions:
	void generate_level();

	//----- game state -----

	//input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, space, up;

	//constants:
	const float GRAVITY = 60000.0f;
	const float GRACE_JUMP_TIME = 0.05f; // allow the player to jump for a bit after leaving a platform
	const float PLAYER_DECEL = 40000.0f;
	const float PLAYER_ACCEL = 80000.0f + PLAYER_DECEL;
	const float JUMP_IMPULSE = 405.0f;
	const float MAX_X_VEL = 160.0f;
	const float MIN_X_VEL = 10.0f;
	const float BOX_TRANSITION_TIME = 0.1f;
	const float WIN_FREEZE_TIME = 1.0f;

	//player data:
	struct Player
	{
		glm::vec2 pos = glm::vec2(128.0f, 160.0f);
		const glm::vec2 size = glm::vec2(12.0f, 12.0f);
		const glm::vec2 drawsize = glm::vec2(16.0f, 16.0f);
		glm::vec2 vel = glm::vec2(0.0f, -1.0f);
		float time_since_touch = 0.0f;
	};

	Player player;

	//level data:
	struct Box
	{
		Box(glm::vec2 pos_) : pos(pos_){};
		glm::vec2 pos;
		glm::vec2 size = glm::vec2(16.0f, 16.0f);
		bool touched = false;
		float touched_time = 0.0f;
	};

	float level_time = 0.0f;
	int num_touched = 0;
	float won_time = 0.0f;
	bool won = false;

	std::vector<Box> boxes;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};

#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *cur_object = nullptr;
	glm::quat plate_base_rotation;
	glm::quat plate_original_rotation;
	bool is_front = 1;
	int front_oil_count = 1;
	int back_oil_count = 1;

	glm::vec3 plate_move_speed = glm::vec3(-100.f, 5.f, 0.f);

	Scene::Transform* front_oil = nullptr;
	Scene::Transform* back_oil = nullptr;

	glm::vec3 front_oil_pos;
	glm::vec3 back_oil_pos;

	float finish_duration = -1.f;
	float duration_limit = 0.4f;

	std::list<Scene::Transform*> plates_on_table;

	glm::vec3 middle_pos = glm::vec3(-35.0f,60.0f, -2.0f);
	glm::vec3 pos_ofs = glm::vec3(0.f, 0.f, 1000.f);
	glm::vec3 move_speed = glm::vec3(0.f, 5.f, -100.f);

	Scene::Transform *wipe = nullptr;
	glm::vec3 wipe_start_pos = glm::vec3(5.f, -16.0f, 60.f);
	glm::vec3 wipe_end_pos = glm::vec3(5.f, -24.0f, 60.f);
	glm::vec3 wipe_ofs = glm::vec3(0.f, 5.0f, 0.f);
	glm::vec3 wipe_original_pos;
	float wipe_speed = 10.f;
	float wipe_time = 0.f;

	float rotate_plate = -1.0f;
	glm::vec3 rotate_speed = glm::vec3(5.f, 0.f, 0.f);

	//hexapod leg to wobble:
	Scene::Transform *hip = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;

	int credit = 0;
	
	//camera:
	Scene::Camera *camera = nullptr;

	std::vector<std::string> text_list;

	size_t serve_food_text_index;
	size_t wipe_plate_text_index;

	size_t text_index;

};

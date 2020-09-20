#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include <string>

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("brunch.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("brunch.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	srand (time(NULL));
	
	serve_food_text_index = text_list.size();
	text_list.emplace_back("Press W to serve foods");
	
	wipe_plate_text_index = text_list.size();
	text_list.emplace_back("Press SPACE to wipe the plate; Press D to flip the plate; Press A to finish");


	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name.compare("oil.1") == 0)
		{
			front_oil = &transform;
			front_oil_pos = transform.position;
		}else if (transform.name.compare("oil.2") == 0)
		{
			back_oil = &transform;
			back_oil_pos = transform.position;
		}
		if (transform.name.compare("wipe") == 0)
		{
			wipe_original_pos = transform.position;
			wipe = &transform;
			continue;
		}
		if (transform.parent == nullptr || transform.parent->name.compare("table") != 0)
		{	
			continue;
		}
		if (transform.name.compare("plant vase") == 0 || transform.name.compare("cactus") == 0)
		{
			continue;
		}
		if (transform.name.compare("dirty_plate") == 0)
		{
			plate_original_rotation = transform.rotation;
		}
		
		plates_on_table.emplace_back(&transform);
		transform.position += pos_ofs;
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.pressed = true;
			return true;
		}
		
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}else if(evt.key.keysym.sym == SDLK_SPACE){
			space.pressed = false;
			return true;
		}
	} 

	return false;
}

void PlayMode::update(float elapsed) {

	if (cur_object == nullptr || finish_duration >= duration_limit)
	{
		if (cur_object)
		{
			cur_object->position += pos_ofs;
			if (cur_object->name.compare("dirty_plate") != 0)
			{
				cur_object->position -= finish_duration * move_speed;
			}else{
				cur_object->position -= finish_duration * plate_move_speed;
				cur_object->rotation = plate_original_rotation;
			}

			front_oil_count = 1;
			back_oil_count = 1;
			front_oil->position = front_oil_pos;
			back_oil->position = back_oil_pos;
		}

		int rand = random() % plates_on_table.size();
		auto trans_it = plates_on_table.begin();
		std::advance(trans_it, rand);
		cur_object = (*trans_it);
		plate_base_rotation = cur_object->rotation;

		cur_object->position -= pos_ofs;

		rand = random() % 4;
		if (rand==0)
		{
			front_oil_count = 0;
			back_oil_count = 0;
			front_oil->position += pos_ofs;
			back_oil->position += pos_ofs;
		}else if (rand == 1)
		{
			front_oil_count = 1;
			back_oil_count = 0;
			back_oil->position += pos_ofs;
		}else if (rand == 2)
		{
			back_oil_count = 1;
			front_oil_count = 0;
			front_oil->position += pos_ofs;
		}

		finish_duration = -1.f;
		rotate_plate = -1.f;

		if (cur_object->name.compare("dirty_plate") == 0)
		{
			text_index = wipe_plate_text_index;
		}else{
			text_index = serve_food_text_index;
		}
		
		is_front = 1;

		return;
	}
	
	if (finish_duration >= 0.f && finish_duration < duration_limit)
	{
		finish_duration += elapsed;
		if (cur_object->name.compare("dirty_plate") != 0)
		{
			cur_object->position += elapsed * move_speed;
		}else{
			cur_object->position += elapsed * plate_move_speed;
		}
		return;
	}

	if (rotate_plate >= 0.f)
	{
		rotate_plate += elapsed * 200.f;
		rotate_plate = (rotate_plate >= 180.f) ? 180.f : rotate_plate;
		cur_object->rotation = plate_base_rotation * glm::angleAxis(
			glm::radians(rotate_plate),
			glm::vec3(1.0f, 0.f, 0.f)
		);
		if (rotate_plate >= 180.f)
		{
			rotate_plate = -1.f;
			plate_base_rotation = cur_object->rotation;
		}
		return;
	}

	// wiping
	if (wipe->position != wipe_original_pos)
	{
		if (wipe_time > duration_limit)
		{
			wipe->position = wipe_original_pos;
			wipe_time = 0.f;
		}else{
			wipe->position.z += wipe_speed * elapsed;
			wipe_time += elapsed;
		}
		return;
	}

	// serve food
	if (up.pressed && cur_object != nullptr && cur_object->name != "" && cur_object->name.compare("dirty_plate") != 0)
	{
		finish_duration = 0.f;
		credit += 1;
		return;
	}

	if (down.pressed && rotate_plate < 0.f && cur_object != nullptr && cur_object->name != ""  && cur_object->name.compare("dirty_plate") == 0)
	{
		rotate_plate = 0.f;
		is_front = is_front ? 0 : 1;
		return;
	}

	if (space.pressed && wipe->position == wipe_original_pos && cur_object->name.compare("dirty_plate") == 0)
	{
		wipe->position = wipe_start_pos;
		wipe->position = cur_object->position + wipe_ofs;
		if (is_front)
		{
			if (front_oil_count)
			{
				front_oil_count = 0;
				front_oil->position += pos_ofs;
			}
		}else{
			if (back_oil_count)
			{
				back_oil_count = 0;
				back_oil->position += pos_ofs;
			}
		}
		return;
	}

	if (left.pressed && finish_duration < 0.f && wipe->position == wipe_original_pos 
	&& cur_object->name.compare("dirty_plate") == 0 && front_oil_count == 0 && back_oil_count == 0 
	&& is_front == 1)
	{
		finish_duration = 0.f;
		return;
	}
	
	//reset button press counters:
	left.pressed = false;
	right.pressed = false;
	up.pressed = false;
	down.pressed = false;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	GL_ERRORS();
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text(text_list[text_index],
			glm::vec3(-aspect + 0.1f * 1.2 * H, -1.0 + 0.1f * 1.2 * H, 0.0),
			glm::vec3(1.2 * H, 0.0f, 0.0f), glm::vec3(0.0f, 1.2 * H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));

		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(text_list[text_index],
			glm::vec3(-aspect + 0.1f * 1.2 * H + ofs, -1.0 + + 0.1f * 1.2 * H + ofs, 0.0),
			glm::vec3(1.2 * H, 0.0f, 0.0f), glm::vec3(0.0f, 1.2 * H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		std::string cur_credit = "Credits: " + std::to_string(credit);
		lines.draw_text(cur_credit,
			glm::vec3(-aspect + 0.2f * 1.5 * H, -1.0 + 2.f * H, 0.0),
			glm::vec3(1.5 * H, 0.0f, 0.0f), glm::vec3(0.0f, 1.5 * H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		
		lines.draw_text(cur_credit,
			glm::vec3(-aspect + 0.2f * 1.5 * H + ofs, -1.0 + + 2.f * H + ofs, 0.0),
			glm::vec3(1.5 * H, 0.0f, 0.0f), glm::vec3(0.0f, 1.5 * H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}

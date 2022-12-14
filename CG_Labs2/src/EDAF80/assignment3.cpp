#include "assignment3.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tinyfiledialogs.h>

#include <clocale>
#include <cstdlib>
#include <stdexcept>

edaf80::Assignment3::Assignment3(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 3", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment3::~Assignment3()
{
	bonobo::deinit();
}

void
edaf80::Assignment3::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	GLuint diffuse_shader = 0u;
	program_manager.CreateAndRegisterProgram("Diffuse",
	                                         { { ShaderType::vertex, "EDAF80/diffuse.vert" },
	                                           { ShaderType::fragment, "EDAF80/diffuse.frag" } },
	                                         diffuse_shader);
	if (diffuse_shader == 0u)
		LogError("Failed to load diffuse shader");

	GLuint normal_shader = 0u;
	program_manager.CreateAndRegisterProgram("Normal",
	                                         { { ShaderType::vertex, "EDAF80/normal.vert" },
	                                           { ShaderType::fragment, "EDAF80/normal.frag" } },
	                                         normal_shader);
	if (normal_shader == 0u)
		LogError("Failed to load normal shader");

	GLuint texcoord_shader = 0u;
	program_manager.CreateAndRegisterProgram("Texture coords",
	                                         { { ShaderType::vertex, "EDAF80/texcoord.vert" },
	                                           { ShaderType::fragment, "EDAF80/texcoord.frag" } },
	                                         texcoord_shader);
	if (texcoord_shader == 0u)
		LogError("Failed to load texcoord shader");
    
    // Add Skybox(Cube map) shader
    GLuint skybox_shader = 0u;
    program_manager.CreateAndRegisterProgram("Skybox",
                                             { { ShaderType::vertex, "EDAF80/skybox.vert" },
                                               { ShaderType::fragment, "EDAF80/skybox.frag" } },
                                             skybox_shader);
    if (skybox_shader == 0u)
        LogError("Failed to load skybox shader");
     
    
    // Add Phong shader with normal map avalaible
    GLuint phong_shader = 0u;
    program_manager.CreateAndRegisterProgram("Phong",
                                             { { ShaderType::vertex, "EDAF80/phong_normalMap.vert" },
                                               { ShaderType::fragment, "EDAF80/phong_normalMap.frag" } },
                                             phong_shader);
    if (phong_shader == 0u)
        LogError("Failed to load phong shader");
    
    // Add Toon shader
    GLuint toon_shader = 0u;
    program_manager.CreateAndRegisterProgram("Toon",
                                             { { ShaderType::vertex, "EDAF80/toon.vert" },
                                               { ShaderType::fragment, "EDAF80/toon2.frag" } },
                                             toon_shader);
    if (toon_shader == 0u)
        LogError("Failed to load toon shader");
    
    // Add Outline shader
    GLuint outline_shader = 0u;
    program_manager.CreateAndRegisterProgram("Outline",
                                             { { ShaderType::vertex, "EDAF80/outline.vert" },
                                               { ShaderType::fragment, "EDAF80/outline.frag" } },
                                             outline_shader);
    if (outline_shader == 0u)
        LogError("Failed to load outline shader");
    
    
	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

    
//	bool use_normal_mapping = false;
//	auto camera_position = mCamera.mWorld.GetTranslation();
//	auto const phong_set_uniforms = [&use_normal_mapping,&light_position,&camera_position](GLuint program){
//		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
//		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
//		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
//        glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));
//        glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(glm::vec3(0.7f, 0.2f, 0.4f)));
//        glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
//        glUniform1f(glGetUniformLocation(program, "shininess"), 10.0);
//	};
     


	//
	// Set up the two spheres used.
	//
	auto skybox_shape = parametric_shapes::createSphere(20.0f, 100u, 100u);
	if (skybox_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}

	Node skybox;
	skybox.set_geometry(skybox_shape);
    skybox.set_program(&skybox_shader, set_uniforms);
	
    GLuint cubemap = bonobo::loadTextureCubeMap(
    config::resources_path("cubemaps/NissiBeach2/posx.jpg"),
    config::resources_path("cubemaps/NissiBeach2/negx.jpg"),
    config::resources_path("cubemaps/NissiBeach2/posy.jpg"),
    config::resources_path("cubemaps/NissiBeach2/negy.jpg"),
    config::resources_path("cubemaps/NissiBeach2/posz.jpg"),
    config::resources_path("cubemaps/NissiBeach2/negz.jpg"));
//    config::resources_path("cubemaps/skybox/right.png"),
//    config::resources_path("cubemaps/skybox/left.png"),
//    config::resources_path("cubemaps/skybox/top.png"),
//    config::resources_path("cubemaps/skybox/bottom.png"),
//    config::resources_path("cubemaps/skybox/front.png"),
//    config::resources_path("cubemaps/skybox/back.png"));
    
    //skybox.add_texture("skybox", cubemap, GL_TEXTURE_CUBE_MAP);
    
   
    

	auto demo_shape = parametric_shapes::createSphere(1.5f, 40u, 40u);
	if (demo_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the demo sphere");
		return;
	}

	bonobo::material_data demo_material;
	demo_material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	demo_material.diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	demo_material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
	demo_material.shininess = 10.0f;
    
    bool use_normal_mapping = false;
    // Add if use diffuse & spectular texture
    bool use_diffuse_texture = false;
    bool use_specular_texture = false;
    // Add toon_color_level
    int toon_color_levels = 3;
    auto camera_position = mCamera.mWorld.GetTranslation();
    auto const phong_set_uniforms = [&use_normal_mapping,&use_diffuse_texture, &use_specular_texture,&toon_color_levels,&light_position,&camera_position, &demo_material](GLuint program){
        glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
        glUniform1i(glGetUniformLocation(program, "use_diffuse_texture"), use_diffuse_texture ? 1 : 0);
        glUniform1i(glGetUniformLocation(program, "use_specular_texture"), use_specular_texture ? 1 : 0);
        glUniform1i(glGetUniformLocation(program, "toon_color_levels"), toon_color_levels),
        glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
        glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
        glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(demo_material.ambient));
        glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(demo_material.diffuse));
        glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(demo_material.specular));
        glUniform1f(glGetUniformLocation(program, "shininess"), demo_material.shininess);
    };

	Node demo_sphere;
	demo_sphere.set_geometry(demo_shape);
	//demo_sphere.set_material_constants(demo_material);
	//demo_sphere.set_program(&fallback_shader, phong_set_uniforms); // move into the big loop
    
    GLuint cubemap2 = bonobo::loadTextureCubeMap(
    config::resources_path("cubemaps/LarnacaCastle/posx.jpg"),
    config::resources_path("cubemaps/LarnacaCastle/negx.jpg"),
    config::resources_path("cubemaps/LarnacaCastle/posy.jpg"),
    config::resources_path("cubemaps/LarnacaCastle/negy.jpg"),
    config::resources_path("cubemaps/LarnacaCastle/posz.jpg"),
    config::resources_path("cubemaps/LarnacaCastle/negz.jpg"));
    demo_sphere.add_texture("skybox", cubemap2, GL_TEXTURE_CUBE_MAP);
    skybox.add_texture("skybox", cubemap2, GL_TEXTURE_CUBE_MAP);
    
    GLuint diffuse_texture = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_coll1_2k.jpg"));
    GLuint specular_texture = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_rough_2k.jpg"));
    GLuint normalmap_texture = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_nor_2k.jpg"));
    demo_sphere.add_texture("diffuse_texture", diffuse_texture, GL_TEXTURE_2D);
    demo_sphere.add_texture("specular_texture", specular_texture, GL_TEXTURE_2D);
    demo_sphere.add_texture("normalmap_texture", normalmap_texture, GL_TEXTURE_2D);
    
    //Spaceship Model
    std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects(config::resources_path("spaceship.glb"));
    auto spaceship_shape = objects.front();
    if (spaceship_shape.vao == 0u) {
        LogError("Failed to retrieve the mesh for the spaceship");
        return;
    }
    
    Node spaceship;
    float spaceship_radius = 0.3f;
    glm::vec3 spaceship_initial_positon (5.0f, -2.0f, 0.0f);
    spaceship.set_geometry(spaceship_shape);
    spaceship.set_program(&toon_shader, set_uniforms);
    spaceship.get_transform().SetTranslate(spaceship_initial_positon);
    spaceship.get_transform().SetScale(spaceship_radius * glm::vec3(0.6f));
    spaceship.get_transform().RotateY(glm::pi<float>());
    spaceship.get_transform().RotateX(glm::pi<float>()/4);
    
    GLuint spaceship_texture = bonobo::loadTexture2D(config::resources_path("textures/Colors30x30_0.png"));
    spaceship.add_texture("diffuse_texture", spaceship_texture, GL_TEXTURE_2D);
    //End of spaceship model setting
    // --------------------------------------
    
    //CaptainAmerica Model
    std::vector<bonobo::mesh_data> const capAmerica = bonobo::loadObjects(config::resources_path("CaptainAmerica/CaptainAmerica.obj"));
    auto captain_shape = capAmerica.front();
    if (captain_shape.vao == 0u) {
        LogError("Failed to retrieve the mesh for the captainAmerica");
        return;
    }
    
    Node captain;
    float captain_radius = 0.3f;
    glm::vec3 captain_initial_positon (5.0f, -2.0f, 0.0f);
    captain.set_geometry(captain_shape);
    captain.set_program(&toon_shader, set_uniforms);
    captain.get_transform().SetTranslate(captain_initial_positon);
    captain.get_transform().SetScale(captain_radius * glm::vec3(0.6f));
    //spaceship.get_transform().RotateY(glm::pi<float>());
    //spaceship.get_transform().RotateX(glm::pi<float>()/4);
    // End of CaptainAmerica model setting
    // --------------------------------------
    
    // Bunny Model
    /**
    std::vector<bonobo::mesh_data> const bunnyObj = bonobo::loadObjects(config::resources_path("bunny.obj"));
    auto bunny_shape = bunnyObj.front();
    if (bunny_shape.vao == 0u) {
        LogError("Failed to retrieve the mesh for the bunny");
        return;
    }
    
    Node bunny;
    float bunny_radius = 0.5f;
    glm::vec3 bunny_initial_positon (5.0f, 3.0f, 0.0f);
    captain.set_geometry(bunny_shape);
    captain.set_program(&toon_shader, set_uniforms);
    captain.get_transform().SetTranslate(bunny_initial_positon);
    captain.get_transform().SetScale(bunny_radius * glm::vec3(0.6f));
    // End of bunny model setting
    // --------------------------------------
     **/
    
    



	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
    // Add Stencil Testing
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);


	auto lastTime = std::chrono::high_resolution_clock::now();

	bool use_orbit_camera = false;
	std::int32_t demo_sphere_program_index = 0;
	auto cull_mode = bonobo::cull_mode_t::disabled;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	changeCullMode(cull_mode);

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);
		if (use_orbit_camera) {
			mCamera.mWorld.LookAt(glm::vec3(0.0f));
		}
		camera_position = mCamera.mWorld.GetTranslation();
        //std::cout << camera_position << std::endl;

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);


		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);


		mWindowManager.NewImGuiFrame();

        // Modified for Stencil Oulining (add clear stencil buffer)
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		bonobo::changePolygonMode(polygon_mode);
        
        skybox.get_transform().SetTranslate(camera_position);
        //demo_sphere.set_geometry(demo_shape); // Set as original size
        

        //glDepthFunc(GL_LEQUAL); // for infinity
        glDisable(GL_DEPTH_TEST);
        // Add Stencil: Render skybox without stencil
        glStencilMask(0x00);
		skybox.render(mCamera.GetWorldToClipMatrix());
        // ======Render sphere to stencil=========
        glEnable(GL_DEPTH_TEST);
        // ---------------------------------------
        // 1st render pass: draw object as normal and write to the stencil buffer
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        demo_sphere.set_program(&toon_shader, phong_set_uniforms);
        demo_sphere.get_transform().SetScale(0.6f);
		demo_sphere.render(mCamera.GetWorldToClipMatrix());
        spaceship.set_program(&toon_shader, phong_set_uniforms);
        spaceship.get_transform().SetScale(spaceship_radius * glm::vec3(0.6f));
        spaceship.render(mCamera.GetWorldToClipMatrix());
        /*
        bunny.set_program(&toon_shader, phong_set_uniforms);
        bunny.get_transform().SetScale(bunny_radius * glm::vec3(0.6f));
        bunny.render(mCamera.GetWorldToClipMatrix());
         */
        // ---------------------------------------
        // 2nd render pass: draw slightly scaled object and disable stencil writing.
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);
        demo_sphere.set_program(&outline_shader, phong_set_uniforms);
        demo_sphere.get_transform().SetScale(0.6f * glm::vec3(1.02f));
        demo_sphere.render(mCamera.GetWorldToClipMatrix());
        spaceship.set_program(&outline_shader, phong_set_uniforms);
        spaceship.get_transform().SetScale(spaceship_radius * glm::vec3(0.6f)*glm::vec3(1.02f));
        spaceship.render(mCamera.GetWorldToClipMatrix());
        /*
        bunny.set_program(&outline_shader, phong_set_uniforms);
        bunny.get_transform().SetScale(bunny_radius * glm::vec3(0.6f)*glm::vec3(1.02f));
        bunny.render(mCamera.GetWorldToClipMatrix());
         */
        
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glEnable(GL_DEPTH_TEST);
        
        
        

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool opened = ImGui::Begin("Scene Control", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			auto const cull_mode_changed = bonobo::uiSelectCullMode("Cull mode", cull_mode);
			if (cull_mode_changed) {
				changeCullMode(cull_mode);
			}
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
			auto demo_sphere_selection_result = program_manager.SelectProgram("Demo sphere", demo_sphere_program_index);
			if (demo_sphere_selection_result.was_selection_changed) {
				demo_sphere.set_program(demo_sphere_selection_result.program, phong_set_uniforms);
			}
            
            
			ImGui::Separator();
			//ImGui::Checkbox("Use normal mapping", &use_normal_mapping);
            ImGui::Checkbox("Use diffuse texture", &use_diffuse_texture);
            //ImGui::Checkbox("Use specular texture", &use_specular_texture);
			ImGui::ColorEdit3("Ambient", glm::value_ptr(demo_material.ambient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(demo_material.diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(demo_material.specular));
			ImGui::SliderFloat("Shininess", &demo_material.shininess, 1.0f, 1000.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
			ImGui::Separator();
			ImGui::Checkbox("Use orbit camera", &use_orbit_camera);
			ImGui::Separator();
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		demo_sphere.set_material_constants(demo_material);
        
        

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());

		opened = ImGui::Begin("Render Time", nullptr, ImGuiWindowFlags_None);
		if (opened)
			ImGui::Text("%.3f ms", std::chrono::duration<float, std::milli>(deltaTimeUs).count());
		ImGui::End();

		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
        //glDepthFunc(GL_LESS); // for infinity, set back
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment3 assignment3(framework.GetWindowManager());
		assignment3.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}

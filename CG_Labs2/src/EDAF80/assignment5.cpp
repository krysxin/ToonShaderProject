#include "assignment5.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tinyfiledialogs.h>

#include <clocale>
#include <cstdlib>
#include <stdexcept>

#include <random>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
    mCamera(0.5f * glm::half_pi<float>(),
            static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
            0.01f, 1000.0f),
    inputHandler(), mWindowManager(windowManager), window(nullptr)
{
    WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

    window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
    if (window == nullptr) {
        throw std::runtime_error("Failed to get a window: aborting!");
    }

    bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
    bonobo::deinit();
}

bool edaf80::Assignment5::getCollided(glm::vec3 asteroid_p, float asteroid_r, glm::vec3 ship_p, float ship_r)
{
    return glm::distance(asteroid_p, ship_p) < asteroid_r + ship_r;
}

bool edaf80::Assignment5::inSkybox(float x, float y, float skybox_r)
{
    bool ok_x = x <= skybox_r && x>= -skybox_r;
    bool ok_y = y <= skybox_r && y>= -skybox_r;
    bool ok_area = (x*x + y*y) <= (skybox_r*skybox_r);
    return ok_x && ok_y && ok_area;
}



void
edaf80::Assignment5::run()
{
    // Set up the camera
    glm::vec3 camera_initial_position (-5.0f, -5.0f, 50.0f);
    mCamera.mWorld.SetTranslate(camera_initial_position);
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
    // Default Shader
    GLuint celestial_body_shader = 0u;
    program_manager.CreateAndRegisterProgram("Celestial Body",
                                             { { ShaderType::vertex, "EDAF80/default.vert" },
                                               { ShaderType::fragment, "EDAF80/default.frag" } },
                                             celestial_body_shader);
    if (celestial_body_shader == 0u) {
        LogError("Failed to generate the “Celestial Body” shader program: exiting.");

        bonobo::deinit();

        return EXIT_FAILURE;
    }
    
    // Phong Shader
    GLuint phong_shader = 0u;
    program_manager.CreateAndRegisterProgram("phong", { { ShaderType::vertex, "EDAF80/phong2.vert" },
                                               { ShaderType::fragment, "EDAF80/phong2.frag" } }, phong_shader);


    if (phong_shader == 0u)
        LogError("Failed to load phong shader");
    
    // Diffuse Shader
    GLuint diffuse_shader = 0u;
    program_manager.CreateAndRegisterProgram("Diffuse",
        { { ShaderType::vertex, "EDAF80/diffuse.vert" },
          { ShaderType::fragment, "EDAF80/diffuse.frag" } },
        diffuse_shader);
    if (diffuse_shader == 0u)
        LogError("Failed to load diffuse shader");
    
    // Add Skybox(Cube map) shader
    GLuint skybox_shader = 0u;
    program_manager.CreateAndRegisterProgram("Skybox",
                                             { { ShaderType::vertex, "EDAF80/skybox.vert" },
                                               { ShaderType::fragment, "EDAF80/skybox.frag" } },
                                             skybox_shader);
    if (skybox_shader == 0u)
        LogError("Failed to load skybox shader");
    


    auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
    bool collided = false;
    auto const set_uniforms = [&light_position, &collided](GLuint program) {
        glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
        glUniform1i(glGetUniformLocation(program, "collided"), collided ? 1 : 0);
    };

    //
    // Todo: Insert the creation of other shader programs.
    //       (Check how it was done in assignment 3.)
    //


    //
    // Todo: Load your geometry
    //

    std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects(config::resources_path("scenes/spaceship.glb"));
    auto spaceship_shape = objects.front();
    
    auto skybox_shape = parametric_shapes::createSphere(900.0f, 10u, 10u);
    if (skybox_shape.vao == 0u) {
        LogError("Failed to retrieve the mesh for the skybox");
        return;
    }

   // Spaceship Initial Setting
    Node spaceship;
    float spaceship_radius = 0.3f;
    glm::vec3 spaceship_initial_positon (-5.0f, -5.0f, 0.0f);
    spaceship.set_geometry(spaceship_shape);
    spaceship.set_program(&phong_shader, set_uniforms);
    spaceship.get_transform().SetTranslate(spaceship_initial_positon);
    spaceship.get_transform().SetScale(spaceship_radius * glm::vec3(0.6f));
    spaceship.get_transform().RotateY(glm::pi<float>());
    spaceship.get_transform().RotateX(glm::pi<float>()/4);
    
    GLuint spaceship_texture = bonobo::loadTexture2D(config::resources_path("textures/Colors30x30_0.png"));
    spaceship.add_texture("spaceship_texture", spaceship_texture, GL_TEXTURE_2D);
    // Texture for collision
    //GLuint spaceship_collide_texture = bonobo::loadTexture2D(config::resources_path("planets/2k_sun.jpg"));
    GLuint spaceship_collide_texture = bonobo::loadTexture2D(config::resources_path("textures/fire.jpeg"));
    spaceship.add_texture("spaceship_collide_texture", spaceship_collide_texture, GL_TEXTURE_2D);
    
    Node skybox;
    skybox.set_geometry(skybox_shape);
    skybox.set_program(&skybox_shader, set_uniforms);
    
    GLuint cubemap = bonobo::loadTextureCubeMap(
    config::resources_path("cubemaps/cloudyhills/posx.png"),
    config::resources_path("cubemaps/cloudyhills/negx.png"),
    config::resources_path("cubemaps/cloudyhills/posy.png"),
    config::resources_path("cubemaps/cloudyhills/negy.png"),
    config::resources_path("cubemaps/cloudyhills/posz.png"),
    config::resources_path("cubemaps/cloudyhills/negz.png"));
    skybox.add_texture("skybox_texture", cubemap, GL_TEXTURE_CUBE_MAP);

    
    Node sphere;
    auto sphere_shape = parametric_shapes::createSphere(2.0f, 100u, 100u);
    sphere.set_geometry(sphere_shape);
    sphere.set_program(&celestial_body_shader, set_uniforms);
    sphere.get_transform().SetTranslate(glm::vec3(0.0f, 0.0f, 0.0f));
    sphere.get_transform().RotateY(glm::pi<float>()/2.0f);
    GLuint const sun_texture = bonobo::loadTexture2D(config::resources_path("planets/2k_sun.jpg"));
    GLuint const rainbow_texture = bonobo::loadTexture2D(config::resources_path("textures/rainbow.jpeg"));
    sphere.add_texture("diffuse_texture", rainbow_texture, GL_TEXTURE_2D);

    /*
    auto const control_point_sphere = parametric_shapes::createSphere(1.5f, 40u, 40u);
    std::array<glm::vec3, 9> control_point_locations = {
        glm::vec3(0.0f,  10.0f,  0.0f),
        glm::vec3(11.0f,  10.8f,  14.0f),
        glm::vec3(20.0f,  23.2f,  24.0f),
        glm::vec3(30.0f,  30.0f,  30.0f),
        glm::vec3(35.0f,  44.0f,  30.0f),
        glm::vec3(-20.0f, -23.0f,  -24.0f),
        glm::vec3(-30.0f, -33.0f, -33.0f),
        glm::vec3(-35.0f, -10.2f, -21.0f),
        glm::vec3(-18.0f, -19.8f, -10.0f)
    };
     
     std::array<Node, control_point_locations.size()> control_points;
     for (std::size_t i = 0; i < control_point_locations.size(); ++i) {
         auto& control_point = control_points[i];
         control_point.set_geometry(control_point_sphere);
         control_point.set_program(&diffuse_shader, set_uniforms);
         control_point.get_transform().SetTranslate(control_point_locations[i]);
     }
     */
    
    
    
    // Asteroids Initial Setting
    int const numAsteroids = 100;
    std::array<Node, numAsteroids> asteroids;
    std::array<float, numAsteroids> asteroid_radius;
    glm::vec3 asteroid_v (0.0f, 0.0f, 0.0f);
    float asteroid_acc = 0.1f;
    float asteroid_range_x = 50.f, asteroid_range_y = 50.f;
    //bool collided = false;
    GLuint asteroid_texture = bonobo::loadTexture2D(config::resources_path("textures/asteroid.jpeg"));
    
    // Random number generator for asteroid
    float const mean_radius = 1.0f, std_dev_radius = 0.5f,  MIN_X = -100.0f, MAX_X = 100.0f, MIN_Y = -100.0f, MAX_Y = 100.0f;

    std::random_device seeder;
    std::default_random_engine generator(seeder());
    std::normal_distribution<float> asteroid_radii(mean_radius, std_dev_radius);
    std::uniform_real_distribution<float> asteroid_coord_x(MIN_X, MAX_X);
    std::uniform_real_distribution<float> asteroid_coord_y(MIN_Y, MAX_Y);
    
    // Generate initial asteroids
    for (int n = 0; n < numAsteroids; ++n) {
        /*
        float sphere_r = (rand() / (RAND_MAX + 1.0f)) * 0.5f + 1.0f; // range from [1,1.5)
        float sphere_coord_x = (rand() / (RAND_MAX + 1.0f)) * asteroid_range_x * 2 - asteroid_range_x; // range from [-asteroid_range_x, asteroid_range_x)
        float sphere_coord_y = (rand() / (RAND_MAX + 1.0f)) * asteroid_range_y * 2 - asteroid_range_y;
         */
        float sphere_r = asteroid_radii(generator);
        float sphere_coord_x = asteroid_coord_x(generator);
        float sphere_coord_y = asteroid_coord_y(generator);
        
        asteroid_radius[n] = sphere_r;
        asteroids[n] = Node();
        auto sphere_shape = parametric_shapes::createSphere(sphere_r, 100u, 100u);
        asteroids[n].set_geometry(sphere_shape);
        asteroids[n].get_transform().SetTranslate(glm::vec3(sphere_coord_x, sphere_coord_y, 0.0f));
        asteroids[n].set_program(&celestial_body_shader, set_uniforms);
        asteroids[n].add_texture("diffuse_texture", asteroid_texture, GL_TEXTURE_2D);
            }
    
    // Generate inital treasures
    int const numTreasures = 5;
    std::array<Node, numTreasures> treasures;
    std::uniform_real_distribution<float> treasure_coord_x(MIN_X+60.0f, MAX_X-60.0f);
    std::uniform_real_distribution<float> treasure_coord_y(MIN_Y+60.0f, MAX_Y-60.0f);
    for (int n = 0; n < numTreasures; ++n) {
        float treasure_r = 2.0f;
        float sphere_coord_x = treasure_coord_x(generator);
        float sphere_coord_y = treasure_coord_y(generator);
        
        treasures[n] = Node();
        auto sphere_shape = parametric_shapes::createSphere(treasure_r, 100u, 100u);
        treasures[n].set_geometry(sphere_shape);
        treasures[n].get_transform().SetTranslate(glm::vec3(sphere_coord_x, sphere_coord_y, 0.0f));
        treasures[n].get_transform().RotateY(glm::pi<float>()/2.0f);
        treasures[n].set_program(&celestial_body_shader, set_uniforms);
        treasures[n].add_texture("diffuse_texture", rainbow_texture, GL_TEXTURE_2D);
            }
    
    
    // Generate inital quad at NEW_GAME scene
    Node quad;
    auto const shape_quad = parametric_shapes::createQuad(80.f, 40.f, 100u, 100u);
    //auto const shape_quad = parametric_shapes::createQuad3(100u, 100u);
    if (shape_quad.vao == 0u) {
        LogError("Failed to retrieve the mesh for the quad shape");
        return;
    }
    quad.set_geometry(shape_quad);
    quad.get_transform().SetTranslate(glm::vec3(-7.0f,-2.0f,0.0f));
    quad.set_program(&celestial_body_shader, set_uniforms);
    GLuint start_texture = bonobo::loadTexture2D(config::resources_path("textures/start.jpeg"));
    quad.add_texture("diffuse_texture", start_texture, GL_TEXTURE_2D);
    //quad win
    Node quad2;
    //auto const shape_quad = parametric_shapes::createQuad3(100u, 100u);
    quad2.set_geometry(shape_quad);
    quad2.get_transform().SetTranslate(glm::vec3(-7.0f,-2.0f,0.0f));
    quad2.set_program(&celestial_body_shader, set_uniforms);
    GLuint win_texture = bonobo::loadTexture2D(config::resources_path("textures/win.jpeg"));
    quad2.add_texture("diffuse_texture", win_texture, GL_TEXTURE_2D);
    // quad lose
    Node quad3;
    //auto const shape_quad = parametric_shapes::createQuad3(100u, 100u);
    quad3.set_geometry(shape_quad);
    quad3.get_transform().SetTranslate(glm::vec3(-7.0f,-2.0f,0.0f));
    quad3.set_program(&celestial_body_shader, set_uniforms);
    GLuint lose_texture = bonobo::loadTexture2D(config::resources_path("textures/lose.jpeg"));
    quad3.add_texture("diffuse_texture", lose_texture, GL_TEXTURE_2D);



    glClearDepthf(1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);


    auto lastTime = std::chrono::high_resolution_clock::now();

    bool show_logs = true;
    bool show_gui = true;
    bool shader_reload_failed = false;
    bool show_basis = false;
    float basis_thickness_scale = 1.0f;
    float basis_length_scale = 1.0f;
    glm::vec3 vel(0.0f, .0f,.0f);
    float accx = 15.0f, accy = 15.0f, accz=10.0f;
    bool rotated_leftz = false;
    
    // GAME STATE
    enum State {
        NEW_GAME, PLAY_GAME, END_GAME,
    };
    State current_state = NEW_GAME;
    
    // Spaceship life
    int spaceship_life = 3;
    bool anyCollison = false;
    int treasurehunted = 0;
    
    while (!glfwWindowShouldClose(window)) {
        auto const nowTime = std::chrono::high_resolution_clock::now();
               auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
               lastTime = nowTime;

               auto& io = ImGui::GetIO();
               inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

        glfwPollEvents();
        inputHandler.Advance();
        mCamera.Update(deltaTimeUs, inputHandler);

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

        float dt = std::chrono::duration<float>(deltaTimeUs).count();
        //
        // Todo: If you need to handle inputs, you can do it here
        //
        
       
        
        mWindowManager.NewImGuiFrame();

        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        
        switch (current_state) {
            case NEW_GAME:{
                spaceship_life = 3;
                treasurehunted = 0;
                glEnable(GL_DEPTH_TEST);
                mCamera.mWorld.SetTranslate(camera_initial_position);
                spaceship.get_transform().SetTranslate(spaceship_initial_positon);
                quad.get_transform().SetTranslate(glm::vec3(-7.0f,-2.0f,0.0f));
                quad.render(mCamera.GetWorldToClipMatrix());
                
                for (int n = 0; n < numTreasures; ++n) {
                    float treasure_r = 2.0f;
                    float sphere_coord_x = treasure_coord_x(generator);
                    float sphere_coord_y = treasure_coord_y(generator);
                    
                    treasures[n] = Node();
                    auto sphere_shape = parametric_shapes::createSphere(treasure_r, 100u, 100u);
                    treasures[n].set_geometry(sphere_shape);
                    treasures[n].get_transform().SetTranslate(glm::vec3(sphere_coord_x, sphere_coord_y, 0.0f));
                    treasures[n].get_transform().RotateY(glm::pi<float>()/2.0f);
                    treasures[n].set_program(&celestial_body_shader, set_uniforms);
                    treasures[n].add_texture("diffuse_texture", rainbow_texture, GL_TEXTURE_2D);
                }
                if (inputHandler.GetKeycodeState(GLFW_KEY_SPACE) & JUST_PRESSED)
                    current_state = PLAY_GAME;
                break;
            }
                
            case PLAY_GAME:{
                // ===PLAY GAME======================================================================================//
                // SPACESHIP KEYCONTROLLER
                if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED) {
                    vel.x += accx * dt;
                    auto curr_position = spaceship.get_transform().GetTranslation();
                    //auto next_position = curr_position + vel * dt;
                    spaceship.get_transform().Translate(glm::vec3(vel.x*dt, 0.0f, 0.0f));
                    mCamera.mWorld.Translate(glm::vec3(vel.x*dt, 0.0f, 0.0f));
                }
                if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & JUST_PRESSED) {
                    spaceship.get_transform().RotateZ(-glm::pi<float>()/6.0f);
                    spaceship.get_transform().RotateY(-glm::pi<float>()/2.0f);
                }
                if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & JUST_RELEASED) {
                    spaceship.get_transform().SetRotateZ(0.0f);
                    spaceship.get_transform().SetRotateY(0.0f);
                    spaceship.get_transform().RotateY(glm::pi<float>());
                    spaceship.get_transform().RotateX(glm::pi<float>()/4);
                }
                if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED) {
                    vel.x += accx * dt;
                    auto curr_position = spaceship.get_transform().GetTranslation();
                    spaceship.get_transform().Translate(glm::vec3(-vel.x*dt, 0.0f, 0.0f));
                    mCamera.mWorld.Translate(glm::vec3(-vel.x*dt, 0.0f, 0.0f));
                }
                if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & JUST_PRESSED) {
                    spaceship.get_transform().RotateZ(glm::pi<float>()/6.0f);
                    spaceship.get_transform().RotateY(glm::pi<float>()/2.0f);
                }
                if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & JUST_RELEASED) {
                    spaceship.get_transform().SetRotateZ(0.0f);
                    spaceship.get_transform().SetRotateY(0.0f);
                    spaceship.get_transform().RotateY(glm::pi<float>());
                    spaceship.get_transform().RotateX(glm::pi<float>()/4);
                }
                
                if (inputHandler.GetKeycodeState(GLFW_KEY_UP) & PRESSED) {
                    vel.y += accy * dt;
                    auto curr_position = spaceship.get_transform().GetTranslation();
                    spaceship.get_transform().Translate(glm::vec3(0.0f, vel.y*dt, 0.0f));
                    mCamera.mWorld.Translate(glm::vec3(0.0f, vel.y*dt, 0.0f));
                    
                }
                
                if (inputHandler.GetKeycodeState(GLFW_KEY_DOWN) & PRESSED) {
                    vel.y += accy * dt;
                    auto curr_position = spaceship.get_transform().GetTranslation();
                    spaceship.get_transform().Translate(glm::vec3(0.0f, -vel.y*dt, 0.0f));
                    mCamera.mWorld.Translate(glm::vec3(0.0f, -vel.y*dt, 0.0f));
                }
                
                //==TEST==============================================================
                auto camera_position = mCamera.mWorld.GetTranslation();
                sphere.get_transform().SetTranslate(glm::vec3(10.0f,
                                                              10.0f,
                                                              0.0f));
                //bool inArea = edaf80::Assignment5::inSkybox(50.0f, 100.0f, 900.0f);
                //std::cout << inArea << std::endl;
                //====================================================================
                
                // For Infinity
                //auto camera_position = mCamera.mWorld.GetTranslation();
                //skybox.get_transform().SetTranslate(camera_position);
                        
                if (!shader_reload_failed) {
                    //
                    // Todo: Render all your geometry here.
                    //
                    
                    
                    glDisable(GL_DEPTH_TEST);
                    skybox.render(mCamera.GetWorldToClipMatrix());
                    glEnable(GL_DEPTH_TEST);
                    spaceship.render(mCamera.GetWorldToClipMatrix());
                    //sphere.render(mCamera.GetWorldToClipMatrix());
                    for (int i = 0; i < numTreasures; ++i) {
                        treasures[i].render(mCamera.GetWorldToClipMatrix());
                    }
                    

                    
                    // Collision Detector
                    auto spaceship_p = spaceship.get_transform().GetTranslation();
                    asteroid_v.y -= dt*asteroid_acc;
                    anyCollison = false;
                    for (int i = 0; i < numAsteroids; ++i) {
                        //collided = false;
                        float asteroid_r = asteroid_radius[i];
                        //asteroid_v.y -= dt*asteroid_acc;
                        asteroid_v.y = -20.0f;
                        asteroids[i].get_transform().Translate(dt*asteroid_v);
                        //steroids[i].get_transform().RotateZ(dt*glm::half_pi<float>()/10.0f);
                        auto asteroid_p = asteroids[i].get_transform().GetTranslation();
                        // Check visibility and if respawn
                        bool inArea = edaf80::Assignment5::inSkybox(asteroid_p.x, asteroid_p.y, 100.0f);
                        if (inArea){
                            asteroids[i].render(mCamera.GetWorldToClipMatrix());
                            // Check Collision
                            if (edaf80::Assignment5::getCollided(asteroid_p, asteroid_r, spaceship_p, spaceship_radius)) {
                                if (!collided) {
                                    spaceship_life -= 1;
                                    Log(("Life left: " + std::to_string(spaceship_life)).c_str());
                                }
                                collided = true;
                                anyCollison =true;
                            }
                        }else{
                            // Respawn
                            float respawn_r = asteroid_radii(generator);
                            float respawn_coord_x = asteroid_coord_x(generator);
                            float respawn_coord_y = asteroid_coord_y(generator);
                            
                            asteroid_radius[i] = respawn_r;
                            auto respawn_shape = parametric_shapes::createSphere(respawn_r, 100u, 100u);
                            asteroids[i].set_geometry(respawn_shape);
                            asteroids[i].get_transform().SetTranslate(glm::vec3(respawn_coord_x, respawn_coord_y, 0.0f));
                        }
                        
                    }
                    // Treasure hunt
                    for (int i = 0; i < numTreasures; ++i) {
                        auto treasure_p = treasures[i].get_transform().GetTranslation();
                        if (edaf80::Assignment5::getCollided(treasure_p, 2.0f, spaceship_p, spaceship_radius)) {
                            treasurehunted += 1;
//                            auto respawn_shape = parametric_shapes::createSphere(2.0f, 100u, 100u);
//                            treasures[i].set_geometry(respawn_shape);
                            treasures[i].get_transform().SetTranslate(glm::vec3(950.0f, 950.0f, 0.0f));
                            Log(("Treasure hunted: " + std::to_string(treasurehunted)).c_str());
                            }
                    }// Treasure Hunt
            
                
                if (!anyCollison){
                    collided = false;
                }
                if (spaceship_life <= 0 || treasurehunted == numTreasures) {
                    current_state = END_GAME;
                    //break;
                }
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                //
                // Todo: If you want a custom ImGUI window, you can set it up
                //       here
                //
                bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
                if (opened) {
                    ImGui::Checkbox("Show basis", &show_basis);
                    ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
                    ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
                }
                ImGui::End();

                if (show_basis)
                    bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
                if (show_logs)
                    Log::View::Render();
                //mWindowManager.RenderImGuiFrame(show_gui);
                
                }// if shader reloaded
                break;
            }
            case END_GAME:{
                mCamera.mWorld.SetTranslate(camera_initial_position);
                if (treasurehunted == numTreasures){
                    quad2.render(mCamera.GetWorldToClipMatrix());
                }
                if (spaceship_life <= 0){
                    quad3.render(mCamera.GetWorldToClipMatrix());
                }
                if (inputHandler.GetKeycodeState(GLFW_KEY_SPACE) & JUST_PRESSED)
                    current_state = NEW_GAME;
                break;
            }
                
        /*
        // ===PLAY GAME======================================================================================//
        // SPACESHIP KEYCONTROLLER
        if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED) {
            vel.x += accx * dt;
            auto curr_position = spaceship.get_transform().GetTranslation();
            //auto next_position = curr_position + vel * dt;
            spaceship.get_transform().Translate(glm::vec3(vel.x*dt, 0.0f, 0.0f));
            mCamera.mWorld.Translate(glm::vec3(vel.x*dt, 0.0f, 0.0f));
        }
        if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & JUST_PRESSED) {
            spaceship.get_transform().RotateZ(-glm::pi<float>()/6.0f);
            spaceship.get_transform().RotateY(-glm::pi<float>()/2.0f);
        }
        if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & JUST_RELEASED) {
            spaceship.get_transform().SetRotateZ(0.0f);
            spaceship.get_transform().SetRotateY(0.0f);
            spaceship.get_transform().RotateY(glm::pi<float>());
            spaceship.get_transform().RotateX(glm::pi<float>()/4);
        }
        
        if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED) {
            vel.x += accx * dt;
            auto curr_position = spaceship.get_transform().GetTranslation();
            spaceship.get_transform().Translate(glm::vec3(-vel.x*dt, 0.0f, 0.0f));
            mCamera.mWorld.Translate(glm::vec3(-vel.x*dt, 0.0f, 0.0f));
        }
        if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & JUST_PRESSED) {
            spaceship.get_transform().RotateZ(glm::pi<float>()/6.0f);
            spaceship.get_transform().RotateY(glm::pi<float>()/2.0f);
        }
        if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & JUST_RELEASED) {
            spaceship.get_transform().SetRotateZ(0.0f);
            spaceship.get_transform().SetRotateY(0.0f);
            spaceship.get_transform().RotateY(glm::pi<float>());
            spaceship.get_transform().RotateX(glm::pi<float>()/4);
        }
        
        if (inputHandler.GetKeycodeState(GLFW_KEY_UP) & PRESSED) {
            vel.y += accy * dt;
            auto curr_position = spaceship.get_transform().GetTranslation();
            spaceship.get_transform().Translate(glm::vec3(0.0f, vel.y*dt, 0.0f));
            mCamera.mWorld.Translate(glm::vec3(0.0f, vel.y*dt, 0.0f));
            
        }
        
        if (inputHandler.GetKeycodeState(GLFW_KEY_DOWN) & PRESSED) {
            vel.y += accy * dt;
            auto curr_position = spaceship.get_transform().GetTranslation();
            spaceship.get_transform().Translate(glm::vec3(0.0f, -vel.y*dt, 0.0f));
            mCamera.mWorld.Translate(glm::vec3(0.0f, -vel.y*dt, 0.0f));
        }
        
        //==TEST==============================================================
        auto camera_position = mCamera.mWorld.GetTranslation();
        sphere.get_transform().SetTranslate(glm::vec3(10.0f,
                                                      10.0f,
                                                      0.0f));
        //bool inArea = edaf80::Assignment5::inSkybox(50.0f, 100.0f, 900.0f);
        //std::cout << inArea << std::endl;
        //====================================================================
        
        // For Infinity
        //auto camera_position = mCamera.mWorld.GetTranslation();
        //skybox.get_transform().SetTranslate(camera_position);
                
        if (!shader_reload_failed) {
            //
            // Todo: Render all your geometry here.
            //
            
            // switch(current_state){
            //     case NEW_GAME:
            //         quad.render(mCamera.GetWorldToClipMatrix());
            //         current_state = PLAY_GAME;
            //         break;
            //     case PLAY_GAME:
        
            
            
            
            glDisable(GL_DEPTH_TEST);
            skybox.render(mCamera.GetWorldToClipMatrix());
            glEnable(GL_DEPTH_TEST);
            spaceship.render(mCamera.GetWorldToClipMatrix());
            sphere.render(mCamera.GetWorldToClipMatrix());

            
            // Collision Detector
            auto spaceship_p = spaceship.get_transform().GetTranslation();
            asteroid_v.y -= dt*asteroid_acc;
            anyCollison = false;
            for (int i = 0; i < numAsteroids; ++i) {
                //collided = false;
                float asteroid_r = asteroid_radius[i];
                //asteroid_v.y -= dt*asteroid_acc;
                asteroid_v.y = -2.0f;
                asteroids[i].get_transform().Translate(dt*asteroid_v);
                //steroids[i].get_transform().RotateZ(dt*glm::half_pi<float>()/10.0f);
                auto asteroid_p = asteroids[i].get_transform().GetTranslation();
                // Check visibility and if respawn
                bool inArea = edaf80::Assignment5::inSkybox(asteroid_p.x, asteroid_p.y, 50.0f);
                if (inArea){
                    asteroids[i].render(mCamera.GetWorldToClipMatrix());
                    // Check Collision
                    if (edaf80::Assignment5::getCollided(asteroid_p, asteroid_r, spaceship_p, spaceship_radius)) {
                        if (!collided) {
                            spaceship_life -= 1;
                            Log(("Life left: " + std::to_string(spaceship_life)).c_str());
                        }
                        collided = true;
                        anyCollison =true;
                    }
                }else{
                    // Respawn
                    float respawn_r = asteroid_radii(generator);
                    float respawn_coord_x = asteroid_coord_x(generator);
                    float respawn_coord_y = asteroid_coord_y(generator);
                    
                    asteroid_radius[i] = respawn_r;
                    auto respawn_shape = parametric_shapes::createSphere(respawn_r, 100u, 100u);
                    asteroids[i].set_geometry(respawn_shape);
                    asteroids[i].get_transform().SetTranslate(glm::vec3(respawn_coord_x, respawn_coord_y, 0.0f));
                }
                

            }
            if (!anyCollison){
                collided = false;
            }
            if (spaceship_life <= 0) {
                break;
            }
            
            

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        //
        // Todo: If you want a custom ImGUI window, you can set it up
        //       here
        //
        bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
        if (opened) {
            ImGui::Checkbox("Show basis", &show_basis);
            ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
            ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
        }
        ImGui::End();

        if (show_basis)
            bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
        if (show_logs)
            Log::View::Render();
            
         mWindowManager.RenderImGuiFrame(show_gui);
         // ===PLAY GAME END======================================================================================//
         */
        
            
    }//Switch
        mWindowManager.RenderImGuiFrame(show_gui);
        glfwSwapBuffers(window);
  }//While window
}

int main()
{
    std::setlocale(LC_ALL, "");

    Bonobo framework;

    try {
        edaf80::Assignment5 assignment5(framework.GetWindowManager());
        assignment5.run();
    } catch (std::runtime_error const& e) {
        LogError(e.what());
    }
}

/*========================================================================================================*/



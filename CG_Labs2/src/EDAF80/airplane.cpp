//
//  airplane.cpp
//  EDAF80_Assignment5
//
//  Created by QINXIN SHU on 2022/10/18.
//

#include "airplane.hpp"
#include "../core/helpers.hpp"
//#include "core/Misc.h"
#include <math.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Airplane::Airplane(GLuint* shader, glm::vec3& light_position, glm::vec3& camera_position){
    std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("airplane.obj");
    if (objects.empty()) {
        printf("Failed to load the sphere geometry: exiting.\n");
    }
    auto airplane_texture = bonobo::loadTexture2D("airplane.png");
    bonobo::mesh_data const& airplane = objects.front();
    baseNode.set_geometry(airplane);
    baseNode.set_program(shader,  [light_position,camera_position, this](GLuint program) {
        glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
        glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
        glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(this->ambient));
        glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(this->specular));
        glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
    
    });
    baseNode.add_texture("diffuse_texture", airplane_texture, GL_TEXTURE_2D);
    //baseNode.get_transform().RotateY(glm::pi<float>());
    baseNode.get_transform().SetTranslate(glm::vec3(0.0,150.0,0.0));
    for(size_t i = 1 ; i < objects.size(); i++){
        printf("%d\n", i);
    }
    
    printf("INIT\n");

}
void Airplane::render(glm::mat4 worldToClipMatrix){
    baseNode.render(worldToClipMatrix);
}
glm::vec3 Airplane::get_position() {
    return baseNode.get_transform().GetTranslation();
}
glm::vec3 Airplane::get_direction() {
    return direction;
}
void Airplane::update(InputHandler inputHandler, float delta) {
    glm::vec3 force_lift   = glm::vec3(0.0,0.0,0.0);
    glm::vec3 force_thrust =  glm::vec3(0.0,0.0,0.0);
    baseNode.get_transform().LookAt(this->get_position() + direction, glm::vec3(0.0, 1.0, 0.0));

     
    if (inputHandler.GetKeycodeState(GLFW_KEY_SPACE) & PRESSED) {
        if (abs(velocity) < 1) {
            velocity += 0.005;
        }
    }
    else {
        velocity -= 0.0002;
        if (velocity < 0) {
            velocity = 0;
        }
    }
    if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED) {
        angleX -= 0.005;
        if (angleX < -0.5) {
            angleX = -0.5;
        }
        
    }
    if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED) {
        angleX += 0.005;
        if (angleX > 0.5) {
            angleX = 0.5;
        }
    }
    angleY = 0;
    bool angleZchanged = false;
    if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED) {
         angleZ -= 0.01;
         if(angleZ < -0.5){
             angleZ = -0.5;
         }
         angleZchanged = true;
    }
    if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED) {
         angleZ += 0.01;
         if(angleZ > 0.5){
             angleZ = 0.5;
         }
         angleZchanged = true;
         
    }
    if(!angleZchanged){
        if(angleZ < 0){
            angleZ += 0.01;
            if(angleZ > 0){
                angleZ = 0;
            }
        }else if(angleZ > 0){
            angleZ -= 0.01;
            if(angleZ < 0){
                angleZ = 0;
            }
        }
    }
    force_thrust.x = direction.x;
    force_thrust.z = direction.z;
    
    force_thrust.y = glm::sin(glm::pi<double>() * angleX);
    force_lift.x = -glm::sin(glm::pi<double>()*(angleZ / 75.0f));
     
     //force_thrust = baseNode.get_transform().GetRotation() * force_thrust;
     force_lift = baseNode.get_transform().GetRotation() * force_lift;
    direction = force_thrust + force_lift;
    direction = normalize(direction);
    baseNode.get_transform().LookAt(this->get_position() + direction, glm::vec3(0.0, 1.0, 0.0));

    baseNode.get_transform().Rotate(glm::pi<double>()*angleZ, glm::vec3(0.0,0.0,1.0));

    baseNode.get_transform().Translate(direction*velocity*60.0f*delta);
}

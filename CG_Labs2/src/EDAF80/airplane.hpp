//
//  airplane.hpp
//  EDAF80_Assignment5
//
//  Created by QINXIN SHU on 2022/10/18.
//

#pragma once
#include "../core/node.hpp"
#include <glm/glm.hpp>
#include "core/InputHandler.h"

class Airplane{
public:
    Airplane(GLuint* shader, glm::vec3&, glm::vec3&);
    ~Airplane() = default;
    void render(glm::mat4);
    glm::vec3 get_position();
    glm::vec3 get_direction();
    void update(InputHandler, float );
    //void set_position();
private:
    Node baseNode;
    float velocity = 0;
    glm::vec3 direction = glm::vec3(0.0,0.0,-1.0);
    float angleX = 0;
    float angleY = 0;
    float angleZ = 0;
    glm::vec3 ambient = glm::vec3(0.0,0.0,0.0);
    glm::vec3 specular = glm::vec3(0.3,0.3,0.3);
    float shininess = 1.0f;


};


#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
    //! \todo Implement this function
    glm::vec2 x_vec = glm::vec2(1.0f, x);
    glm::mat2 B = glm::mat2(1.0f, 0.0f, -1.0f, 1.0f);
    glm::mat2x3 p = glm::mat2x3(p0, p1);
    glm::vec3 res = (p * B) * x_vec;
    return res;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
    //! \todo Implement this function
    glm::vec4 x_vec = glm::vec4(1.0f,x, x*x, x*x*x);
    glm::mat4 B = glm::mat4(0.0f, 1.0f, 0.0f, 0.0f, -t, 0, t, 0, 2*t, t-3, 3-2*t, -t, -t, 2-t, t-2,t);
    glm::mat4x3 p = glm::mat4x3(p0, p1, p2, p3);
    glm::vec3 res = (p * B) * x_vec;
    return res;
}

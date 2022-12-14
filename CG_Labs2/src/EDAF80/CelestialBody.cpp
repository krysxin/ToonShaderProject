#include "CelestialBody.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include "core/helpers.hpp"
#include "core/Log.h"

CelestialBody::CelestialBody(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id)
{
    _body.node.set_geometry(shape);
    _body.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
    _body.node.set_program(program);
}

glm::mat4 CelestialBody::render(std::chrono::microseconds elapsed_time,
                                glm::mat4 const& view_projection,
                                glm::mat4 const& parent_transform,
                                bool show_basis)
{
    // Convert the duration from microseconds to seconds.
    auto const elapsed_time_s = std::chrono::duration<float>(elapsed_time).count();
    // If a different ratio was needed, for example a duration in
    // milliseconds, the following would have been used:
    // auto const elapsed_time_ms = std::chrono::duration<float, std::milli>(elapsed_time).count();

    //_body.spin.rotation_angle = -glm::half_pi<float>() / 2.0f;

    
    glm::mat4 I = glm::mat4(1.0f);
    glm::mat4 world = parent_transform;
    //exercise1
    glm::mat4 s = glm::scale(I, _body.scale);
    
    //exercise2
    _body.spin.rotation_angle += _body.spin.speed * elapsed_time_s;
    std::cout << _body.spin.rotation_angle << std::endl;
    glm::mat4 R1s =  glm::rotate(I, _body.spin.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 R2s =  glm::rotate(I, _body.spin.axial_tilt, glm::vec3(0.0f, 0.0f, 1.0f));
    
    //exercise3
    //_body.orbit.speed += elapsed_time_s;
    _body.orbit.rotation_angle +=  _body.orbit.speed * elapsed_time_s;
    glm::mat4 T0 = glm::translate(I, glm::vec3(_body.orbit.radius, 0.0f, 0.0f));
    glm::mat4 R10 =  glm::rotate(I, _body.orbit.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 R20 =  glm::rotate(I, _body.orbit.inclination, glm::vec3(0.0f, 0.0f, 1.0f));
    
    world = parent_transform * R20 * R10 * T0 * R2s * R1s * s;
    
    //exercise4
    //world = parent_transform * R20 * R10 * R2s * R1s; //remove *s (scaling) wrong!
    glm::mat4 modified_transform = parent_transform * R20 * R10 * T0;
    
    // exercise7
    // Rotate rings by 90 degree
    glm::mat4 Rr = glm::rotate(I, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    // Scale rings
    glm::mat4 sr = glm::scale(I, glm::vec3(_ring.scale.x, _ring.scale.y, 0.0f));
    
    glm::mat4 ring_world = parent_transform * R20 * R10 * T0 * R2s * Rr * sr;
    
    
    
    if (_ring.is_set == true)
    {
        _ring.node.render(view_projection, ring_world);
    }
    

    if (show_basis)
    {
        bonobo::renderBasis(1.0f, 2.0f, view_projection, world);
    }

    // Note: The second argument of `node::render()` is supposed to be the
    // parent transform of the node, not the whole world matrix, as the
    // node internally manages its local transforms. However in our case we
    // manage all the local transforms ourselves, so the internal transform
    // of the node is just the identity matrix and we can forward the whole
    // world matrix.
    _body.node.render(view_projection, world);
    

    return modified_transform;
}

void CelestialBody::add_child(CelestialBody* child)
{
    _children.push_back(child);
}

std::vector<CelestialBody*> const& CelestialBody::get_children() const
{
    return _children;
}

void CelestialBody::set_orbit(OrbitConfiguration const& configuration)
{
    _body.orbit.radius = configuration.radius;
    _body.orbit.inclination = configuration.inclination;
    _body.orbit.speed = configuration.speed;
    _body.orbit.rotation_angle = 0.0f;
}

void CelestialBody::set_scale(glm::vec3 const& scale)
{
    _body.scale = scale;
}

void CelestialBody::set_spin(SpinConfiguration const& configuration)
{
    _body.spin.axial_tilt = configuration.axial_tilt;
    _body.spin.speed = configuration.speed;
    _body.spin.rotation_angle = 0.0f;
}

void CelestialBody::set_ring(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id,
                             glm::vec2 const& scale)
{
    _ring.node.set_geometry(shape);
    _ring.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
    _ring.node.set_program(program);

    _ring.scale = scale;

    _ring.is_set = true;
}




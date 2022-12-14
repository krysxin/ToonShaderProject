#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 light_position;
uniform vec3 camera_position;


out VS_OUT {
	vec3 world_normal;
	vec3 world_camera;
    vec3 world_light;
    vec2 texcoord;
    vec3 T;
    vec3 B;
    vec3 N;
} vs_out;


void main()
{
    vec3 world_vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
    vs_out.world_normal = vec3(normal_model_to_world * vec4(normal, 0.0));
    vs_out.world_camera = camera_position - world_vertex;
    vs_out.world_light = light_position - world_vertex;
    vs_out.texcoord = texcoord.xy;
    // Prepare TBN matrix for normal mapping
    vs_out.T = normalize(tangent);
    vs_out.B = normalize(binormal);
    vs_out.N = normalize(normal);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}




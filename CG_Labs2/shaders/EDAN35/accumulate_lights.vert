#version 410



struct ViewProjTransforms
{
	mat4 view_projection;
	mat4 view_projection_inverse;
};

layout (std140) uniform CameraViewProjTransforms
{
	ViewProjTransforms camera;
};

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;

layout (location = 0) in vec3 vertex;


out VS_OUT {
	vec3 vertex;

} vs_out;

void main() {
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));

	gl_Position = camera.view_projection * vertex_model_to_world * vec4(vertex, 1.0);
}

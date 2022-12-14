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
 vec3 fN; 
 vec3 fV;
 vec3 fL;
 vec2 texcoord;
 mat3 TBN;
}
vs_out;



void main()
{

 vec3 T = normalize(tangent);
 vec3 B = normalize(binormal);
 vec3 N = normalize(normal);
 vs_out.TBN = mat3(T,B,N);


 vec3 worldPos = (vertex_model_to_world*vec4(vertex,1)).xyz;
 vs_out.fN = (normal_model_to_world*vec4(normal,0.0)).xyz;
 vs_out.texcoord = texcoord.xy;
 vs_out.fV = camera_position - worldPos;
 vs_out.fL = light_position - worldPos;
 gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}


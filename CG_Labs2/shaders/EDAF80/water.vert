#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 camera_position;
uniform float time;

out VS_OUT {
	vec3 vertex;
    vec2 texcoord;
	vec3 world_normal;
    vec3 world_camera;

    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2; 

    vec3 tangent;
    vec3 binormal;
} vs_out;

float wave(in vec2 position, in vec2 direction, in float amplitude, in float frequency, in float phase, in float sharpness, in float time)
{
    return amplitude * pow(sin((position.x * direction.x + position.y * direction.y) * frequency + phase * time) * 0.5 + 0.5, sharpness);
}

float wave_dx(in vec2 position, in vec2 direction, in float amplitude, in float frequency,
in float phase, in float sharpness, in float time)
{
    float sinangle = pow(sin((position.x * direction.x + position.y * direction.y) * frequency + phase * time) * 0.5 + 0.5, sharpness-1);
    float cosangle = cos((position.x * direction.x + position.y * direction.y)*frequency + phase * time);
    return 0.5 * sharpness * frequency * amplitude * sinangle * cosangle * direction.x;
}

float wave_dz(in vec2 position, in vec2 direction, in float amplitude, in float frequency, in float phase, in float sharpness, in float time)
{
    float sinangle = pow(sin((position.x * direction.x + position.y * direction.y) * frequency + phase * time) * 0.5 + 0.5, sharpness-1);
    float cosangle = cos((position.x * direction.x + position.y * direction.y)*frequency + phase * time);
    return 0.5 * sharpness * frequency * amplitude * sinangle * cosangle * direction.y;
}


void main()
{
	vec3 displaced_vertex = vertex;
    float wave1 = wave(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, time);
    float wave2 = wave(vertex.xz, vec2(-0.7, 0.7), 0.5, 0.4, 1.3, 2.0, time);
    displaced_vertex.y += wave1;
    displaced_vertex.y+= wave2;

    float wavedx1 = wave_dx(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, time);
    float wavedx2 = wave_dx(vertex.xz, vec2(-0.7, 0.7), 0.5, 0.4, 1.3, 2.0, time);
    float hdx = wavedx1 + wavedx2; 

    float wavedz1 = wave_dz(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, time);
    float wavedz2 = wave_dz(vertex.xz, vec2(-0.7, 0.7), 0.5, 0.4, 1.3, 2.0, time);
    float hdz = wavedz1 + wavedz2; 
    

    vec3 world_displaced_vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
    vs_out.vertex = world_displaced_vertex;

    vec3 n = normalize(vec3(-hdx, 1.0, -hdz));
	vs_out.world_normal = vec3(normal_model_to_world * vec4(n, 0.0));
    vs_out.world_camera = camera_position - world_displaced_vertex;

    // Calculate the normalCoord
    vec2 texScale = vec2(8, 4);
    float normalTime = mod(time, 100.0);
    vec2 normalSpeed = vec2(-0.05, 0.0);
    vs_out.normalCoord0 = texcoord.xy * texScale + normalTime * normalSpeed;
    vs_out.normalCoord1 = texcoord.xy * texScale * 2 + normalTime * normalSpeed * 4;
    vs_out.normalCoord2 = texcoord.xy * texScale * 4 + normalTime * normalSpeed * 8;
    // Calculate tangent and binormal
    vec3 t = normalize(vec3(1, -hdx, 0));
    vec3 b = normalize(vec3(0, -hdz, 1));
    vs_out.tangent = vec3(vertex_model_to_world * vec4(t, 0.0));
    vs_out.binormal = vec3(vertex_model_to_world * vec4(b, 0.0));

    vs_out.texcoord = texcoord.xy;
    
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced_vertex, 1.0);
}




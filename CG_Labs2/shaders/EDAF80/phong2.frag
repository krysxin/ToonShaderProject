#version 410

out vec4 frag_color;

in VS_OUT {
 vec3 fN;
 vec3 fV;
 vec3 fL; 
 vec2 texcoord;
 mat3 TBN;
}
fs_in;

uniform float shininess_value;
uniform sampler2D spaceship_texture;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform int use_normal_mapping;
uniform mat4 normal_model_to_world;
uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform int collided;
uniform sampler2D spaceship_collide_texture;

void main()
{

vec3 N = normalize(fs_in.fN);

if(use_normal_mapping==1){

N = texture(normal_map,fs_in.texcoord).rgb;
N = N * 2.0 - 1.0;
N = normalize(normal_model_to_world*vec4(fs_in.TBN * N,0.0)).xyz;


}


vec3 L = normalize(fs_in.fL);
vec3 V = normalize(fs_in.fV);
vec3 R = normalize(reflect(-L,N));
vec3 d = diffuse_colour * texture(spaceship_texture,fs_in.texcoord).rgb*max(dot(N,L),0.0);
if (collided == 1) {
    d = diffuse_colour * texture(spaceship_collide_texture,fs_in.texcoord).rgb*max(dot(N,L),0.0);
} else {
    d = diffuse_colour * texture(spaceship_texture,fs_in.texcoord).rgb*max(dot(N,L),0.0);
}
vec3 s = specular_colour * texture(specular_map,fs_in.texcoord).rgb*pow(max(dot(R,V),0.0), shininess_value);
frag_color.xyz = ambient_colour + d + s;
frag_color.w = 1.0;


}

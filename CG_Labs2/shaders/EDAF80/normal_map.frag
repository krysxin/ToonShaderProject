#version 410

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
uniform mat4 normal_model_to_world;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normalmap_texture;

uniform int use_normal_mapping;

in VS_OUT {
	vec3 world_camera;
    vec3 world_light;
    vec3 T;
    vec3 B;
    vec3 N;
    vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main() {
    mat3 TBN = mat3(fs_in.T, fs_in.B, fs_in.N);
    vec3 normal = texture(normalmap_texture, fs_in.texcoord).rgb;
    normal = normal * 2.0 - 1.0;
    normal = (normal_model_to_world * vec4((TBN * normal), 0.0)).xyz;
    normal = normalize(normal);

    vec3 L = normalize(fs_in.world_light);
    vec3 V = normalize(fs_in.world_camera);
    vec3 R = normalize(reflect(-L, normal));

    vec3 diffuse_color = texture(diffuse_texture, fs_in.texcoord).rgb;
    vec3 specular_color = texture(specular_texture, fs_in.texcoord).rgb;

    vec3 diffuse = diffuse_color * max(dot(normal,L), 0.0);
    vec3 specular = specular_color * pow(max(dot(R,V), 0.0), shininess);

    frag_color.xyz = ambient + diffuse + specular;
    frag_color.w = 1.0;

}







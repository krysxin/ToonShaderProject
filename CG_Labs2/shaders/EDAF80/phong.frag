#version 410

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;

in VS_OUT {
	vec3 world_normal;
	vec3 world_camera;
    vec3 world_light;
    vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 N = normalize(fs_in.world_normal);
    vec3 L = normalize(fs_in.world_light);
    vec3 V = normalize(fs_in.world_camera);
    vec3 R = normalize(reflect(-L, N));
    
    // Use default color
    // vec3 diffuse = diffuse * max(dot(N,L), 0.0);
    // vec3 specular = specular * pow(max(dot(R,V), 0.0), shininess);

    // Use texture
    vec3 diffuse_color = texture(diffuse_texture, fs_in.texcoord).rgb;
    vec3 specular_color = texture(specular_texture, fs_in.texcoord).rgb;
    vec3 diffuse = diffuse_color * max(dot(N,L), 0.0);
    vec3 specular = specular_color * pow(max(dot(R,V), 0.0), shininess);
    frag_color.xyz = ambient + diffuse + specular;
    frag_color.w = 1.0;

}

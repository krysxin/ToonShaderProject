#version 410

uniform int use_normal_mapping;
uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 diffuse;
uniform vec3 specular;
uniform vec3 ambient;
uniform float shininess;

//uniform samplerCube skybox_texture;
uniform sampler2D diffuse_texture;
uniform sampler2D normalmap_texture;
uniform sampler2D specular_texture;

uniform int has_diffuse_texture;
uniform int use_diffuse_texture;

float saturate(float f){
	return clamp(f, 0.0, 1.0);
}

in VS_OUT {
	vec3 world_normal;
	vec3 world_camera;
    vec3 world_light;
    vec3 world_vertex;
    vec2 texcoord;
    vec3 T;
    vec3 B;
    vec3 N;
} fs_in;

out vec4 frag_color;

void main() {
	vec3 normal = normalize(fs_in.world_normal);

	if(use_normal_mapping != 0){
		vec3 normalColor = texture(normalmap_texture, fs_in.texcoord).rgb * 2.0 - 1.0;
		mat3 TBN = mat3(normalize(fs_in.T),
										normalize(fs_in.B),
										normalize(fs_in.N));
		normal = normalize(TBN * normalColor);
	}

	vec3 L = normalize(fs_in.world_light);
	vec3 V = normalize(fs_in.world_camera);
    vec3 R = normalize(reflect(-L, normal));

	//float shininess = 1.0 / (texture(roughness_texture, fs_in.texcoord).r / shininess_value);

	float diffuse_f =  max(dot(normal, L), 0.0);

	if(diffuse_f >= 0.8)
	{
		diffuse_f = 1.0f;
	}
	else if(diffuse_f >= 0.6)
	{
		diffuse_f = 0.6f;
	}
	else if(diffuse_f >= 0.2)
	{
		diffuse_f = 0.3;
	}
	else
	{
		diffuse_f = 0.0f;
	}

	float specular_f =  pow(max(dot(R,V), 0.0), shininess);
	float specularintensity = step(0.98, specular_f);

	vec3 rimLight = vec3(0.0f,0.0f,0.0f);
    rimLight = vec3(1.0 - max ( 0.0 , dot(normalize(-fs_in.world_vertex), normal)));
	rimLight =(pow(rimLight, vec3(2.0)) * 1.2);
	rimLight = smoothstep(0.3,0.4,rimLight);
    rimLight *= diffuse;

    vec3 diffuseColor = texture(diffuse_texture, fs_in.texcoord).rgb;
    
	//vec3 skyboxColor = textureLod(skybox_texture, reflect(-V, n), mix(0.0, 9.0, saturate(1.0 - shininess/50.0))).rgb;
    
     if (has_diffuse_texture==1 && use_diffuse_texture==1){
        frag_color = vec4(diffuseColor, 1.0) * vec4(
			ambient +
			diffuse * diffuse_f +
			specular * specularintensity +
			rimLight,
			1.0);
     }else{
        frag_color = vec4(
			ambient +
			diffuse * diffuse_f +
			specular * specularintensity + rimLight,
			1.0);
     }
	
}
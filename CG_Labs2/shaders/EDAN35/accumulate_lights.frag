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

layout (std140) uniform LightViewProjTransforms
{
	ViewProjTransforms lights[4];
};

uniform int light_index;

uniform sampler2D depth_texture;
uniform sampler2D normal_texture;
uniform sampler2D shadow_texture;

uniform vec2 inverse_screen_resolution;

uniform vec3 camera_position;

uniform vec3 light_color;
uniform vec3 light_position;
uniform vec3 light_direction;
uniform float light_intensity;
uniform float light_angle_falloff;

float saturate(float f){
	return clamp(f, 0.0, 1.0);
}

in VS_OUT {
	vec3 vertex;

} fs_in;

layout (location = 0) out vec4 light_diffuse_contribution;
layout (location = 1) out vec4 light_specular_contribution;


void main()
{
	vec2 shadowmap_texel_size = 1.0f / textureSize(shadow_texture, 0);

	vec2 texcoord = gl_FragCoord.xy * inverse_screen_resolution;
	vec4 normal = texture(normal_texture, texcoord)*2.0f - 1.0f;
	vec4 depth = texture(depth_texture, texcoord);
	vec3 loc_pos = vec3(texcoord.x, texcoord.y, depth.x) * 2.0f - 1.0f;
	vec4 world_pos = camera.view_projection_inverse * vec4(loc_pos, 1.0f);
	vec3 pos = world_pos.xyz / world_pos.w;

	vec3 L = normalize(light_position - pos.xyz);
	vec3 V = normalize(camera_position - pos.xyz);
	vec3 R = normalize(reflect(-L, normalize(normal).xyz)); 

	// Light falloff calculation
	float d = distance(pos.xyz, light_position);
	float angle = acos(clamp(dot(-L, light_direction), 0.0f, 1.0f));
	float d_falloff = 1 / (d * d);
	float angular_falloff = max(0.0f, (-1 - light_angle_falloff) * angle + 1.0f);
	float light_intensity_falloffed = light_intensity * d_falloff * angular_falloff;

		// Check shadow using shadow map
	vec4 shadow_pos = lights[light_index].view_projection * vec4(pos, 1.0f);
	shadow_pos = shadow_pos / shadow_pos.w;
	shadow_pos = shadow_pos * 0.5f + 0.5f;
	shadow_pos.z = shadow_pos.z - 0.000001f; // If w/o epsilon


	// Shadow w/ Percentage closer rendering (PCR)
	float shadow_depth = 0.0f;
	for(int x = -2; x <= 2; ++x){
		for(int y = -2; y <= 2; ++y){
			float pcfDepth = texture(shadow_texture, shadow_pos.xy + vec2(x, y) * shadowmap_texel_size).x; 
			shadow_depth += shadow_pos.z > pcfDepth ? 1.0f : 0.0f;        
		}    
	}
	shadow_depth /= 25.0f;
	float shadow = 1.0f - shadow_depth;

	// Shadow w/o PCR-filter
	//float shadow_depth = texture(shadow_texture, shadow_pos.xy).x;
	//float isShadow = shadow_pos.z > shadow_depth ? 1.0f : 0.0f;
	float diffuse =  saturate(dot(normalize(normal).xyz, L) * 0.75 + 0.25);
	if(diffuse >= 0.8)
	{
		diffuse = 1.0f;
	}
	else if(diffuse >= 0.6)
	{
		diffuse = 0.6f;
	}
	else if(diffuse >= 0.2)
	{
		diffuse = 0.3;
	}
	else
	{
		diffuse = 0.0f;
	}

	float specular =  pow(saturate(dot(V, R)), 50.0f);
	float specularintensity = step(0.98, specular);


	vec3 rimLight = vec3(0.0f,0.0f,0.0f);
    rimLight = vec3(1.0 - max ( 0.0 , dot(normalize(-fs_in.vertex), normal.xyz)));
	rimLight =(pow(rimLight, vec3(2.0)) * 1.2);
	rimLight = smoothstep(0.3,0.4,rimLight);
    rimLight *= light_color;


	// Light color calculation
    light_diffuse_contribution =  shadow * light_intensity_falloffed * vec4(light_color * diffuse, 1.0f) + vec4(rimLight,1.0f);
	light_specular_contribution =  shadow * light_intensity_falloffed * vec4(light_color * specularintensity, 1.0f);


	// light_diffuse_contribution  = vec4(0.0, 0.0, 0.0, 1.0);
	// light_specular_contribution = vec4(0.0, 0.0, 0.0, 1.0);

}

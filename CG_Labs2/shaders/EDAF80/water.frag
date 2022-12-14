#version 410

uniform vec3 light_position;
uniform float time;

uniform mat4 normal_model_to_world;

uniform samplerCube skybox_texture;
uniform sampler2D water_texture;

in VS_OUT {
	vec3 vertex;
	vec3 world_normal;
    vec3 world_camera;
    vec2 texcoord;

    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2; 

    vec3 tangent;
    vec3 binormal;
} fs_in;

out vec4 frag_color;



void main()
{
	//vec3 L = normalize(light_position - fs_in.vertex);
    vec3 V = normalize(fs_in.world_camera);
    vec3 N = normalize(fs_in.world_normal);
    vec3 T = normalize(fs_in.tangent);
    vec3 B = normalize(fs_in.binormal);
    mat3 BTN = mat3(B, T, N);
    mat3 TBN = mat3(T, B, N);

    // Animated normal mapping
    vec4 n0 = texture(water_texture, fs_in.normalCoord0) * 2.0 - 1.0;
    vec4 n1 = texture(water_texture, fs_in.normalCoord1) * 2.0 - 1.0;
    vec4 n2 = texture(water_texture, fs_in.normalCoord2) * 2.0 - 1.0;
    vec4 nbump_tg = n0+n1+n2;
    vec3 nbump = normalize(TBN * nbump_tg.xyz);
    //vec3 world_nbump = (normal_model_to_world * vec4(nbump, 0.0)).xyz;


    vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
    vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);

    float facing = 1 - max(dot(V, nbump), 0.0);
    vec3 R = normalize(reflect(-V, nbump));

    // Fresnel
    float e0 = 1.0;
    float e1 = 1.33;
    float R0 = 0.02037;
    float fresnel = R0 + (1 - R0) * pow((1 - dot(V,nbump)), 5);
    vec3 refrac = refract(-V, nbump, e0/e1);



    // Base color water
    vec4 color_water = mix(color_deep, color_shallow, facing);

    // Reflection color
    vec4 reflection_color = texture(skybox_texture, R);
    // Refraction
    vec4 refraction_color = texture(skybox_texture, refrac);

    

	frag_color = color_water + reflection_color * fresnel + refraction_color * (1-fresnel);
    //frag_color = reflection_color * fresnel;
    //frag_color = vec4(fs_in.texcoord.x,fs_in.texcoord.y, 0.0, 0.0);
    //frag_color = texture(normalmap_texture, fs_in.normalCoord0);
    //frag_color = vec4(nbump, 0.0);
}
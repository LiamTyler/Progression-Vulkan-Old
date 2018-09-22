#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) writeonly uniform image2D img_output;

layout(rgba32f, binding = 1) readonly uniform image2D gPosition;
layout(rgba32f, binding = 2) readonly uniform image2D gNormal;
layout(rgba8, binding = 3) readonly uniform image2D gDiffuse;
layout(rgba32f, binding = 4) readonly uniform image2D gSpecularExp;

uniform int numPointLights;

layout(std430, binding=5) buffer point_light_list
{
    vec4 lights[];
};

void main() {
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    
    vec3 position     = imageLoad(gPosition, coords).rgb;
    vec3 n            = imageLoad(gNormal, coords).xyz;
    vec3 diffuseColor = imageLoad(gDiffuse, coords).rgb;
    vec4 specExp      = imageLoad(gSpecularExp, coords);
    
    vec3 e = normalize(-position);

    vec3 color = vec3(0);
    for (int i = 0; i < numPointLights; ++i) {
        vec3 lightPos = lights[2 * i + 0].xyz;
        vec3 lightColor = lights[2 * i + 1].rgb;
        
        vec3 l = normalize(lightPos - position);
        vec3 h = normalize(l + e);
        float attenuation = 1.0 / pow(length(lightPos - position), 2.0);
              
        // Blinn-phong
        // color += lightColor * ka;
        color += attenuation * lightColor * diffuseColor * max(0.0, dot(l, n));
        color += attenuation * lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), specExp.a);
    }
    
    imageStore(img_output, coords, vec4(color, 1));
}
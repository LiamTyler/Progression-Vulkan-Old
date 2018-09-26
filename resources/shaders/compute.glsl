#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) writeonly uniform image2D img_output;

layout(rgba32f, binding = 1) readonly uniform image2D gPosition;
layout(rgba32f, binding = 2) readonly uniform image2D gNormal;
layout(rgba8, binding = 3) readonly uniform image2D gDiffuse;
layout(rgba32f, binding = 4) readonly uniform image2D gSpecularExp;

uniform int numPointLights;

uniform vec3 NDX;
uniform vec3 NDY;
uniform vec3 FDX;
uniform vec3 FDY;

uniform vec3 NBL;
uniform vec3 FBL;


layout(std430, binding=5) buffer point_light_list
{
    vec4 lights[];
};

#define MAX_LIGHTS_PER_FRUSTUM 1
// shared vec4 lightList[2 * MAX_LIGHTS_PER_FRUSTUM];
shared int numLightsInTile;

vec4 calculatePlane(in vec3 p1, in vec3 p2, in vec3 p3) {
    vec3 p12 = p2 - p1;
    vec3 p13 = p3 - p1;
    
    vec3 n = normalize(cross(p12, p13));
    float d = dot(n, -p1);
    return vec4(n, d);
}

bool sameSide(in vec3 point, in vec4 plane) {
    return dot(vec4(point, 1), plane) >= 0;
}

vec3 get_max_corner(in vec4 plane, in vec3 point, in float radius) {
    vec3 P = point - vec3(radius);
    vec3 max = point + vec3(radius);
    if (plane.x >= 0)
        P.x = max.x;
    if (plane.y >= 0)
        P.y = max.y;
    if (plane.z >= 0)
        P.z = max.z;
    
    return P;
}

vec3 get_min_corner(in vec4 plane, in vec3 point, in float radius) {
    vec3 P = point + vec3(radius);
    vec3 min = point - vec3(radius);
    if (plane.x >= 0)
        P.x = min.x;
    if (plane.y >= 0)
        P.y = min.y;
    if (plane.z >= 0)
        P.z = min.z;
    
    return P;
}
int lightInFrustum(in vec4 planes[6], in vec4 light) {
    int count = 0;
    for (int i = 0; i < 6; ++i) {
        vec3 P = get_max_corner(planes[i], light.xyz, light.w);
        if (!sameSide(P, planes[i]))
            return count;
        count++;
    }
    return count;
}

void sync() {
    memoryBarrierShared();
    barrier();
}

void main() {
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    if (gl_LocalInvocationID.x == 0)
        numLightsInTile = 0;
    sync();
    
    vec4 planes[6];
    uvec2 groupID = gl_WorkGroupID.xy;
    
    // near points
    vec3 groupNBL = NBL + groupID.x * NDX + groupID.y * NDY;
    vec3 groupNBR = groupNBL + NDX;
    vec3 groupNTL = groupNBL + NDY;
    vec3 groupNTR = groupNTL + NDX;
    // far points
    vec3 groupFBL = FBL + groupID.x * FDX + groupID.y * FDY;
    vec3 groupFBR = groupFBL + FDX;
    vec3 groupFTL = groupFBL + FDY;
    vec3 groupFTR = groupFTL + FDX;
    
    planes[0] = calculatePlane(groupNTL, groupNTR, groupNBR); // near plane
    planes[1] = calculatePlane(groupFTR, groupFTL, groupFBL); // far plane
    planes[2] = calculatePlane(groupNTL, groupNBL, groupFBL); // left plane
    planes[3] = calculatePlane(groupNBR, groupNTR, groupFBR); // right plane
    planes[4] = calculatePlane(groupNTR, groupNTL, groupFTL); // top plane
    planes[5] = calculatePlane(groupNBL, groupNBR, groupFBR); // bottom plane
    
    int lightsPerThread = int(ceil(numPointLights / 256.0));
    
    /*
    for (int i = 0; i < lightsPerThread; i++) {
        // uint lightIdx = (gl_LocalInvocationIndex * lightsPerThread + i);
        uint lightIdx = gl_LocalInvocationIndex;
        if (lightIdx < numPointLights) {
            vec4 light = lights[2 * lightIdx];
            if (lightInFrustum(planes, light)) {
                atomicAdd(numLightsInTile, 1);
            }
        }
    }
    */
    int numLights = 0;
    int val = 0;
    for (int i = 0; i < numPointLights; i++) {
        vec4 light = lights[2 * i];
        val = lightInFrustum(planes, light);
    }
    
    sync();
    
    //float workID = groupID.y + float(groupID.x);
    //float localID = gl_LocalInvocationID.y + gl_LocalInvocationID.x;
    // imageStore(img_output, coords, vec4(vec3(workID / (45.0 + 80.0)), 1));
    // imageStore(img_output, coords, vec4(vec3(localID / 32.0), 1));
    // imageStore(img_output, coords, vec4(vec3(lightIndex / (16*16)), 1));
    vec4 colors[7];
    colors[0] = vec4(0, 0, 0, 1);
    colors[1] = vec4(1, 0, 0, 1);
    colors[2] = vec4(0, 1, 0, 1);
    colors[3] = vec4(0, 0, 1, 1);
    colors[4] = vec4(1, 1, 0, 1);
    colors[5] = vec4(0, 1, 1, 1);
    colors[6] = vec4(1, 1, 1, 1);
    // imageStore(img_output, coords, vec4(vec3(numLights / (1.0*MAX_LIGHTS_PER_FRUSTUM)), 1));
    imageStore(img_output, coords, colors[val]);
    
    /*
    
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
    */
}
#version 430
#define WORK_GROUP_SIZE 16
layout(local_size_x = WORK_GROUP_SIZE, local_size_y = WORK_GROUP_SIZE) in;
layout(rgba16f, binding = 0) writeonly uniform image2D imgOutput;
layout(rgba16f, binding = 1) writeonly uniform image2D bloomOutput;

layout(rgba32f, binding = 2) readonly uniform image2D gPosition;
layout(rgba32f, binding = 3) readonly uniform image2D gNormal;
layout(rgba8, binding = 4) readonly uniform image2D gDiffuse;
layout(rgba32f, binding = 5) readonly uniform image2D gSpecularExp;
layout(rgba16f, binding = 6) readonly uniform image2D gEmissive;


uniform int numPointLights;
uniform int numDirectionalLights;
uniform ivec2 screenSize;
uniform mat4 invProjMatrix;


layout(std430, binding=10) buffer point_light_list
{
    vec4 lights[];
};

#define EPSILON 0.000001
#define MAX_LIGHTS_PER_TILE 600
shared int tileLightList[MAX_LIGHTS_PER_TILE];
shared int numLightsInTile;

shared uint minGroupDepth;
shared uint maxGroupDepth;

void sync() {
    memoryBarrierShared();
    barrier();
}

float getSignedDistanceFromPlane(in vec3 plane, in vec3 point) {
    return dot(plane, point);
}

vec3 tileCornerToViewSpace(in int pixelX, in int pixelY) {
    vec2 NDCPixel = 2.0 * vec2(float(pixelX) / screenSize.x, float(pixelY) / screenSize.y) - vec2(1.0);
    vec4 p = vec4(NDCPixel, 1.0, 1.0);
    return (invProjMatrix * p).xyz;
}

vec3 createPlane(in vec3 v1, in vec3 v2) {
    return normalize(cross(v2, v1));
}

void main() {
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    if (gl_LocalInvocationID.x == 0) {
        numLightsInTile = 0;
        maxGroupDepth = 0;
        minGroupDepth = 0xFFFFFFFF;
    }
    sync();
        
    vec3 position = imageLoad(gPosition, coords).xyz;
    uint depthAsUint = uint(position.z * 0xFFFFFFFF);
    atomicMin(minGroupDepth, depthAsUint);
    atomicMax(maxGroupDepth, depthAsUint);
    
    sync();
    
    float minZ = float(minGroupDepth / float(0xFFFFFFFF));
    float maxZ = float(maxGroupDepth / float(0xFFFFFFFF));
    
    ivec2 tileID = ivec2(gl_WorkGroupID.xy);
    vec3 tileCorners[4];
    tileCorners[0] = tileCornerToViewSpace(WORK_GROUP_SIZE * tileID.x, WORK_GROUP_SIZE * tileID.y); // bl
    tileCorners[1] = tileCornerToViewSpace(WORK_GROUP_SIZE * (tileID.x + 1), WORK_GROUP_SIZE * tileID.y); // br
    tileCorners[2] = tileCornerToViewSpace(WORK_GROUP_SIZE * (tileID.x+1), WORK_GROUP_SIZE * (tileID.y+1)); // tr
    tileCorners[3] = tileCornerToViewSpace(WORK_GROUP_SIZE * tileID.x, WORK_GROUP_SIZE * (tileID.y + 1)); // tl

    vec3 frustumPlanes[4];
    for (int i = 0; i < 4; ++i) {
        frustumPlanes[i] = createPlane(tileCorners[i], tileCorners[(i + 1) & 3]);
    }
    
    sync();
    
    int threadsPerTile = WORK_GROUP_SIZE * WORK_GROUP_SIZE;
    for (int i = int(gl_LocalInvocationIndex); i < numPointLights; i += threadsPerTile) {
        vec4 light = lights[2 * (numDirectionalLights + i) + 0];
        vec3 pos = light.xyz;
        float r = light.w;
        
        if (numLightsInTile < MAX_LIGHTS_PER_TILE) {
            if (pos.z + r > maxZ &&
                pos.z - r < minZ &&
                getSignedDistanceFromPlane(frustumPlanes[0], pos) < r &&
                getSignedDistanceFromPlane(frustumPlanes[1], pos) < r &&
                getSignedDistanceFromPlane(frustumPlanes[2], pos) < r &&
                getSignedDistanceFromPlane(frustumPlanes[3], pos) < r)
                {
                    uint id = atomicAdd(numLightsInTile, 1);
                    tileLightList[id] = i;
                }
        }
    }
    
    sync();
    
    //imageStore(img_output, coords, vec4(vec3(float(numLightsInTile) / MAX_LIGHTS_PER_TILE), 1));
    
    vec3 n            = imageLoad(gNormal, coords).xyz;
    vec3 diffuseColor = imageLoad(gDiffuse, coords).rgb;
    vec4 specExp      = imageLoad(gSpecularExp, coords);

    vec3 e = normalize(-position);
    vec3 color = imageLoad(gEmissive, coords).rgb;

    for (int i = 0; i < numDirectionalLights; ++i) {
        vec3 lightDir   = lights[2 * i + 0].xyz;
        vec3 lightColor = lights[2 * i + 1].xyz;
        vec3 l = normalize(-lightDir);
        vec3 h = normalize(l + e);
        // color += lightColor * ka;
        color += lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            color += lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), 4*specExp.a);
    }

    for (int i = 0; i < numLightsInTile; ++i) {
        int lightID = tileLightList[i];
        vec3 lightPos = lights[2 * (lightID + numDirectionalLights) + 0].xyz;
        vec3 lightColor = lights[2 * (lightID + numDirectionalLights) + 1].rgb;
        
        vec3 l = normalize(lightPos - position);
        vec3 h = normalize(l + e);
        float attenuation = 1.0 / pow(length(lightPos - position), 2.0);
              
        // outColor += lightColor * ka;
        color += attenuation * lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            color += lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), 4*specExp.a);
    }
    
    imageStore(imgOutput, coords, vec4(color, 1));
    
    vec3 brightnessVec = vec3(0.2126, 0.7152, 0.0722);
	// vec3 brightnessVec = vec3(0.299, 0.587, 0.114);
    vec4 glowColor = vec4(0, 0, 0, 1);
	if (dot(brightnessVec, color) > 1)
		glowColor = vec4(color, 1);

    imageStore(bloomOutput, coords, glowColor);
}

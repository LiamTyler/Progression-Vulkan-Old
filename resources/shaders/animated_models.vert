#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;
layout( location = 3 ) in vec3 inTangent;
layout( location = 4 ) in vec4 inBoneWeights;
layout( location = 5 ) in uvec4 inBoneJoints;

layout( location = 0 ) out vec3 posInWorldSpace;
layout( location = 1 ) out vec2 texCoord;
layout( location = 2 ) out mat3 TBN;

layout( set = PG_SCENE_CONSTANT_BUFFER_SET, binding = 0 ) uniform SceneConstantBufferUniform
{
    SceneConstantBufferData sceneConstantBuffer;
};

layout( std430, push_constant ) uniform PerObjectData
{
    AnimatedObjectConstantBufferData perObjectData;
};

layout( set = 2, binding = 0 ) buffer BoneTransforms
{
   mat4 boneTransforms[];
};

void main()
{
    // mat4 BoneTransform = boneTransforms[inBoneJoints[0]] * inBoneWeights[0];
    // BoneTransform     += boneTransforms[inBoneJoints[1]] * inBoneWeights[1];
    // BoneTransform     += boneTransforms[inBoneJoints[2]] * inBoneWeights[2];
    // BoneTransform     += boneTransforms[inBoneJoints[3]] * inBoneWeights[3];
    // vec4 localPos    = BoneTransform * vec4( inPosition, 1 );
    // vec4 localNormal = BoneTransform * vec4( inNormal, 0 );
    uint offset = perObjectData.boneTransformIdx;
    //vec4 localPos    = vec4( inPosition, 1 );
    //vec4 localNormal = vec4( inNormal, 0 );
    
    vec4 localPos    = vec4( 0 );
    vec4 localNormal = vec4( 0 );
    localPos += inBoneWeights[0] * boneTransforms[offset + inBoneJoints[0]] * vec4( inPosition, 1 );
    localPos += inBoneWeights[1] * boneTransforms[offset + inBoneJoints[1]] * vec4( inPosition, 1 );
    localPos += inBoneWeights[2] * boneTransforms[offset + inBoneJoints[2]] * vec4( inPosition, 1 );
    localPos += inBoneWeights[3] * boneTransforms[offset + inBoneJoints[3]] * vec4( inPosition, 1 );
    
    localNormal += inBoneWeights[0] * boneTransforms[offset + inBoneJoints[0]] * vec4( inNormal, 0 );
    localNormal += inBoneWeights[1] * boneTransforms[offset + inBoneJoints[1]] * vec4( inNormal, 0 );
    localNormal += inBoneWeights[2] * boneTransforms[offset + inBoneJoints[2]] * vec4( inNormal, 0 );
    localNormal += inBoneWeights[3] * boneTransforms[offset + inBoneJoints[3]] * vec4( inNormal, 0 );

    texCoord            = inTexCoord;
    gl_Position         = sceneConstantBuffer.VP * perObjectData.M * localPos;
    posInWorldSpace     = ( perObjectData.M * localPos ).xyz;
    
    vec3 worldT = normalize( ( perObjectData.M * vec4( inTangent, 0 ) ).xyz );
    vec3 worldN = normalize( ( perObjectData.N * localNormal ).xyz );
    vec3 worldB = cross( worldN, worldT );
    TBN         = mat3( worldT, worldB, worldN );
}

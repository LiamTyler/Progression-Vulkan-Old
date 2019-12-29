#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec4 inBoneWeights;
layout( location = 2 ) in uvec4 inBoneJoints;

layout( std430, push_constant ) uniform PerObjectData
{
    AnimatedShadowPerObjectData perObjectData;
};

layout( set = PG_BONE_TRANSFORMS_SET, binding = 0 ) buffer BoneTransforms
{
   mat4 boneTransforms[];
};

void main()
{
    uint offset = perObjectData.boneTransformIdx;
    mat4 BoneTransform = boneTransforms[offset + inBoneJoints[0]] * inBoneWeights[0];
    BoneTransform     += boneTransforms[offset + inBoneJoints[1]] * inBoneWeights[1];
    BoneTransform     += boneTransforms[offset + inBoneJoints[2]] * inBoneWeights[2];
    BoneTransform     += boneTransforms[offset + inBoneJoints[3]] * inBoneWeights[3];
    vec4 localPos      = BoneTransform * vec4( inPosition, 1 );

    gl_Position        = perObjectData.MVP * localPos;
}

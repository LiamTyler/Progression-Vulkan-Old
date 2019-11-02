#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;
layout( location = 3 ) in vec4 inBoneWeights;
layout( location = 4 ) in uvec4 inBoneJoints;

layout( location = 0 ) out vec3 posInWorldSpace;
layout( location = 1 ) out vec3 normalInWorldSpace;
layout( location = 2 ) out vec2 texCoord;

layout( set = 0, binding = 0 ) uniform SceneConstantBuffer
{
    mat4 VP;
} sceneConstantBuffer;

layout( set = 2, binding = 0 ) buffer BoneTransforms
{
   mat4 boneTransforms[];
};

layout( std430, push_constant ) uniform PerObjectData
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} perObjectData;

void main()
{
    // mat4 BoneTransform = boneTransforms[inBoneJoints[0]] * inBoneWeights[0];
    // BoneTransform     += boneTransforms[inBoneJoints[1]] * inBoneWeights[1];
    // BoneTransform     += boneTransforms[inBoneJoints[2]] * inBoneWeights[2];
    // BoneTransform     += boneTransforms[inBoneJoints[3]] * inBoneWeights[3];
    // vec4 localPos    = BoneTransform * vec4( inPosition, 1 );
    // vec4 localNormal = BoneTransform * vec4( inNormal, 0 );
    
    vec4 localPos    = vec4( 0 );
    localPos += inBoneWeights[0] * boneTransforms[inBoneJoints[0]] * vec4( inPosition, 1 );
    localPos += inBoneWeights[1] * boneTransforms[inBoneJoints[1]] * vec4( inPosition, 1 );
    localPos += inBoneWeights[2] * boneTransforms[inBoneJoints[2]] * vec4( inPosition, 1 );
    localPos += inBoneWeights[3] * boneTransforms[inBoneJoints[3]] * vec4( inPosition, 1 );
    
    vec4 localNormal = vec4( 0 );
    localNormal += inBoneWeights[0] * boneTransforms[inBoneJoints[0]] * vec4( inNormal, 0 );
    localNormal += inBoneWeights[1] * boneTransforms[inBoneJoints[1]] * vec4( inNormal, 0 );
    localNormal += inBoneWeights[2] * boneTransforms[inBoneJoints[2]] * vec4( inNormal, 0 );
    localNormal += inBoneWeights[3] * boneTransforms[inBoneJoints[3]] * vec4( inNormal, 0 );

    texCoord           = inTexCoord;
    gl_Position         = sceneConstantBuffer.VP * perObjectData.modelMatrix * localPos;
    posInWorldSpace     = ( perObjectData.modelMatrix * localPos ).xyz;
    normalInWorldSpace  = ( perObjectData.normalMatrix * localNormal ).xyz;
}

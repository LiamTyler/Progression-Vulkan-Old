#pragma once

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/lights.h"

PG_NAMESPACE_BEGIN
PG_GPU_NAMESPACE_BEGIN

struct SceneConstantBufferData
{
    MAT4 V;
    MAT4 P;
    MAT4 VP;
    MAT4 invVP;
    MAT4 LSM;
    VEC4 cameraPos;
    VEC4 ambientColor;
    DirectionalLight dirLight;
    UINT numPointLights;
    UINT numSpotLights;
};

struct ObjectConstantBufferData
{
    MAT4 M;
    MAT4 N;
};

struct AnimatedObjectConstantBufferData
{
    MAT4 M;
    MAT4 N;
    UINT boneTransformIdx;
};

struct AnimatedShadowPerObjectData
{
    MAT4 MVP;
    UINT boneTransformIdx;
};

struct MaterialConstantBufferData
{
    VEC4 Kd;
    float roughness;
    float metallic;
    UINT diffuseTexIndex;
    UINT normalTexIndex;
    UINT metallicTexIndex;
    UINT roughnessTexIndex;
};

struct SSAOShaderData
{
    MAT4 V;
    MAT4 P;
    MAT4 invP;
};

PG_GPU_NAMESPACE_END
PG_NAMESPACE_END
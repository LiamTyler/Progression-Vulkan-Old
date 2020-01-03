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
    VEC4 Ks;
    UINT diffuseTexIndex;
    UINT normalMapIndex;
};

struct SSAOShaderData
{
    MAT4 V;
    MAT4 P;
};

PG_GPU_NAMESPACE_END
PG_NAMESPACE_END
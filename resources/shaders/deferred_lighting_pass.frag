#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/lights.h"
#include "graphics/shader_c_shared/structs.h"
#include "lighting_functions.h"

layout( location = 0 ) in vec2 UV;

layout( location = 0 ) out vec4 outColor;

layout( set = PG_SCENE_CONSTANT_BUFFER_SET, binding = 0 ) uniform SceneConstantBufferUniform
{
    SceneConstantBufferData sceneConstantBuffer;
};

layout( std140, set = 3, binding = 1 ) buffer PointLights
{
    PointLight pointLights[];
};

layout( std140, set = 3, binding = 2 ) buffer SpotLights
{
    SpotLight spotLights[];
};

layout( set = PG_2D_TEXTURES_SET, binding = 0 ) uniform sampler2D textures[PG_MAX_NUM_TEXTURES];

layout( set = 2, binding = 0 ) uniform sampler2D positionTex;
layout( set = 2, binding = 1 ) uniform sampler2D normalTex;
layout( set = 2, binding = 2 ) uniform usampler2D diffuseAndSpecularTex;
layout( set = 2, binding = 3 ) uniform sampler2D ssaoTex;

layout( std430, push_constant ) uniform SSAOToggle
{
    layout( offset = 0 ) int ssaoOn;
};

void UnpackShortToTwoFloats( in const uint packed, out float x, out float y )
{
    x = float( packed >> 8 ) / 255.0;
    y = float( packed & 0xFF ) / 255.0;
}

void UnpackDiffuseAndSpecular( in const uvec4 packed, out vec3 Kd, out vec4 Ks )
{
    UnpackShortToTwoFloats( packed.x, Kd.x, Ks.x );
    UnpackShortToTwoFloats( packed.y, Kd.y, Ks.y );
    UnpackShortToTwoFloats( packed.z, Kd.z, Ks.z );
    Ks.w = float( packed.w );
}

void main()
{
    vec3 posInWorldSpace = texture( positionTex, UV ).xyz;
    vec3 n               = texture( normalTex,   UV ).xyz;
    uvec4 packed = texture( diffuseAndSpecularTex,  UV );
    vec3 Kd;
    vec4 Ks;
    UnpackDiffuseAndSpecular( packed, Kd, Ks );

    vec3 e     = normalize( sceneConstantBuffer.cameraPos.xyz - posInWorldSpace );
    vec3 color = vec3( 0, 0, 0 );
    
    // ambient
    float ambientOcclusion = 1;
    if ( ssaoOn != 0 )
    {
        ambientOcclusion = texture( ssaoTex, UV ).r;
    }
    color += ambientOcclusion * Kd * sceneConstantBuffer.ambientColor.xyz;

    // directional light
    vec3 lightColor = sceneConstantBuffer.dirLight.colorAndIntensity.w * sceneConstantBuffer.dirLight.colorAndIntensity.xyz;
    vec3 l = normalize( -sceneConstantBuffer.dirLight.direction.xyz );
    vec3 h = normalize( l + e );
    
    // float S = 1 - ShadowAmount( posInWorldSpace );
    float S = 1 - ShadowAmount( sceneConstantBuffer.DLSM * vec4( posInWorldSpace, 1 ), textures[sceneConstantBuffer.shadowTextureIndex] );
    color += S * lightColor * Kd * max( 0.0, dot( l, n ) );
    if ( dot( l, n ) > PG_SHADER_EPSILON )
    {
        color += S * lightColor * Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4 * Ks.w );
    }

    // pointlights
    for ( uint i = 0; i < sceneConstantBuffer.numPointLights; ++i )
    {
        vec3 lightColor = pointLights[i].colorAndIntensity.w * pointLights[i].colorAndIntensity.xyz;
        vec3 d = pointLights[i].positionAndRadius.xyz - posInWorldSpace;
        vec3 l = normalize( d );
        vec3 h = normalize( l + e );
        float attenuation = Attenuate( dot( d, d ), pointLights[i].positionAndRadius.w * pointLights[i].positionAndRadius.w );

        color += attenuation * lightColor * Kd * max( 0.0, dot( l, n ) );
        if ( dot( l, n ) > PG_SHADER_EPSILON )
        {
            color += attenuation * lightColor * Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4 * Ks.w );
        }
    }
    
    // spotlights
    for ( uint i = 0; i < sceneConstantBuffer.numSpotLights; ++i )
    {
        vec3 l = normalize( spotLights[i].positionAndRadius.xyz - posInWorldSpace );
        float theta = dot( -l, spotLights[i].directionAndCutoff.xyz );
        if ( theta > cos( spotLights[i].directionAndCutoff.w ) )
        {
            vec3 lightColor = spotLights[i].colorAndIntensity.w * spotLights[i].colorAndIntensity.xyz;
            vec3 d = spotLights[i].positionAndRadius.xyz - posInWorldSpace;
            vec3 l = normalize( d );
            vec3 h = normalize( l + e );
            float attenuation = Attenuate( dot( d, d ), spotLights[i].positionAndRadius.w * spotLights[i].positionAndRadius.w );

            color += attenuation * lightColor * Kd * max( 0.0, dot( l, n ) );
            if ( dot( l, n ) > PG_SHADER_EPSILON )
            {
                color += attenuation * lightColor * Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4 * Ks.w );
            }
        }
    }
    
    outColor = vec4( color, 1.0 );
}

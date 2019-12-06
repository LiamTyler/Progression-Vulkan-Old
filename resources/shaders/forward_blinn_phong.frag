#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/lights.h"
#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) out vec4 outColor;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec2 texCoord;
layout( location = 2 ) in mat3 TBN;


layout( set = PG_SCENE_CONSTANT_BUFFER_SET, binding = 0 ) uniform SceneConstantBufferUniform
{
    SceneConstantBufferData sceneConstantBuffer;
};

layout( std140, set = PG_SCENE_CONSTANT_BUFFER_SET, binding = 1 ) buffer PointLights
{
    PointLight pointLights[];
};

layout( std140, set = PG_SCENE_CONSTANT_BUFFER_SET, binding = 2 ) buffer SpotLights
{
    SpotLight spotLights[];
};

layout( set = PG_2D_TEXTURES_SET, binding = 0 ) uniform sampler2D textures[PG_MAX_NUM_TEXTURES];

layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = PG_MATERIAL_PUSH_CONSTANT_OFFSET ) MaterialConstantBufferData material;
};

float Attenuate( in const float distSquared, in const float radiusSquared )
{
    float frac = distSquared / radiusSquared;
    float atten = max( 0, 1 - frac * frac );
    return (atten * atten) / ( 1.0 + distSquared );
}

float ShadowAmount( in const vec3 fragWorldPos )
{
    vec4 posInLightSpace = sceneConstantBuffer.DLSM * vec4( fragWorldPos, 1 );
    vec3 ndc             = posInLightSpace.xyz / posInLightSpace.w;
    vec3 projCoords      = .5 * ndc + vec3( .5 );
    projCoords.y         = 1 - projCoords.y; // Account for flip in projection matrix
    float currentDepth   = ndc.z; // Already between 0 and 1
    
    if ( currentDepth > 1 )
    {
        return 0;
    }

    float shadowMapDepth = texture( textures[sceneConstantBuffer.shadowTextureIndex], projCoords.xy ).r;
    if ( shadowMapDepth < currentDepth )
    {
        return 1.0;
    }
    
    return 0;
}

void main()
{
    // vec3 n = normalize( normalInWorldSpace );
    vec3 n = normalize( TBN[2] );
    if ( material.normalMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        n = texture( textures[material.normalMapIndex], texCoord ).xyz;
        n = normalize( n * 2 - 1 );
        n = normalize( TBN * n );
    }
    
    vec3 e = normalize( sceneConstantBuffer.cameraPos.xyz - posInWorldSpace );

    vec3 color = vec3( 0, 0, 0 );
    vec3 Kd    = material.Kd.xyz;
    if ( material.diffuseTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        Kd *= texture( textures[material.diffuseTexIndex], texCoord ).xyz;
    }
    color += material.Kd.xyz * sceneConstantBuffer.ambientColor.xyz;
    
    vec3 lightColor = sceneConstantBuffer.dirLight.colorAndIntensity.w * sceneConstantBuffer.dirLight.colorAndIntensity.xyz;
    vec3 l = normalize( -sceneConstantBuffer.dirLight.direction.xyz );
    vec3 h = normalize( l + e );
    
    float S = 1 - ShadowAmount( posInWorldSpace );
    color += S * lightColor * Kd * max( 0.0, dot( l, n ) );
    if ( dot( l, n ) > PG_SHADER_EPSILON )
    {
        color += S * lightColor * material.Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4 * material.Ks.w );
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
            color += attenuation * lightColor * material.Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4 * material.Ks.w );
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
                color += attenuation * lightColor * material.Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4 * material.Ks.w );
            }
        }
    }
        
    outColor = vec4( color, 1.0 );
}

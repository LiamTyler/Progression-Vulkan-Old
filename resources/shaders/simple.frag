#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/standard_defines.h"
#include "graphics/shader_c_shared/lights.h"

layout( location = 0 ) out vec4 outColor;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec3 normalInWorldSpace;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in float boneSum;

layout( set = 0, binding = 0 ) uniform SceneConstantBuffer
{
    mat4 VP;
    vec3 cameraPos;
    vec3 ambientColor;
    DirectionalLight dirLight;
    uint numPointLights;
    uint numSpotLights;
} sceneConstantBuffer;

layout( std140, binding = 1 ) buffer PointLights
{
   PointLight pointLights[];
};

layout( std140, binding = 2 ) buffer SpotLights
{
   SpotLight spotLights[];
};

layout( set = 2, binding = 0 ) uniform sampler2D textures[PG_MAX_NUM_TEXTURES];

layout( std430, push_constant ) uniform MaterialConstantBuffer
{
    layout( offset = 128 ) vec4 Ka;
    vec4 Kd;
    vec4 Ks;
    uint diffuseTexIndex;
} material;

float Attenuate( in const float distSquared, in const float radiusSquared )
{
    float frac = distSquared / radiusSquared;
    float atten = max( 0, 1 - frac * frac );
    return (atten * atten) / ( 1.0 + distSquared );
}

void main()
{
    //outColor.xyz = vec3( 1, 0, 0 );
    //outColor.w = 1; 
    //return;
    //outColor.xyz = vec3( boneSum );
    //outColor.w = 1; 
    //return;
    vec3 n = normalize( normalInWorldSpace );
    vec3 e = normalize( sceneConstantBuffer.cameraPos - posInWorldSpace );

    vec3 color = material.Ka.xyz * sceneConstantBuffer.ambientColor;
    vec3 Kd    = material.Kd.xyz;
    if ( material.diffuseTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        Kd *= texture( textures[material.diffuseTexIndex], texCoord ).xyz;
    }
    
    // outColor = vec4( Kd, 1 );
    // return;
    // directional light
    vec3 lightColor = sceneConstantBuffer.dirLight.colorAndIntensity.w * sceneConstantBuffer.dirLight.colorAndIntensity.xyz;
    vec3 l = normalize( -sceneConstantBuffer.dirLight.direction.xyz );
    vec3 h = normalize( l + e );

    color += lightColor * Kd * max( 0.0, dot( l, n ) );
    if ( dot( l, n ) > PG_SHADER_EPSILON )
    {
        color += lightColor * material.Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4 * material.Ks.w );
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
    //for ( uint i = 0; i < sceneConstantBuffer.numSpotLights; ++i )
    for ( uint i = 0; i < 1; ++i )
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

#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/lights.h"
#include "graphics/shader_c_shared/structs.h"
#include "lighting_functions.h"
#include "packing.h"

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

// layout( std140, set = 3, binding = 2 ) buffer SpotLights
// {
//     SpotLight spotLights[];
// };

layout( set = PG_2D_TEXTURES_SET, binding = 0 ) uniform sampler2D textures[PG_MAX_NUM_TEXTURES];

layout( set = 2, binding = 0 ) uniform sampler2D normalTex;
layout( set = 2, binding = 1 ) uniform sampler2D diffuseTex;
layout( set = 2, binding = 2 ) uniform sampler2D metallicAndRoughnessTex;
layout( set = 2, binding = 3 ) uniform sampler2D ssaoTex;
layout( set = 2, binding = 4 ) uniform sampler2D depthTex;

#if PG_DEBUG_BUILD
layout( std430, push_constant ) uniform DebugSwitch
{
    DebugRenderData debugData;
};
#endif // #if PG_DEBUG_BUILD

float D_GGX( float NdotH, float roughness )
{
    float a     = roughness * roughness;
    float a2    = a*a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    denom       = PI * denom * denom;
	
    return a2 / max( denom, 0.001 );
}
  
float G_SchlickSmithGGX( float NdotL, float NdotV, float roughness )
{
    float k  = roughness + 1;
    k        = (k * k) / 8.0;
    float GL = NdotL / (NdotL * (1.0 - k) + k);
    float GV = NdotV / (NdotV * (1.0 - k) + k);
	
    return GL * GV;
}

vec3 F_Schlick( float cosTheta, vec3 F0 )
{
    return F0 + (1.0 - F0) * pow( 1.0 - cosTheta, 5 );
}

void BRDF( vec3 L, vec3 radiance, vec3 V, vec3 N, float metallic, float roughness,
           float shadowed, vec3 albedo, vec3 F0, inout vec3 diffuse, inout vec3 specular )
{
    vec3 H      = normalize( V + L );
	float NdotV = max( dot( N, V ), 0.0 );
	float NdotL = max( dot( N, L ), 0.0 );
	float NdotH = max( dot( N, H ), 0.0 );

	if ( NdotL > 0 )
	{
		// D = Normal distribution (Distribution of the microfacets)
        // G = Geometric shadowing term (Microfacets shadowing)
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float D = D_GGX( NdotH, roughness ); 
		float G = G_SchlickSmithGGX( NdotL, NdotV, roughness );
		vec3 F  = F_Schlick( max( dot( V, H ), 0.0 ), F0 );

		vec3 spec = D * F * G / max( 4.0 * NdotL * NdotV, 0.001 );
        vec3 kS   = F;
        vec3 kD   = (vec3( 1.0 ) - kS) * (1.0 - metallic); // metallic objects dont have diffuse color
        
        vec3 lightEnergy = shadowed * NdotL * radiance;
		diffuse  += lightEnergy * kD * albedo / PI;
		specular += lightEnergy * spec; // kS since kS == F and F is already multiplied into spec
	}
}

void main()
{
    vec3 posInWorldSpace      = ReconstructPosFromDepth( sceneConstantBuffer.invVP, UV, texture( depthTex, UV ).r );
    vec3 N                    = DecodeOctVec( texture( normalTex, UV ).xyz );
    vec3 albedo               = texture( diffuseTex, UV ).xyz;
    float metallic            = texture( metallicAndRoughnessTex, UV ).x;
    float roughness           = texture( metallicAndRoughnessTex, UV ).y;
    vec3 F0                   = mix( vec3( 0.04 ), albedo, metallic );
    vec3 V                    = normalize( sceneConstantBuffer.cameraPos.xyz - posInWorldSpace );
    
    vec3 ambientColor  = vec3( 0, 0, 0 );
    vec3 diffuseColor  = vec3( 0, 0, 0 );
    vec3 specularColor = vec3( 0, 0, 0 );

    float ambientOcclusion = texture( ssaoTex, UV ).r;
    ambientColor = ambientOcclusion * albedo * sceneConstantBuffer.ambientColor.xyz;

    // directional light
    {
        vec3 lightColor = sceneConstantBuffer.dirLight.colorAndIntensity.w * sceneConstantBuffer.dirLight.colorAndIntensity.xyz;
        vec3 L = normalize( -sceneConstantBuffer.dirLight.direction.xyz );
        
        float S = 1;
        if ( sceneConstantBuffer.dirLight.shadowMapIndex.x != PG_INVALID_TEXTURE_INDEX )
        {
            S = 1 - ShadowAmount( sceneConstantBuffer.LSM * vec4( posInWorldSpace, 1 ), textures[sceneConstantBuffer.dirLight.shadowMapIndex.x] );
        }
        BRDF( L, lightColor, V, N, metallic, roughness, S, albedo, F0, diffuseColor, specularColor );
    }
    
    // pointlights
    for ( uint i = 0; i < sceneConstantBuffer.numPointLights; ++i )
    {
        vec3 lightColor = pointLights[i].colorAndIntensity.w * pointLights[i].colorAndIntensity.xyz;
        vec3 d  = pointLights[i].positionAndRadius.xyz - posInWorldSpace;
        vec3 L  = normalize( d );
        float a = Attenuate( dot( d, d ), pointLights[i].positionAndRadius.w * pointLights[i].positionAndRadius.w );
        BRDF( L, a * lightColor, V, N, metallic, roughness, 1, albedo, F0, diffuseColor, specularColor );
    }

    // // spotlights
    // for ( uint i = 0; i < sceneConstantBuffer.numSpotLights; ++i )
    // {
    //     vec3 l = normalize( spotLights[i].positionAndRadius.xyz - posInWorldSpace );
    //     float theta = dot( -l, spotLights[i].directionAndCutoff.xyz );
    //     if ( theta > cos( spotLights[i].directionAndCutoff.w ) )
    //     {
    //         vec3 lightColor = spotLights[i].colorAndIntensity.w * spotLights[i].colorAndIntensity.xyz;
    //         vec3 d = spotLights[i].positionAndRadius.xyz - posInWorldSpace;
    //         vec3 l = normalize( d );
    //         vec3 h = normalize( l + V );
    //         float attenuation = Attenuate( dot( d, d ), spotLights[i].positionAndRadius.w * spotLights[i].positionAndRadius.w );
    // 
    //         diffuseColor += attenuation * lightColor * Kd * max( 0.0, dot( l, N ) );
    //         if ( dot( l, N ) > PG_SHADER_EPSILON )
    //         {
    //             specularColor += attenuation * lightColor * Ks.xyz * pow( max( dot( h, N ), 0.0 ), 4 * Ks.w );
    //         }
    //     }
    // }
    
    vec3 color;
#if PG_DEBUG_BUILD
    if ( debugData.layer == PG_SHADER_DEBUG_LAYER_REGULAR )
    {
        color = ambientColor + diffuseColor + specularColor;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_AMBIENT )
    {
        color = ambientColor;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_LIT_DIFFUSE )
    {
        color = diffuseColor;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_LIT_SPECULAR )
    {
        color = specularColor;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_NO_SSAO )
    {
        ambientColor = albedo * sceneConstantBuffer.ambientColor.xyz;
        color = ambientColor + diffuseColor + specularColor;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_SSAO_ONLY )
    {
        color = vec3( texture( ssaoTex, UV ).r );    
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_POSITIONS )
    {
        color = posInWorldSpace;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_NORMALS )
    {
        color = N;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_ALBEDO )
    {
        color = albedo;
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_METALLIC )
    {
        color = vec3( metallic );
    }
    else if ( debugData.layer == PG_SHADER_DEBUG_LAYER_ROUGHNESS )
    {
        color = vec3( roughness );
    }
    else
    {
        color = vec3( UV, 1 );
    }
#else // #if PG_DEBUG_BUILD
    color = ambientColor + diffuseColor + specularColor;
#endif // #else // #if PG_DEBUG_BUILD
    
    outColor = vec4( color, 1.0 );
}

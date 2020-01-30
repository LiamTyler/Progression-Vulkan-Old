#include "graphics/shader_c_shared/defines.h"

float D_GGX( vec3 N, vec3 H, float a )
{
    float a2    = a*a;
    float NdotH = max( dot( N, H ), 0.0 );	
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;
	
    return a2 / denom;
}

float GeometrySchlickGGX( float NdotV, float k )
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}
  
float G_SchlickSmithGGX( vec3 NdotL, vec3 NdotV, float roughness )
{
    float k  = roughness + 1;
    k        = (k * k) / 8.0;
    float GL = NdotL / (NdotL * (1.0 - k) + k);
    float GV = NdotV / (NdotV * (1.0 - k) + k);
	
    return GL * GV;
}

// vec3 F0 = vec3(0.04);
// F0      = mix(F0, surfaceColor.rgb, metalness);
vec3 F_Schlick( float cosTheta, float metallic, vec3 albedo )
{
    vec3 F0 = mix( vec4( 0.04 ), albedo, metallic );
    return F0 + (1.0 - F0) * pow( 1.0 - cosTheta, 5 );
}

vec3 BRDF( vec3 lightDir, vec3 lightColor, vec3 V, vec3 N, float metallic, float roughness )
{
    vec3 H = normalize( V + lightDir );
	float NdotV = clamp( dot( N, V ), 0.0, 1.0 );
	float NdotL = clamp( dot( N, lightDir ), 0.0, 1.0 );
	float NdotH = clamp( dot( N, H ), 0.0, 1.0 );

	vec3 color = vec3( 0.0 );

	if ( NdotL > 0.0 )
	{
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX( NdotH, roughness ); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX( NdotL, NdotV, roughness );
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick( NdotV, metallic );

		vec3 spec = D * F * G / (4.0 * NdotL * NdotV);

		color += spec * NdotL * lightColor;
	}

	return color;
}
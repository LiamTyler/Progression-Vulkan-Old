#define PCF_HALF_WIDTH 1
#define PCF_FULL_WIDTH ( 2 * PCF_HALF_WIDTH + 1 )

float Attenuate( in const float distSquared, in const float radiusSquared )
{
    float frac = distSquared / radiusSquared;
    float atten = max( 0, 1 - frac * frac );
    return (atten * atten) / ( 1.0 + distSquared );
}

float ShadowAmount( in const vec4 posInLightSpace, in sampler2D shadowMap )
{
    vec3 ndc             = posInLightSpace.xyz / posInLightSpace.w;
    vec3 projCoords      = .5 * ndc + vec3( .5 );
    projCoords.y         = 1 - projCoords.y; // Account for flip in projection matrix
    float currentDepth   = ndc.z; // Already between 0 and 1
    
    if ( currentDepth > 1 )
    {
        return 0;
    }
    
    vec2 dUV = 1.0 / textureSize( shadowMap, 0 );
    float totalShadowStrength = 0;
    for ( int r = -PCF_HALF_WIDTH; r <= PCF_HALF_WIDTH; ++r )
    {
        for ( int c = -PCF_HALF_WIDTH; c <= PCF_HALF_WIDTH; ++c )
        {
            vec2 coords = projCoords.xy + vec2( c * dUV.x, r * dUV.y );
            float d = texture( shadowMap, coords ).r;
            if ( d < currentDepth )
            {
                totalShadowStrength += 1.0;
            }
        }
    }
    
    return totalShadowStrength / ( PCF_FULL_WIDTH * PCF_FULL_WIDTH );
}
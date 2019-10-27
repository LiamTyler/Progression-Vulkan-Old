#version 450
#extension GL_ARB_separate_shader_objects : enable

#define EPSILON 0.00001

layout( location = 0 ) out vec4 outColor;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec3 normalInWorldSpace;
layout( location = 2 ) in vec2 texCoord;

layout( set = 0, binding = 0 ) uniform PerSceneConstantBuffer
{
    mat4 VP;
    vec3 cameraPos;
} perSceneConstantBuffer;

layout( set = 1, binding = 0 ) uniform MaterialConstantBuffer
{
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;
} material;

layout( set = 1, binding = 1 ) uniform sampler2D texSampler;

void main()
{
    vec3 n = normalize( normalInWorldSpace );
    vec3 e = normalize( perSceneConstantBuffer.cameraPos - posInWorldSpace );
    
    vec3 lightDir   = normalize( vec3( 1, 0, -1 ) );
    vec3 lightColor = vec3( 1, 1, 1 );
    vec3 l = normalize( -lightDir );
    vec3 h = normalize( l + e );

    vec3 color = material.Ka.xyz;
    // Kd *= texture( texSampler, texCoord ).xyz;
    color += lightColor * material.Kd.xyz * max( 0.0, dot( l, n ) );
    if ( dot( l, n ) > EPSILON )
    {
        color += lightColor * material.Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4*material.Ks.w );
    }
        
    outColor = vec4( color, 1.0 );
}

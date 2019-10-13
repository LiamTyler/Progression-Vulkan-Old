#version 450
#extension GL_ARB_separate_shader_objects : enable

#define EPSILON 0.00001

layout( location = 0 ) out vec4 outColor;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec3 normalInWorldSpace;
layout( location = 2 ) in vec2 texCoord;

layout( binding = 1 ) uniform sampler2D texSampler;

void main()
{
    vec3 cameraPos = vec3( 0, 0, 3 );
    vec3 n = normalize( normalInWorldSpace );
    vec3 e = normalize( cameraPos - posInWorldSpace );
    
    vec3 lightDir   = normalize( vec3( 1, 0, -1 ) );
    // vec3 lightDir   = normalize( vec3( 1, -1, -1 ) );
    vec3 lightColor = vec3( 1, 1, 1 );
    vec3 l = normalize( -lightDir );
    vec3 h = normalize( l + e );

    vec3 Ka = vec3( 0.0 );
    vec3 Kd = vec3( 1, 0, 0 );
    vec3 Ks = vec3( 1, 1, 1 );
    float specular = 200;
    
    vec3 color = Ka;
    // Kd *= texture( texSampler, texCoord ).xyz;
    color += lightColor * Kd * max( 0.0, dot( l, n ) );
    if ( dot( l, n ) > EPSILON )
    {
        // color += lightColor * Ks * pow( max( dot( reflect( -l, n ), e ), 0.0 ), specular );
        color += lightColor * Ks * pow( max( dot( h, n ), 0.0 ), 4*specular );
    }
        
    // outColor = vec4( e, 1.0 );
    outColor = vec4( color, 1.0 );
    
    // outColor = vec4( fragTexCoord, 0, 1.0 );
    // outColor = texture( texSampler, texCoord );
}

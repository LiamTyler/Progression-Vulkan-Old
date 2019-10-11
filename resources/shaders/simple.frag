#version 450
#extension GL_ARB_separate_shader_objects : enable

#define EPSILON 0.00001

layout( location = 0 ) out vec4 outColor;

layout( location = 0 ) in vec3 fragColor;
//layout( location = 0 ) in vec3 fragPosInWorldSpace;
//layout( location = 1 ) in vec3 normalInWorldSpace;

void main()
{
    /*
    vec3 cameraPos = vec3( 0, 0, 0 );
    vec3 n = normalize( normalInWorldSpace );
    vec3 e = normalize( cameraPos - fragPosInWorldSpace );
    // outColor = vec4( fragColor, 1.0 );
    
    vec3 color = vec3( .2, .2, .2 );
    vec3 Kd    = vec3( 1, 0, 0 );
    vec3 Ks    = vec3( 1, 1, 1 );
    float specular = 50;
    
    vec3 lightDir   = vec3( 0, 0, -1 );
    vec3 lightColor = vec3( 1, 1, 1 );
    vec3 l = normalize( -lightDir );
    vec3 h = normalize( l + e );
    color += lightColor * Kd * max( 0.0, dot( l, n ) );
    //if ( dot( l, n ) > EPSILON )
    //    color += lightColor * Ks * pow( max( dot( h, n ), 0.0 ), 4*specular );
        
    outColor = vec4( n, 1.0 );
    */
    
    outColor = vec4( fragColor, 1.0 );
}

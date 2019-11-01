#include "core/math.hpp"
#include "core/lua.hpp"

void RegisterLuaFunctions_Math( lua_State* L )
{
    sol::state_view lua(L);

    sol::usertype< glm::vec2 > vec2_type = lua.new_usertype< glm::vec2 >( "vec2", sol::constructors< glm::vec2(), glm::vec2( float ), glm::vec2( const glm::vec2& ), glm::vec2( float, float ) >() );
    vec2_type["x"] = &glm::vec2::x;
	vec2_type["y"] = &glm::vec2::y;
    vec2_type.set_function( sol::meta_function::addition,       static_cast< glm::vec2 (*)( const glm::vec2&, const glm::vec2& ) >( &glm::operator+ ) );
    vec2_type.set_function( sol::meta_function::subtraction,    static_cast< glm::vec2 (*)( const glm::vec2&, const glm::vec2& ) >( &glm::operator- ) );
    vec2_type.set_function( sol::meta_function::multiplication, static_cast< glm::vec2 (*)( const glm::vec2&, const glm::vec2& ) >( &glm::operator* ) );
    vec2_type.set_function( sol::meta_function::division,       static_cast< glm::vec2 (*)( const glm::vec2&, float ) >( &glm::operator/ ) );
    vec2_type.set_function( sol::meta_function::unary_minus,    static_cast< glm::vec2 (*)( const glm::vec2& ) >( &glm::operator- ) );
    vec2_type.set_function( "scale",     static_cast< glm::vec2 (*)( float, const glm::vec2& ) >( &glm::operator* ) );
    vec2_type.set_function( "dot",       static_cast< float (*)( const glm::vec2&, const glm::vec2& ) >( &glm::dot ) );
    vec2_type.set_function( "length",    static_cast< float (*)( const glm::vec2& ) >( &glm::length ) );
    vec2_type.set_function( "normalize", static_cast< glm::vec2 (*)( const glm::vec2& ) >(&glm::normalize ) );

    sol::usertype< glm::vec3 > vec3_type = lua.new_usertype< glm::vec3 >( "vec3", sol::constructors< glm::vec3(), glm::vec3( float ), glm::vec3( const glm::vec3& ), glm::vec3( float, float, float ) >() );
    vec3_type["x"] = &glm::vec3::x;
	vec3_type["y"] = &glm::vec3::y;
	vec3_type["z"] = &glm::vec3::z;
    vec3_type.set_function( sol::meta_function::addition,       static_cast< glm::vec3 (*)( const glm::vec3&, const glm::vec3& ) >( &glm::operator+ ) );
    vec3_type.set_function( sol::meta_function::subtraction,    static_cast< glm::vec3 (*)( const glm::vec3&, const glm::vec3& ) >( &glm::operator- ) );
    vec3_type.set_function( sol::meta_function::multiplication, static_cast< glm::vec3 (*)( const glm::vec3&, const glm::vec3& ) >( &glm::operator* ) );
    vec3_type.set_function( sol::meta_function::division,       static_cast< glm::vec3 (*)( const glm::vec3&, float ) >( &glm::operator/ ) );
    vec3_type.set_function( sol::meta_function::unary_minus,    static_cast< glm::vec3 (*)( const glm::vec3& ) >( &glm::operator- ) );
    vec3_type.set_function( "scale",     static_cast< glm::vec3 (*)( float, const glm::vec3& ) >( &glm::operator* ) );
    vec3_type.set_function( "dot",       static_cast< float (*)( const glm::vec3&, const glm::vec3& ) >( &glm::dot ) );
    vec3_type.set_function( "cross",     static_cast< glm::vec3 (*)( const glm::vec3&, const glm::vec3& ) >( &glm::cross ) );
    vec3_type.set_function( "length",    static_cast< float (*)( const glm::vec3& ) >( &glm::length ) );
    vec3_type.set_function( "normalize", static_cast< glm::vec3 (*)( const glm::vec3& ) >(&glm::normalize ) );

    sol::usertype< glm::vec4 > vec4_type = lua.new_usertype< glm::vec4 >( "vec4", sol::constructors <glm::vec4(), glm::vec4( float ), glm::vec4( const glm::vec4& ), glm::vec4( float, float, float, float ) >() );
    vec4_type["x"] = &glm::vec4::x;
	vec4_type["y"] = &glm::vec4::y;
	vec4_type["z"] = &glm::vec4::z;
	vec4_type["w"] = &glm::vec4::z;
    vec4_type.set_function( sol::meta_function::addition,       static_cast< glm::vec4 (*)( const glm::vec4&, const glm::vec4& ) >( &glm::operator+ ) );
    vec4_type.set_function( sol::meta_function::subtraction,    static_cast< glm::vec4 (*)( const glm::vec4&, const glm::vec4& ) >( &glm::operator- ) );
    vec4_type.set_function( sol::meta_function::multiplication, static_cast< glm::vec4 (*)( const glm::vec4&, const glm::vec4& ) >( &glm::operator* ) );
    vec4_type.set_function( sol::meta_function::division,       static_cast< glm::vec4 (*)( const glm::vec4&, const float& ) >( &glm::operator/ ) );
    vec4_type.set_function( sol::meta_function::unary_minus,    static_cast< glm::vec4 (*)( const glm::vec4& ) >( &glm::operator- ) );
    vec4_type.set_function( "scale",     static_cast< glm::vec4 (*)( float, const glm::vec4& ) >( &glm::operator* ) );
    vec4_type.set_function( "dot",       static_cast< float (*)( const glm::vec4&, const glm::vec4& ) >( &glm::dot ) );
    vec4_type.set_function( "length",    static_cast< float (*)( const glm::vec4& ) >( &glm::length ) );
    vec4_type.set_function( "normalize", static_cast< glm::vec4 (*)( const glm::vec4& ) >(&glm::normalize ) );

    sol::usertype< glm::mat3 > mat3_type = lua.new_usertype< glm::mat3 >( "mat3", sol::constructors< glm::mat3(), glm::mat3( float ), glm::mat3( const glm::mat3& ) >() );
    mat3_type.set_function( sol::meta_function::index,          []( const glm::mat3& m, const int index ) { return m[index]; } );
    mat3_type.set_function( sol::meta_function::new_index,      []( glm::mat3& m, const int index, const glm::vec3& x ) { m[index] = x; } );
    mat3_type.set_function( sol::meta_function::addition,       static_cast< glm::mat3 (*)( const glm::mat3&, const glm::mat3& ) >( &glm::operator+ ) );
    mat3_type.set_function( sol::meta_function::subtraction,    static_cast< glm::mat3 (*)( const glm::mat3&, const glm::mat3& ) >( &glm::operator- ) );
    mat3_type.set_function( sol::meta_function::multiplication, static_cast< glm::vec3 (*)( float, const glm::vec3& ) >( &glm::operator* ) );
    mat3_type.set_function( sol::meta_function::division,       static_cast< glm::mat3 (*)( const glm::mat3&, float ) >( &glm::operator/ ) );
    mat3_type.set_function( sol::meta_function::unary_minus,    static_cast< glm::mat3 (*)( const glm::mat3& ) >( &glm::operator- ) );

    sol::usertype< glm::mat4 > mat4_type = lua.new_usertype< glm::mat4 >( "mat4", sol::constructors< glm::mat4(), glm::mat4( float ), glm::mat4( const glm::mat4& ) >() );
    mat4_type.set_function( sol::meta_function::index,          []( const glm::mat4& m, const int index ) { return m[index]; } );
    mat4_type.set_function( sol::meta_function::new_index,      []( glm::mat4& m, const int index, const glm::vec4& x ) { m[index] = x; } );
    mat4_type.set_function( sol::meta_function::addition,       static_cast< glm::mat4 (*)( const glm::mat4&, const glm::mat4& ) >( &glm::operator+ ) );
    mat4_type.set_function( sol::meta_function::subtraction,    static_cast< glm::mat4 (*)( const glm::mat4&, const glm::mat4& ) >( &glm::operator- ) );
    mat4_type.set_function( sol::meta_function::multiplication, static_cast< glm::vec3 (*)( float, const glm::vec3& ) >( &glm::operator* ) );
    mat4_type.set_function( sol::meta_function::division,       static_cast< glm::mat4 (*)( const glm::mat4&, const float& ) >( &glm::operator/ ) );
    mat4_type.set_function( sol::meta_function::unary_minus,    static_cast< glm::mat4 (*)( const glm::mat4& ) >( &glm::operator- ) );
}
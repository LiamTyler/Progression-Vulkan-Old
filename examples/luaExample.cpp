#include "progression.hpp"
#include <future>
#include <iomanip>
#include <thread>
#include <array>
#include "entt/entt.hpp"
#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"
#include <typeinfo>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

std::string demangle(const char* name) {

    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return (status==0) ? res.get() : name ;
}

#else

// does nothing if not g++
std::string demangle(const char* name) {
    return name;
}

#endif

template <class T>
std::string type(const T& t) {

    return demangle(typeid(t).name());
}

using namespace Progression;
using namespace Progression::Gfx;
// using namespace std;
// using namespace luabridge;

struct Position {
    float x;
    float y;

    Position( float a, float b ) : x(a), y(b) {}
};

struct Velocity {
    float x;
    float y;
    Velocity( float a, float b ) : x(a), y(b) {}
};

std::ostream& operator<<(std::ostream& out, const Position& p )
{
    return out << p.x << " " << p.y;
}

int main( int argc, char* argv[] )
{
    sol::state lua;

	lua.open_libraries(sol::lib::base);

	sol::usertype<Position> position_type = lua.new_usertype<Position>("Position", sol::constructors<Position( float, float )>());
    position_type["x"] = &Position::x;
	position_type["y"] = &Position::y;
	sol::usertype<Velocity> velocity_type = lua.new_usertype<Velocity>("Velocity", sol::constructors<Velocity( float, float )>());
    velocity_type["x"] = &Velocity::x;
	velocity_type["y"] = &Velocity::y;
    /*
	sol::usertype<glm::vec3> vec3_type = lua.new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(), glm::vec3( float ), glm::vec3( float, float, float )>());
    vec3_type["x"] = &glm::vec3::x;
	vec3_type["y"] = &glm::vec3::y;
	vec3_type["z"] = &glm::vec3::z;
    vec3_type.set_function( sol::meta_function::addition, (glm::vec3 (*)( const glm::vec3&, const glm::vec3& )) &glm::operator+);
    vec3_type.set_function( sol::meta_function::subtraction, (glm::vec3 (*)( const glm::vec3&, const glm::vec3& )) &glm::operator-);
    vec3_type.set_function( sol::meta_function::multiplication, (glm::vec3 (*)( const glm::vec3&, const glm::vec3& )) &glm::operator*);
    vec3_type.set_function( sol::meta_function::division, (glm::vec3 (*)( const glm::vec3&, float )) &glm::operator/);
    vec3_type.set_function( sol::meta_function::unary_minus, (glm::vec3 (*)(const glm::vec3&)) &glm::operator-);
    vec3_type.set_function( "scale", (glm::vec3 (*)( float, const glm::vec3& )) &glm::operator*);
    vec3_type.set_function( "dot", (float (*)( const glm::vec3&, const glm::vec3& )) &glm::dot);
    vec3_type.set_function( "cross", (glm::vec3 (*)( const glm::vec3&, const glm::vec3& )) &glm::cross);
    vec3_type.set_function( "length", (float (*)( const glm::vec3& )) &glm::length);
    vec3_type.set_function( "normalize", (glm::vec3 (*)( const glm::vec3& )) &glm::normalize);

    sol::usertype<glm::vec2> vec2_type = lua.new_usertype<glm::vec2>("vec2", sol::constructors<glm::vec2(), glm::vec2( float ), glm::vec2( float, float )>());
    vec2_type["x"] = &glm::vec2::x;
	vec2_type["y"] = &glm::vec2::y;
    vec2_type.set_function( sol::meta_function::addition, (glm::vec2 (*)( const glm::vec2&, const glm::vec2& )) &glm::operator+);
    vec2_type.set_function( sol::meta_function::subtraction, (glm::vec2 (*)( const glm::vec2&, const glm::vec2& )) &glm::operator-);
    vec2_type.set_function( sol::meta_function::multiplication, (glm::vec2 (*)( const glm::vec2&, const glm::vec2& )) &glm::operator*);
    vec2_type.set_function( sol::meta_function::division, (glm::vec2 (*)( const glm::vec2&, float )) &glm::operator/);
    vec2_type.set_function( sol::meta_function::unary_minus, (glm::vec2 (*)(const glm::vec2&)) &glm::operator-);
    vec2_type.set_function( "scale", (glm::vec2 (*)( float, const glm::vec2& )) &glm::operator*);
    vec2_type.set_function( "dot", (float (*)( const glm::vec2&, const glm::vec2& )) &glm::dot);
    vec2_type.set_function( "length", (float (*)( const glm::vec2& )) &glm::length);
    vec2_type.set_function( "normalize", (glm::vec2 (*)( const glm::vec2& )) &glm::normalize);

    sol::usertype<glm::vec4> vec4_type = lua.new_usertype<glm::vec4>("vec4", sol::constructors<glm::vec4(), glm::vec4( float ), glm::vec4( float, float, float, float )>());
    vec4_type["x"] = &glm::vec4::x;
	vec4_type["y"] = &glm::vec4::y;
	vec4_type["z"] = &glm::vec4::z;
	vec4_type["w"] = &glm::vec4::z;
    vec4_type.set_function( sol::meta_function::addition, (glm::vec4 (*)( const glm::vec4&, const glm::vec4& )) &glm::operator+);
    vec4_type.set_function( sol::meta_function::subtraction, (glm::vec4 (*)( const glm::vec4&, const glm::vec4& )) &glm::operator-);
    vec4_type.set_function( sol::meta_function::multiplication, (glm::vec4 (*)( const glm::vec4&, const glm::vec4& )) &glm::operator*);
    vec4_type.set_function( sol::meta_function::division, (glm::vec4 (*)( const glm::vec4&, const float& )) &glm::operator/);
    vec4_type.set_function( sol::meta_function::unary_minus, (glm::vec4 (*)(const glm::vec4&)) &glm::operator-);
    vec4_type.set_function( "scale", (glm::vec4 (*)( float, const glm::vec4& )) &glm::operator*);
    vec4_type.set_function( "dot", (float (*)( const glm::vec4&, const glm::vec4& )) &glm::dot);
    vec4_type.set_function( "length", (float (*)( const glm::vec4& )) &glm::length);
    vec4_type.set_function( "normalize", (glm::vec4 (*)( const glm::vec4& )) &glm::normalize);

    sol::usertype<glm::mat3> mat3_type = lua.new_usertype<glm::mat3>("mat3", sol::constructors<glm::mat3(), glm::mat3( float )>());
    mat3_type.set_function( sol::meta_function::index, [](const glm::mat3& m, const int index ) { return m[index]; } );
    mat3_type.set_function( sol::meta_function::new_index, []( glm::mat3& m, const int index, const glm::vec3& x) { m[index] = x; } );
    mat3_type.set_function( sol::meta_function::addition, (glm::mat3 (*)( const glm::mat3&, const glm::mat3& )) &glm::operator+);
    mat3_type.set_function( sol::meta_function::subtraction, (glm::mat3 (*)( const glm::mat3&, const glm::mat3& )) &glm::operator-);
    mat3_type.set_function( sol::meta_function::multiplication, (glm::vec3 (*)( float, const glm::vec3& )) &glm::operator*);
    mat3_type.set_function( sol::meta_function::division, (glm::mat3 (*)( const glm::mat3&, float )) &glm::operator/);
    mat3_type.set_function( sol::meta_function::unary_minus, (glm::mat3 (*)(const glm::mat3&)) &glm::operator-);

    sol::usertype<glm::mat4> mat4_type = lua.new_usertype<glm::mat4>("mat4", sol::constructors<glm::mat4(), glm::mat4( float )>());
    mat4_type.set_function( sol::meta_function::index, [](const glm::mat4& m, const int index ) { return m[index]; } );
    mat4_type.set_function( sol::meta_function::new_index, []( glm::mat4& m, const int index, const glm::vec4& x) { m[index] = x; } );
    mat4_type.set_function( sol::meta_function::addition, (glm::mat4 (*)( const glm::mat4&, const glm::mat4& )) &glm::operator+);
    mat4_type.set_function( sol::meta_function::subtraction, (glm::mat4 (*)( const glm::mat4&, const glm::mat4& )) &glm::operator-);
    mat4_type.set_function( sol::meta_function::multiplication, (glm::vec4 (*)( float, const glm::vec4& )) &glm::operator*);
    mat4_type.set_function( sol::meta_function::division, (glm::mat4 (*)( const glm::mat4&, const float& )) &glm::operator/);
    mat4_type.set_function( sol::meta_function::unary_minus, (glm::mat4 (*)(const glm::mat4&)) &glm::operator-);
    */
    
    entt::registry registry;

    sol::usertype<entt::registry> reg_type = lua.new_usertype<entt::registry>("registry");
    reg_type.set_function( "create", (entt::entity(entt::registry::*)() )&entt::registry::create );
    // auto fp = static_cast< glm::vec3&(entt::registry::*)(entt::entity, float)>(&entt::registry::assign<glm::vec3, float>); // no gcc
    // reg_type.set_function( "assignPosition", &entt::registry::assign<glm::vec3, float, float, float> ); // works on gcc 
    //
    reg_type.set_function( "assignPosition", static_cast<Position&(entt::registry::*)(const entt::entity, float&&, float&&)> (&entt::registry::assign<Position, float, float> ) ); // works on gcc
    reg_type.set_function( "removePosition", &entt::registry::remove<Position> );
    reg_type.set_function( "getPosition", static_cast<glm::vec3&(entt::registry::*)(entt::entity)>( &entt::registry::get<glm::vec3> ) );
    reg_type.set_function( "try_getPosition", static_cast<glm::vec3*(entt::registry::*)(entt::entity)>( &entt::registry::try_get<glm::vec3> ) );
    reg_type.set_function( "assignVelocity", static_cast<Velocity&(entt::registry::*)(const entt::entity, float&&, float&&)> (&entt::registry::assign<Velocity, float, float> ) ); // works on gcc
    reg_type.set_function( "removeVelocity", &entt::registry::remove<Velocity> );
    reg_type.set_function( "getVelocity", static_cast<Velocity&(entt::registry::*)(entt::entity)>( &entt::registry::get<Velocity> ) );
    reg_type.set_function( "try_getVelocity", static_cast<Velocity*(entt::registry::*)(entt::entity)>( &entt::registry::try_get<Velocity> ) );

    using F = std::function<void(Position&)>;

    auto tp = registry.view<Position>();
    std::cout << typeid(decltype(tp)).name() << std::endl;
    std::cout << type(tp) << std::endl;
    std::cout << typeid(registry).name() << std::endl;
    std::cout << type(registry) << std::endl;
    using posGroup_t = entt::basic_view<entt::entity, entt::exclude_t<>, Position>;
    using SIG = entt::basic_view<entt::entity, entt::exclude_t<>, Position>(entt::registry::*)();
    auto FP = (entt::basic_view<entt::entity, entt::exclude_t<>, Position>(entt::registry::*)()) &entt::registry::view<entt::exclude_t<>, Position>;
    // auto FP = &entt::registry::view<entt::exclude_t<>, Position>;
    // auto each = tp)::each<std::function<void(Position&)>>;
    auto pos_view = lua.new_usertype< posGroup_t >( "viewPosition" );
    // reg_type.set_function( "viewPosition", (posGroup_t(entt::registry::*)())entt::registry::view<entt::entity, Position> );
    // reg_type.set_function( "viewPosition", &entt::registry::view<entt::entity, entt::exclude_t<>, Position> );
    reg_type.set_function( "eachPosition", &posGroup_t::each<F> );
    lua["reg"] = &registry;
    // registry.assign<glm::vec3( e, 1.0f );
    lua.script_file( PG_ROOT_DIR "script.lua");

    std::cout << std::endl << "Entities: " << std::endl;
    registry.view<Position>().each([](auto& pos)
    {
        std::cout << "entity " << pos << std::endl;
    });
    std::cout << std::endl;

    registry.view<Position, Velocity>().each( lua["fn"] );


    std::cout << std::endl << "Entities: " << std::endl;
    registry.view<Position>().each([](auto& pos)
    {
        std::cout << "entity " << pos << std::endl;
    });
    std::cout << std::endl;
    /*
    for(auto i = 0; i < 4; ++i )
    {
        auto entity = registry.create();
        registry.assign< position >( entity, i * 1.f, i * 1.f );
        if( i % 2 == 0 )
        {
            registry.assign< velocity >( entity, 1.0f, 0.0f );
        }
    }
    */

    /*
    Time::Reset();
    while ( true )
    {
        Time::StartFrame();
        std::cout << Time::DeltaTime() << std::endl;
        update( Time::DeltaTime(), registry );
        std::this_thread::sleep_for(std::chrono::milliseconds( 1000 ));

        int i = 0;
        
        registry.view<glm::vec3>().each([&i](auto& pos)
        {
            std::cout << "entity " << i++ << " " << pos.x << " " << pos.y << std::endl;
        });
        std::cout << std::endl;

        Time::EndFrame();
    }
    */
    

    return 0;
}

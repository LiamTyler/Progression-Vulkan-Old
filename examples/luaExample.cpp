#include "progression.hpp"
#include <future>
#include <iomanip>
#include <thread>
#include <array>
#include "entt/entt.hpp"
#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"


using namespace Progression;
using namespace Progression::Gfx;
// using namespace std;
// using namespace luabridge;

struct position {
    float x;
    float y;

    position( float a, float b ) : x(a), y(b) {}

    position operator+( const position& p )
    {
        return { x + p.x, y + p.y };
    }
};

struct velocity {
    float dx;
    float dy;
};

struct player {
public:
	int bullets;
	int speed;

	player()
		: player(3, 100) {

	}

	player(int ammo)
		: player(ammo, 100) {

	}

	player(int ammo, int hitpoints)
		: bullets(ammo), hp(hitpoints) {

	}

	void boost() {
		speed += 10;
	}

	bool shoot() {
		if (bullets < 1)
			return false;
		--bullets;
		return true;
	}

	void set_hp(int value) {
		hp = value;
	}

	int get_hp() const {
		return hp;
	}

private:
	int hp;
};

void update( float dt, entt::registry &registry) {
    registry.view<position, velocity>().each([dt](auto &pos, auto &vel) {
        // gets all the components of the view at once ...

        pos.x += vel.dx * dt;
        pos.y += vel.dy * dt;
    });
}

int main( int argc, char* argv[] )
{

    sol::state lua;

	lua.open_libraries(sol::lib::base);

	// note that you can set a 
	// userdata before you register a usertype,
	// and it will still carry 
	// the right metatable if you register it later

	// set a variable "p2" of type "player" with 0 ammo
	lua["p2"] = player(0);

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

	lua.script_file( PG_ROOT_DIR "script2.lua");
    
    
    entt::registry registry;


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

    Time::Reset();
    while ( true )
    {
        Time::StartFrame();
        cout << Time::DeltaTime() << endl;
        update( Time::DeltaTime(), registry );
        std::this_thread::sleep_for(std::chrono::milliseconds( 1000 ));

        int i = 0;
        registry.view<position>().each([&i](auto& pos)
        {
        cout << "entity " << i++ << " " << pos.x << " " << pos.y << endl;
        });
        cout << endl;

        Time::EndFrame();
    }
    */

    return 0;
}

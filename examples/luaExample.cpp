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
using namespace std;

struct position {
    float x;
    float y;
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
	lua.open_libraries( sol::lib::base );

	lua.script( "print('bark bark bark!')" );

    lua["p2"] = player(0);

	// make usertype metatable
	sol::usertype<player> player_type = lua.new_usertype<player>("player",
		// 3 constructors
		sol::constructors<player(), player(int), player(int, int)>());

	// typical member function that returns a variable
	player_type["shoot"] = &player::shoot;
	// typical member function
	player_type["boost"] = &player::boost;

	// gets or set the value using member variable syntax
	player_type["hp"] = sol::property(&player::get_hp, &player::set_hp);

	// read and write variable
	player_type["speed"] = &player::speed;
	// can only read from, not write to
	// .set(foo, bar) is the same as [foo] = bar;
	player_type.set("bullets", sol::readonly(&player::bullets));

	// lua.script_file("prelude_script.lua");
	// lua.script_file("player_script.lua");
    lua.script_file( PG_ROOT_DIR "player.lua");

    /*
    entt::registry registry;

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

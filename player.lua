p1 = player.new(2)

function player:brake ()
	self.speed = 0
	print("we hit the brakes!")
end

-- p2 is still here from being 
-- set with lua["p2"] = player(0); below
local p2shoots = p2:shoot()
assert(not p2shoots)
-- had 0 ammo
	
-- set variable property setter
p1.hp = 545
-- get variable through property unqualified_getter
print(p1.hp)
assert(p1.hp == 545)

local did_shoot_1 = p1:shoot()
print(did_shoot_1)
print(p1.bullets)
local did_shoot_2 = p1:shoot()
print(did_shoot_2)
print(p1.bullets)
local did_shoot_3 = p1:shoot()
print(did_shoot_3)
	
-- can read
print(p1.bullets)
-- would error: is a readonly variable, cannot write
-- p1.bullets = 20

p1:boost()
-- call the function we define at runtime from a Lua script
p1:brake()

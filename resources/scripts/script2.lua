function Start()
    print("In Start() for script2.lua")
end

val = 0
function P()
    val = val + 1
end

function Update()
    print( "entity: ", registry:get_NameComponent( entity ).name, ", val: ", val)
end
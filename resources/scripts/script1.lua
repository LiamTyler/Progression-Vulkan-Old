Script2Func = nil

function Start()
    print("In Start() for script.lua")
    if entity == 0 then
        e = GetEntityByName( registry, "entity2")
        scriptComp = registry:get_ScriptComponent( e )
        Script2Func = scriptComp:GetFunction( "single", "P" )
    end
end

function Update()
    if Input.GetKeyDown( Key.A ) and Script2Func then
        Script2Func()
    end
end


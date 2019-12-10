
theta = 0
vel = .1
position = nil
radius = 30

function Start()
    position = vec3.new( 0 )
end

function Update()
    theta = theta + Time.dt * vel
    position.y = radius * math.cos( theta );
    position.z = -radius * math.sin( theta );
    if math.abs( theta ) > math.pi / 5 then
        vel = vel * -1
    end
    scene.directionalLight.direction.z = math.sin( theta )
    scene.directionalLight.direction.y = -math.cos( theta )
end
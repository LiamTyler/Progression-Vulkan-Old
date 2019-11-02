
velocity = nil
moveSpeed = 3
turnSpeed = 0.002
maxAngle = 85 * 3.14159/180
camera = nil

function Start()
    camera = scene.camera
    velocity = vec3.new( 0 )
end

function Update()
    if Input.GetKeyDown( Key.A ) then
        velocity.x = -1
    end
    if Input.GetKeyDown( Key.D ) then
        velocity.x = 1
    end
    if Input.GetKeyDown( Key.W ) then
        velocity.z = 1
    end
    if Input.GetKeyDown( Key.S ) then
        velocity.z = -1
    end
    
    if Input.GetKeyUp( Key.A ) and velocity.x == -1 then
        velocity.x = 0
    end
    if Input.GetKeyUp( Key.D ) and velocity.x == 1 then
        velocity.x = 0
    end
    if Input.GetKeyUp( Key.W ) and velocity.z == 1 then
        velocity.z = 0
    end
    if Input.GetKeyUp( Key.S ) and velocity.z == -1 then
        velocity.z = 0
    end
    
    local dMouse    = -Input:GetMouseChange()
    local dRotation = vec3.new( dMouse.y, dMouse.x, 0 )

    camera.rotation   = camera.rotation + vec3.scale( turnSpeed, dRotation )
    camera.rotation.x = math.max( -maxAngle, math.min( maxAngle, camera.rotation.x ) );
    
    camera:UpdateOrientationVectors()
    
    local forward = camera:GetForwardDir()
    local right = camera:GetRightDir()
    local step = moveSpeed * Time.dt
    camera.position = camera.position + vec3.scale( velocity.z * step, forward ) + vec3.scale( velocity.x * step, right )
    camera:UpdateViewMatrix()
end
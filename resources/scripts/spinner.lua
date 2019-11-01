
theta = 0
scale = 0
transform = nil

function Start()
    transform = registry:get_Transform( entity )
    originalScale = vec3.new( transform.scale )
end

function Update()
    transform.rotation.y = transform.rotation.y + 2 * Time.dt
    transform.scale      = vec3.scale( 1 + .2 * math.sin( theta ) , originalScale );
    theta = theta + 3 * Time.dt
end
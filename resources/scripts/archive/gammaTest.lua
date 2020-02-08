function Start()
    model = ResourceManager.Get_Model( "cube" )
    gammas = { 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25 }
    for row,gamma in ipairs( gammas ) do
        for c=1,11 do
            e = registry:create()
            
            material = Material.NewShared()
            color = ( ( c - 1 ) / 10 ) ^ ( 1.0 / gamma )
            material.Kd = vec3.new( color )
            material.metallic = 1;
            material.roughness = color

            modelRenderer = registry:assign_ModelRenderer( e )
            modelRenderer.model = model
            modelRenderer.materials:add( material )
            
            transform = registry:assign_Transform( e )
            transform.position = vec3.new( -10 + 2*c, 8 - 2*row, -5 )
            --transform.scale = vec3.new( 2, 2, 2 )
        end
    end
end
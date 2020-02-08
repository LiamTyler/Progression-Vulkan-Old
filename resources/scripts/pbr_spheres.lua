
function Start()
    model = ResourceManager.Get_Model( "sphere" )
    nrRows    = 7;
    nrColumns = 7;
    spacing   = 2.5;
    
    for r=1,nrRows do
        for c=1,nrColumns do
            e = registry:create()
            
            material = Material.NewShared()
            material.Kd = vec3.new( 0.5, 0, 0 )
            material.metallic  = ( r - 1 ) / nrRows
            material.roughness = math.min( math.max( ( c - 1 ) / nrColumns, 0.05 ), 1.0 )

            modelRenderer = registry:assign_ModelRenderer( e )
            modelRenderer.model = model
            modelRenderer.materials:add( material )
            
            transform = registry:assign_Transform( e )
            transform.position = vec3.new( (c - (nrColumns / 2)) * spacing, (r - (nrRows / 2)) * spacing, 0 )
            --transform.scale = vec3.new( 2, 2, 2 )
        end
    end
end
e1 = reg:create()
reg:assignPosition( e1, 1, 2 )
reg:assignVelocity( e1, 1, 0 )
e2 = reg:create()
reg:assignPosition( e2, 4, 5 )
reg:assignVelocity( e2, -1, 0 )
e3 = reg:create()
reg:assignPosition( e3, 7, 8 )
reg:assignVelocity( e3, 0, 1 )

function fn( p, v )
    p.x = p.x + v.x;
    p.y = p.y + v.y
end

function f( p )
    p.x = p.x + 10;
end

v = reg:GetPosView( reg )

--v.each(f)

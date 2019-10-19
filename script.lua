function f( p )
    p.x = p.x + 10;
end

reg:view_Position( reg ):each( f )

v = vec3.new( 0, 1, 3)
print( v )

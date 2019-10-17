e3 = reg:create()
reg:assignPosition( e3, 7, 8, 9 )
reg:removePosition( e2 )
p = reg:getPosition( e1 )
print("P: ", p)
p.x = 100
e3 = reg:create()
reg:assignPosition( e3, 7, 8, 9 )
p = reg:try_getPosition( e2 )
if p then
    print("yos: ", p)
else
    print("nos")
end
reg:removePosition( e2 )
p = reg:try_getPosition( e2 )
if p then
    print("yos: ", p)
else
    print("nos")
end
p = reg:getPosition( e1 )
print("P: ", p)
p.x = 100

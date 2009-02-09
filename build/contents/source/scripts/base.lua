--default handling
function eventHandler (instance, box)
	--log (instance..box)
	--flashMessage(instance..":"..box)

	if box=="shoptruck" then
		showChooser("vehicle", instance, "spawnzone")
	elseif box=="shopboat" then
		showChooser("boat", instance, "spawnzone")
	elseif box=="shopplane" then
		showChooser("airplane", instance, "spawnzone")
	elseif box=="shopextension" then
		showChooser("extension", instance, "spawnzone")
	elseif box=="repair" then
		repairVehicle(instance, "spawnzone")
	end

end





-- some utilities
function explode(div, str)
	if (div=='') then return false end
	pos = 0
	arr = {}
	-- for each divider found
	for st,sp in function() return string.find(str,div,pos,true) end do
		table.insert(arr,string.sub(str,pos,st-1)) -- Attach chars left of current divider
		pos = sp + 1 -- Jump past current divider
	end
	table.insert(arr,string.sub(str,pos)) -- Attach chars right of last divider
	return arr
end



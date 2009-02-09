-- thanks to sweetman for contributing parts of the race store code

races = {}
racescount = 0
currentrace = ""
cache_file = getSetting("Cache Path") .. "race_scores_island.dat"
log("race system using scores file: "..cache_file)

function writeScore(racename,score)
	local f = io.open(cache_file, "a+")
	f:write(racename.."|"..score.."\n")
	f:close()
	log('race score file written')
end

function findScore(racename)
	local f = io.open(cache_file, "r")
	minval = 9999
	if not f == nil then
		for line in f:lines()
		do
			line_ar = explode('|', line)
			if line_ar[1] == racename or tonumber(line_ar[2]) < minval then
				minval = tonumber(rcdt[2])
			end
		end
		log('race score file read')
	else
		log('race score file not existant')
	end
	return minval
end

function setupRace(racename, positions, loop)
	for a,v in ipairs(positions) do
		if a == 1 then
			oname = 'chp-start'
		else
			oname = 'chp-checkpoint'
		end
		if a == table.getn(positions) and not loop then
			-- put the finish thing at the end, not a normal checkpoint, only on non-loop races
			oname = 'chp-start'		
		end
		if #v == 7 then
			oname = v[7]
		end
		iname = 'race_'..racename..'_'..a
		log("*creating object '"..oname.."' with name '"..iname.."'.")
		spawnObject(oname, iname, v[1],v[2],v[3], v[4],v[5],v[6], raceEventHandler)
	end
	-- add the race finally
	races[racename] = {}
	races[racename]['besttime'] = findScore(racename)
	races[racename]['racename'] = racename
	races[racename]['positions'] = positions
	races[racename]['isloop'] = loop
	races[racename]['startTime'] = 0
	races[racename]['lastcheckpoint'] = 1
	races[racename]['checkpointspassed'] = 1
	races[racename]['numcheckpoints'] = table.getn(positions)
	if loop then
		races[racename]['goalnum'] = 1
	else
		races[racename]['goalnum'] = table.getn(positions)
	end
	
end

function getCheckPointFromName(instance)
	ar = explode('_', instance)
	return tonumber(ar[3])
end

function getRacenameFromName(instance)
	ar = explode('_', instance)
	return tostring(ar[2])
end

function setupArrow(position)
	--log("reposition arrow: "..position)
	txt = currentrace.."\ncheckpoint "..tostring(position).." / "..races[currentrace]['numcheckpoints']
	if position == 1 and races[currentrace]['isloop'] then
		txt = "Finish"
	end
	if position == -1 then
		-- hide it
		pointArrow(0, 0, 0, "")
	else
		data = races[currentrace]['positions'][position]
		if data then
			pointArrow(data[1], data[2], data[3], txt)
		else
			pointArrow(0, 0, 0, "")
		end
	end
end

function raceEventHandler(instance, box)
	--log("racehandler: "..instance.." - "..box)
	checknum = getCheckPointFromName(instance)
	racename = getRacenameFromName(instance)
	
	--log("just passed checkpoint number "..checknum.." of race "..racename)

	-- create an array with all required names in it
	if currentrace == "" then
		-- not racing currently
		if checknum == 1 then
			-- start of the race
			currentrace = racename
			--log(races[currentrace]['racename'])
			races[currentrace]['startTime'] = getTime()
			races[racename]['lastcheckpoint'] = 1
			races[currentrace]['checkpointspassed'] = 1
			setupArrow(2)
			startTimer()
			flashMessage("Race "..racename.." started!", 4)
		--else
			-- passed some not-start checkpoint
			--flashMessage("This is checkpoint "..checknum.." of the Race "..racename.."!", 4)
		end
	elseif currentrace == racename then
		--log(checknum.."###"..races[currentrace]['numcheckpoints'].."###"..races[currentrace]['lastcheckpoint'].."###"..tostring(races[currentrace]['isloop']))
		--log("passed: "..races[currentrace]['checkpointspassed'])
		-- we hit a checkpoint from the same race!
		if checknum == races[currentrace]['lastcheckpoint'] + 1 then
			-- racing, correct checkpoint!
			if checknum == races[currentrace]['numcheckpoints'] and races[currentrace]['isloop'] then
				setupArrow(1)
			else
				setupArrow(checknum+1)
			end
			races[currentrace]['lastcheckpoint'] = checknum
			races[currentrace]['checkpointspassed'] = races[currentrace]['checkpointspassed'] + 1
			time = getTime() - races[currentrace]['startTime']
			msg = "Passed checkpoint "..checknum.." of "..(races[currentrace]['numcheckpoints']).." after "..string.format("%.3f", time).." seconds."
			flashMessage(msg, 4)
		else
			if not checknum == races[currentrace]['lastcheckpoint'] then
				-- racing, wrong checkpoint, prevent trigger of the same checkpoint twice
				flashMessage("Wrong gate! You must find and pass Checkpoint "..races[currentrace]['lastcheckpoint'] + 1, 4)
			end
		end
		if checknum == races[currentrace]['goalnum'] and races[currentrace]['checkpointspassed'] == races[racename]['numcheckpoints'] then
			-- racing, reached finish
			--time = getTime() - starttime
			time = stopTimer()
			msg = "Finished! You needed "..string.format("%.3f", time).." seconds!"

			if races[currentrace]['besttime'] == 9999 or time < tonumber(races[currentrace]['besttime']) then
				races[currentrace]['besttime'] = time
				writeScore(races[currentrace]['racename'], time)
				msg = msg .. "\nNew Best Lap Time!"
			end
			msg = msg .. "\n (Best Lap: "..string.format("%.3f", races[currentrace]['besttime'])..")"
			
			
			setupArrow(-1)
			-- reset some values
			races[currentrace]['checkpointspassed'] = 0
			
			currentrace = ""
			-- flash finish message for 10 seconds
			flashMessage(msg, 10)
		end
	else
		-- flashMessage("This Checkpoint belongs to race"..racename.."!", 4)
	end

end

race_tothetop = {
{791.551, 87.6567, 1558.25, 0, 35.3333, 0},
{970.905, 119.2, 1421.01, 0, -93.64700, 0},
{1350.28, 157.644, 1375.19, 0, -44.40600},
{1506.18, 180.185, 1309.12, 0, 29.4644, 0},
{1306.9, 209.358, 1250.37, 0, 70.3679, 0},
{940.346, 236.788, 926.603, 0, 16.9539, 0}
}

function init()
	setupRace("ToTheTop", race_tothetop, false)
	log("island.terrn.lua file loaded")	
end

init()
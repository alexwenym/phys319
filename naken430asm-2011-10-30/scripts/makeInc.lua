-- makeInc.lua
-- reads all .cmd and .h files in directory tiInclude, and converts .cmd and .h files to .inc files for naken430asm
-- The naken430asm .inc files are put in directory nakenInclude.
-- tested using ubuntu 10.04
-- ToDo: extract the /* comments */ from the input files and include them in the output files.

require("lfs") -- get this using synaptic if it is not found.

local function trimEnds(s) -- trims spaces from the ends
    return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

local function tokenize(str) -- tokenizes a line, keeps punctuation. 
	local tokens = {}
	local notes
	local index
	local toks
	local str = trimEnds(str) -- removes white space from both ends
	if #str == 0 then -- blank line
		return tokens	-- an empty list is not no list (Rich Hinkey, clojure).
	end
	index = string.find(str,"//*") -- skip comment lines
	if index then
		notes = string.sub(str,index)
		str = string.sub(str,1,index-1)
	end
	if string.find(str,"*",1) then -- skip lines starting with "*"
		return tokens
	end
	for tok in str:gmatch("[%w%p]+") do table.insert(tokens, tok) end -- word or punctuation
	return tokens -- {} for blank line
end -- function tokenize(str)

local function Parse(inFile,outFile)
	local tokens = {}
	local tok = ""
	local lineNr = 0;
	local start,fin
	print("outFile =",outFile)
	io.output(outFile)	-- comment this out to get output to console
	
	-- parse the TI .cmd file
	for line in io.lines(inFile,"r") do
		lineNr = lineNr + 1
		repeat
			tokens = tokenize(line)
			--print(#tokens)
			if #tokens == 0 then -- skip blank lines
				--io.write("\n")
			else
				tok = tokens[1]
				if string.find(tok,"/",1) then 
					io.write(line)
					io.write("\n") 
				elseif string.find(tok,"*",1) then 
					io.write(line) 
					io.write("\n")
				else
					local index = string.find(tokens[3],";")
					if index and index > 1 then
					 tokens[3] = string.sub(tokens[3],1,index-1)
					end
					io.write(string.format("%-12s %3s %s",tokens[1],"equ",tokens[3]))
					io.write("\n")
				end
			end
		until true
	end -- parse .cmd file
	
	-- now parse the TI .h file and add the results to the outFile
	start,fin = string.find(inFile,".cmd") -- get the new input file name; replace .cmd with .h
	inFile = string.sub(inFile,1,start-1) .. ".h"
	--print("inFile for .h =",inFile)
	lineNr = 0
	for line in io.lines(inFile,"r") do
		--io.write(lineNr,line)
		--io.write("\n")
		lineNr = lineNr + 1
		repeat
			tokens = tokenize(line)
			--print(#tokens)
			if #tokens < 3 then -- skip blank lines or lines that are too short
				--io.write("\n")
			else
				--print(string.sub(tokens[3],1,1))
				if tokens[1]  == "#define" and string.sub(tokens[3],1,1) == "(" 
				and string.sub(tokens[3],#tokens[3]) == ")" then
					tok = string.sub(tokens[3],2,#tokens[3]-1)
					if #tokens > 3 then notes = tokens[4] else notes = "" end
					--line = tokens[1] .. " " .. tokens[2] .. " " .. tok .. " " .. notes
					--print(string.format("%7s %-16s %12s    %s\n",tokens[1],tokens[2],tok,notes))
					io.write(string.format("%7s %-16s %12s    %s\n",tokens[1],tokens[2],tok,notes))
					--io.write(line) 
					--io.write("\n")
				end
			end
		until true
	end
	io.close()
end -- Parse()

path = lfs.currentdir() .. [[/tiInclude]]
inputFile = path .. [[/msp430g2553.cmd]]
outPath = lfs.currentdir() .. [[/nakenInclude]]

-- get all the .cmd files
cmdFiles = {}
for file in lfs.dir(path) do
	start,fin = string.find(file,"msp430")
	if start == 1 then -- file name starts with "msp430"
		start,fin = string.find(file,".cmd")
		if start and (start == (#file-3)) then cmdFiles[file] = "" end
	end
end

--for file,_ in pairs(cmdFiles) do print(file) end

-- get all the .h files

hFiles = {}
for file in lfs.dir(path) do
	start,fin = string.find(file,"msp430")
	if start ==1 then
		start,fin = string.find(file,".h")
		if start and (start == (#file-1)) then hFiles[file] = "" end
	end
end

--for file,_ in pairs(hFiles) do print(file) end

--now make sure there is a .h file for each .cmd file

for file,_ in pairs(cmdFiles) do
	test = string.sub(file,1,#file-3) .. "h"
	--print(test)
	if not hFiles[test] then print("no pair for file",file) end
end

-- we now have a table with all the .cmd files that we need to process in table cmdFiles.
-- for each .cmd file there is also a .h file, in table hFiles.

-- here is code to translate just a single .cmd/.h file to a .inc file.
--for file,_ in pairs(cmdFiles) do print(file) end

--outFile = outPath .. [[/msp430g2553.inc]]	-- temp for testing
--Parse(inputFile,outFile)
	
---[[
for file,_ in pairs(cmdFiles) do
	start,fin  = string.find(file,".cmd")
	--print(file,start,fin)
	outFile = outPath .. [[/]] .. string.sub(file,1,start) .. "inc"
	fileName = path ..[[/]] .. file
	--print(fileName,outFile)
	Parse(fileName,outFile)
end
--]]
print("finished. " )

--io.stdout:write("ok to io.write\n")

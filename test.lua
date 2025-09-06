for pat in string.gmatch("/home/ouyangjun/code/github/onbnet/service/?.lua", "([^;]+);*") do
	local filename = string.gsub(pat, "?", "bootstrap")
	local f, msg = loadfile(filename)
	if not f then
		table.insert(err, msg)
	else
        print("loadfile " .. filename)
		pattern = pat
		main = f
		break
	end
end
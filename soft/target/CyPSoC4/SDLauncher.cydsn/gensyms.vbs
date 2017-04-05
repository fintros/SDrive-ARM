Do until WScript.Stdin.AtEndOfStream
	Dim line
	line = WScript.Stdin.ReadLine
	line = Replace(Replace(Replace(line, "  ", " "), "  ", " "), vbTab, " ")
	Dim items
	items = Split(line)
	WScript.Echo items(UBound(items)) & " = 0x" & items(0) & ";"
Loop

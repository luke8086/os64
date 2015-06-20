\ -------------------------------------------------
\ echo.nf - print the command line arguments (os64)
\ -------------------------------------------------

1 do
	dup argc <
while
	dup 1 != if
		" " printf drop
	then

	dup argv "%s" printf drop

	1 +
repeat

"\n" printf

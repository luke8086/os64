\ -------------------------------------
\ cat.nf - display file contents (os64)
\ -------------------------------------

\ global variables

32 "size" var
0 "buf" var
-1 "fd_in" var
-1 "fd_out" var
0 "count" var
"/dev/vt" "path_out" var

\ cleanup and exit

:
	buf 0 != if buf sys-free then
	fd_in 0 >= if fd_in sys-close drop then
	fd_out 0 >= if fd_out sys-close drop then
	sys-exit
; "cat-exit" def

\ check command line arguments

argc 2 < if
	"usage: cat.nf <filename>\n" printf drop
	-1 cat-exit
then	

\ allocate buffer

size sys-malloc "buf" :=
buf 0 == if -1 cat-exit then

\ open input file

1 argv sys-open "fd_in" :=
fd_in 0 < if -1 cat-exit then

\ open virtual terminal device

path_out sys-open "fd_out" :=
fd_out 0 < if -1 cat-exit then

\ copy from input file to terminal until end of file

do
	size buf fd_in sys-read "count" :=
	count 0 < if -1 cat-exit then
	count 0 == if 0 cat-exit then
	count buf fd_out sys-write drop
0 until

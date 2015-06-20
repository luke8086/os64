\ -------------------------------
\ ls.nf - list directory contents
\ -------------------------------

\ global variables

48 "file_info_size" var
12 "file_info_name_ofs" var
-1 "fd" var
0 "buf" var
0 "count" var

\ cleanup and exit

:
	buf 0 != if buf sys-free then
	fd -1 > if fd sys-close then
	sys-exit
; "ls-exit" def
	
\ check command line arguments

argc 2 < if
	"usage: ls.nf <dirname>\n" printf drop
	-1 ls-exit
then

\ allocate buffer

file_info_size sys-malloc "buf" :=
buf 0 == if -1 ls-exit then

\ open directory

1 argv sys-open "fd" :=
fd 0 < if -1 ls-exit then

\ FIXME: check if the file is a directory

\ iterate over directory entries

do
	file_info_size buf fd sys-read "count" :=
	count 0 == if 0 ls-exit then
	count file_info_size != if -1 ls-exit then
	buf file_info_name_ofs + "- %s\n" printf drop
0 until

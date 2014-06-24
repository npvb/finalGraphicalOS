as86 lib.asm -o lib_asm.o
bcc -ansi -c lib.c -o lib.o
bcc -ansi -c gshell.c -o gshell.o
ld86 -0 -d gshell.o lib.o lib_asm.o
mv a.out GSHELL
./loadFile GSHELL
mv GSHELL gshell

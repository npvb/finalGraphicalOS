as86 lib.asm -o lib_asm.o
bcc -ansi -c lib.c -o lib.o
bcc -ansi -c shell.c -o shell.o
ld86 -0 -d shell.o lib.o lib_asm.o
mv a.out SHELL
./loadFile SHELL
mv SHELL shell

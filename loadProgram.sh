as86 lib.asm -o lib_asm.o
bcc -ansi -c lib.c -o lib.o
bcc -ansi -c $1.c -o $1.o
ld86 -0 -d $1.o lib.o lib_asm.o
mv a.out $1
./loadFile $1

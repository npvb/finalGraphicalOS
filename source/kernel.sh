as86 kernel.asm -o kernel_asm.o
bcc -ansi -c kernel.c -o kernel.o
ld86 -0 -d kernel.o kernel_asm.o
mv a.out kernel
dd if=kernel of=floppya.img bs=512 conv=notrunc seek=3

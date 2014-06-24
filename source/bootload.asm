;bootload.asm
;Michael Black, December 2007
;
;This sits in the 0th sector of a 3 1/2 floppy.  It loads the kernel.
;The kernel is expected to start at sector 4 and be contiguous.
;This should be compiled with nasm into a binary.
;It should be exactly 512 bytes

	bits 16
KSEG	equ	0x1000		;kernel goes into memory at 0x10000
KSIZE	equ	12		;kernel is (currently) seven sectors (and growing!)
KSTART	equ	4		;kernel lives at sector 4 (makes room for map & dir)

	;boot loader starts at 0 in segment 0x7c00
	org 0h

	;make the data segment 0x7c00 (the code already is)
	mov ax,0x7c0
	mov ds,ax

	;print out boot loader message
	mov si,msg
	call print

	;let's put the kernel at KSEG:0
	mov ax,KSEG
	mov es,ax
	call readsector

	;set up the segments and stack
	mov ax,KSEG
	mov ds,ax
	mov ss,ax
	mov ax,0xfff0		;stack starts at 0xfff0 in the segment
	mov sp,ax
	mov bp,ax

	;jump directly to the kernel - this ends the boot loader
	jmp KSEG:0

;read sectors from the floppy and return
;this makes use of the BIOS interrupt 0x13
readsector:
        mov     cl,KSTART      	;cl holds sector number
        mov     dh,0     	;dh holds head number
        mov     ch,0     	;ch holds track number


        mov     ah,2            ;absolute disk read
        mov     al,KSIZE        ;read KSIZE sectors
        mov     dl,0            ;read from floppy disk A
        mov     ebx,0		;read into 0 (in the segment)
        int     13h

        ret

;print prints a line of text using BIOS interrupt 0x10
;the text should start at DS:SI and be terminated with 0x0
print:
	lodsb		;load al from [si]
	cmp al,0	;if al==0, we're done
	jz printdone
	mov ah,0xe
	mov bx,7
	int 10h		;print character
	jmp print
printdone:
	ret

;printhex is for debugging purposes.  it can be removed
;it prints out the contents of ax in hexadecimal
printhex:
        push bx
        push ax
        push ax
        push ax
        push ax
        mov al,ah
        mov ah,0xe
        mov bx,7
        shr al,4
        and al,0xf
        cmp al,0xa
        jb ph1
        add al,0x7
ph1:    add al,0x30
        int 10h

        pop ax
        mov al,ah
        mov ah,0xe
        and al,0xf
        cmp al,0xa
        jb ph2
        add al,0x7
ph2:    add al,0x30
        int 10h

        pop ax
        mov ah,0xe
        shr al,4
        and al,0xf
        cmp al,0xa
        jb ph3
        add al,0x7
ph3:    add al,0x30
        int 10h

        pop ax
        mov ah,0xe
        and al,0xf
        cmp al,0xa
        jb ph4
        add al,0x7
ph4:    add al,0x30
        int 10h

        pop ax
        pop bx
        ret

;the bootloader prints out this message
msg:	db 'Loading the kernel',0xd,0xa,0x0

;fill out the code to 512 bytes (including the next two bytes)
	times 510-($-$$) db 0

;if the boot loader doesn't end with AA55, the BIOS won't recognize it
	dw 0xAA55


;lib.asm
;Michael Black, 2007
;
;These provide assembly functions used by the shell
;and user applications

	.global _readc
	.global _printc
	.global _readsect
	.global _writesect
	.global _int21
	.global _seti
	.global _setvideographics
	.global _setvideotext
	.global _drawpixel
	.global _interrupt
	.global _putInMemory
	.global _printhex

;invoke int 21
;this can take an arbitrary number of parameters - extra parameters will just be garbage
;inputs: AH code (char), BX (int / address), CX (int / address), DX (int / address)
_int21:
	sti

        mov di,sp
        mov ah,[di+2]
        mov bx,[di+4]
        mov cx,[di+6]
        mov dx,[di+8]

        int 0x21

        ret

;read character.  character is read to al
_readc:
        mov ah,#0
        int 0x16
        mov ah,#0
        ret

;print character. character is located at sp+2
_printc:
        mov si,sp
        mov si,[si+2]
        mov ah,#0xe
        mov bx,7
        int 0x10
        ret

;do an absolute sector read
;takes 4 parameters: address, sector, head, track
_readsect:
	cli
        push    bp
        mov     bp,sp

        mov     bx,[bp+4]       ;bx holds the address of the buffer
        mov     cl,[bp+6]      ;cl holds sector number
        mov     dh,[bp+8]     ;dh holds head number
        mov     ch,[bp+10]     ;ch holds track number

        mov     ah,#2            ;absolute disk read
        mov     al,#1            ;read 1 sector
        mov     dl,#0            ;read from floppy disk A
        int     0x13

        mov     dl,al
        mov     ax,#0
        mov     al,dl

        pop     bp
	sti

        ret

;do an absolute sector write
;takes 4 parameters: address, sector, head, track
_writesect:
	cli
        push    bp
        mov     bp,sp

        mov     bx,[bp+4]       ;bx holds the address of the buffer
        mov     cl,[bp+6]      ;cl holds sector number
        mov     dh,[bp+8]     ;dh holds head number
        mov     ch,[bp+10]     ;ch holds track number

        mov     ah,#3            ;absolute disk write
        mov     al,#1            ;read 1 sector
        mov     dl,#0            ;read from floppy disk A
        int     0x13

        mov     dl,al
        mov     ax,#0
        mov     al,dl

        pop     bp

	sti

        ret

;set interrupts as enabled
;this is essential for multitasking, otherwise the timer won't go off
_seti:
	sti
	ret

;switch the video mode to 320x200 graphics
_setvideographics:
        mov ah,#0
        mov al,#0x13    ;320x200, 256 color
        int 0x10
        ret

;switch the video mode back to text
_setvideotext:
        mov ah,#0
        mov al,#0x3     ;text 80x25
        int 0x10
        ret

;set the color of a pixel
;this should be used only in graphics mode
_drawpixel:
        push bp
        push ds

        mov bp,sp
        mov di,[bp+8]
        mov ax,[bp+6]

        mov cx,#0xa000
        mov ds,cx
        mov [di],ax

        pop ds
        pop bp
        ret

;int interrupt (int number, int AX, int BX, int CX, int DX)
_interrupt:
        push bp
        mov bp,sp
        mov ax,[bp+4]   ;get the interrupt number in AL
        push ds         ;use self-modifying code to call the right interrupt
        mov bx,cs
        mov ds,bx
        mov si,#intr
        mov [si+1],al   ;change the 00 below to the contents of AL
        pop ds
        mov ax,[bp+6]   ;get the other parameters AX, BX, CX, and DX
        mov bx,[bp+8]
        mov cx,[bp+10]
        mov dx,[bp+12]

intr:   int #0x00       ;call the interrupt (00 will be changed above)

        mov ah,#0       ;we only want AL returned
        pop bp
        ret

;void putInMemory (int segment, int address, char character)
_putInMemory:
        push bp
        mov bp,sp
        push ds
        mov ax,[bp+4]
        mov si,[bp+6]
        mov cl,[bp+8]
        mov ds,ax
        mov [si],cl
        pop ds
        pop bp
        ret



;this is for debugging only
;prints out ax in hexadecimal
_printhex:
        push bx
        push ax
        push ax
        push ax
        push ax
        mov al,ah
        mov ah,#0xe
        mov bx,#7
        shr al,#4
        and al,#0xf
        cmp al,#0xa
        jb ph1
        add al,#0x7
ph1:    add al,#0x30
        int 0x10

        pop ax
        mov al,ah
        mov ah,#0xe
        and al,#0xf
        cmp al,#0xa
        jb ph2
        add al,#0x7
ph2:    add al,#0x30
        int 0x10

        pop ax
        mov ah,#0xe
        shr al,#4
        and al,#0xf
        cmp al,#0xa
        jb ph3
        add al,#0x7
ph3:    add al,#0x30
        int 0x10

        pop ax
        mov ah,#0xe
        and al,#0xf
        cmp al,#0xa
        jb ph4
        add al,#0x7
ph4:    add al,#0x30
        int 0x10

        pop ax
        pop bx
        ret


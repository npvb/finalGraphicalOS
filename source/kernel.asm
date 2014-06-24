;kernel.asm
;Michael Black, 2007
;
;This file holds the assembly language functions called by kernel.c
;It should be compiled using as86

;these flags tell how often the timer interrupt should go off.
;as infrequently as possible
TIMER_LOW equ 0xff
TIMER_HIGH equ 0xff
;a process switch happens every TIMER_MAX timer interrupts
TIMER_MAX equ 1

	.global _bios_printstr
	.global _terminate
	.global _int21
	.global _makeinterrupt21
	.global _maketimerinterrupt
	.global _timer_restore
	.global _readsector
	.global _writesector
	.global _loadprogram
	.global _launchprogram
	.global	_printhex
	.global _readchar
	.global _printchar
	.global _setdatasegkernel
	.global _restoredataseg
	.global _getprocessid
	.global _printtop
	.extern _main
	.extern _handleinterrupt21
	.extern _handletimerinterrupt

;read character.  character is read to al
_readchar:
	mov ah,#0
	int 0x16
	mov ah,#0
	ret

;print character. character is located at sp+2
_printchar:
	mov si,sp
	mov si,[si+2]
	mov ah,#0xe
	mov bx,7
	int 0x10
	ret

;this print string is only ever called by the kernel - it uses the kernel's segment
_bios_printstr:
	mov di,sp
	mov si,[di+2]

print:
	mov al,[si]
	inc si

	cmp al,#0
	jz done

	mov ah,#0xe
	mov bx,#7
	int 0x10

	jmp print

done:
	ret

;just hangs up by busy waiting.
;called when waiting for the timer to set another process running
_terminate:
	sti
_there:	jmp _there

;sets up the timer's ISR
_maketimerinterrupt:
	cli
	mov dx,#timer_ISR ;get address of timerISR in dx

	push ds
	mov ax,#0	;interrupts are at lowest memory
	mov ds,ax
	mov si,#0x20	;timer interrupt vector (8 * 4)
	mov ax,cs	;have interrupt go to the current segment
	mov [si+2],ax
	mov [si],dx	;address of our vector
	pop ds


	;start the timer
	mov al,#0x36
	out #0x43,al
	mov ax,#TIMER_LOW
	out #0x40,al
	mov ax,#TIMER_HIGH
	out #0x40,al

	ret

;sets up the int 0x21 ISR
_makeinterrupt21:
	mov dx,#int21_ISR	;get address of int21 in dx

	push ds
	mov ax,#0	;interrupts are at lowest memory
	mov ds,ax
	mov si,#0x84	;interrupt 21 vector

	mov ax,cs	;have interrupt go to the current segment
	mov [si+2],ax
	mov [si],dx	;address of our vector

	pop ds

	ret

;sets the data segment to the kernel, saving the current ds on the stack
_setdatasegkernel:
	pop bx
	push ds
	push bx
	mov ax,#0x1000
	mov ds,ax
	ret

;restores the data segment
_restoredataseg:
	pop bx
	pop ds
	push bx
	ret

;returns the process id number (take the contents of ss and shift)
_getprocessid:
	mov ax,ss
	mov al,ah
	mov ah,#0
	shr al,4
	dec al
	ret

;holds the number of timer interrupts until the next process switch
count:	dw	TIMER_MAX

;this routine runs on timer interrupts
timer_ISR:
	
	;disable interrupts
	cli

	;save all regs for the old process on the old process's stack
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	push ax
	push ds
	push es

	;reset interrupt controller so it performs more interrupts
	mov al,#0x20
	out #0x20,al

	;check: do we want to do a process switch now?
	;get the count's segment
	push ds
	mov ax,#0x1000
	mov ds,ax
	mov ax,count
	pop ds

	;yes, time to switch
	cmp ax,#0
	jz doswitch

	;no? just return
	dec ax
	;decrement the count
	push ds
	mov bx,#0x1000
	mov ds,bx
	mov count,ax
	pop ds

	;restore the state
	pop es
	pop ds
	pop ax
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	sti
	iret

;do a process switch
doswitch:

	;get the segment (ss) and the stack pointer (sp) - we need to keep these
	mov bx,ss
	mov cx,sp

	;set all segments to the kernel
	mov ax,#0x1000
	mov ds,ax
	mov es,ax
	mov ss,ax
	;set the kernel's stack
	mov ax,#0xdff0
	mov sp,ax
	mov bp,ax

	;set the count back to the max
	mov ax,#TIMER_MAX
	mov count,ax

	;call handle interrupt with 2 parameters: the segment, the stack pointer.
	push cx
	push bx
	call _handletimerinterrupt

;returns from a timer interrupt to a different process
_timer_restore:
	;pop off the local return address - don't need it
	pop ax
	;get the segment and stack pointer
	pop bx
	pop cx

	;get rid of the junk from the two calls and no returns
	pop ax
	pop ax
	pop ax
	pop ax
	pop ax
	pop ax
	pop ax

	;set up the stack
	mov sp,cx
	;set up the stack segment
	mov ss,bx

	;now we're back to the program's area
	;reload the registers (if this is it's first time running, these will be zeros)
	pop es
	pop ds
	pop ax
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx

	;enable interrupts and return
	sti
	iret

;this is called immediately on an interrupt 0x21.
int21_ISR:
	push ds

	;let's call a C interrupt handler
	;pass it the contents of ah - this tells which interrupt was called
	;pass it the contents of bx,cx,dx - the parameters
	mov al,ah
	mov ah,#0

	push dx
	push cx
	push bx
	push ax
	call _handleinterrupt21
	pop ax
	pop bx
	pop cx
	pop dx

	pop ds

	iret

;invoke int 21
;this can take an arbitrary number of parameters - extra parameters will just be garbage
;inputs: AH code (char), BX (int / address), CX (int / address), DX (int / address)
_int21:
	mov di,sp
	mov ah,[di+2]
	mov bx,[di+4]
	mov cx,[di+6]
	mov dx,[di+8]

	int 0x21

	ret

;this loads a new program but doesn't start it running
;the scheduler will take care of that
;the program will be located at the beginning of the segment at [sp+2]
;the filebuffer is located in the current segment at [sp+4]
;the number of bytes is located at [sp+6]
_loadprogram:
	;bx=new segment
	;cx=length
	;si=buffer
	;di=new location
	mov 	bp,sp
	mov	bx,[bp+2]
	mov	si,[bp+4]
	mov	cx,[bp+6]
	mov	di,#0

;copy the program to the new segment
ldtransfer:
	cmp	cx,#0
	jz	ldtransferdone
	mov	al,[si]
	push	ds
	mov	ds,bx
	mov	[di],al

	pop	ds
	inc	si
	inc	di
	dec	cx
	jmp	ldtransfer

ldtransferdone:
;make a stack image so that the timer interrupt can start this program

	;save the caller's stack pointer and segment
	mov	cx,sp
	mov	dx,ss
	mov	ax,#0xff18	;this allows an initial sp of 0xff00
	mov	sp,ax
	mov	ss,bx


	mov	ax,#0	;IP
	push	ax
	mov	ax,bx	;CS
	push	ax
	mov	ax,#0x0		;a normal flag setting
	push	ax
	mov	ax,#0		;set all the general registers to 0
	push	ax	;bx
	push	ax	;cx
	push	ax	;dx
	push	ax	;si
	push	ax	;di
	push	ax	;bp
	push	ax	;ax
	mov	ax,bx
	push	ax	;ds
	push	ax	;es

	;restore the stack to the caller
	mov	sp,cx
	mov	ss,dx
	ret

;_launchprogram currently isn't used.
;it starts a program immediately without using the timer

;this launches a new program
;the number of bytes is located at [sp+6]
;the filebuffer is located in the current segment at [sp+4]
;the program will be located at the beginning of the segment at [sp+2]
_launchprogram:
	;bx=new segment
	;cx=length
	;si=buffer
	;di=new location
	mov 	bp,sp
	mov	bx,[bp+2]
	mov	si,[bp+4]
	mov	cx,[bp+6]
	mov	di,#0

;transfer the program from buffer to the new segment
lptransfer:
	cmp	cx,#0
	jz	lptransferdone
	mov	al,[si]
	push	ds
	mov	ds,bx
	mov	[di],al
	pop	ds
	inc	si
	inc	di
	dec	cx
	jmp	lptransfer

lptransferdone:
	;set the segment in the jump instruction below to bx
	;self modifying code!
        push    ds
        mov     ax,cs
        mov     ds,ax
        mov     si,#jump
        mov     [si+3],bx
        pop     ds

	;set up the segments
	mov	ds,bx
	mov	ss,bx
	mov	es,bx

	;set up the stack
	mov	sp,#0xfff0
	mov	bp,#0xfff0

	;enable interrupts
	sti

	;and jump to the program
	;(note that the first 0x0000 is just a placeholder)
jump:	jmp	#0x0000:0x0000



;do an absolute sector read
;takes 4 parameters: address, sector, head, track
_readsector:
        push    bp
        mov     bp,sp

	mov	bx,[bp+4]	;bx holds the address of the buffer
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

        ret

;do an absolute sector write
;takes 4 parameters: address, sector, head, track
_writesector:
        push    bp
        mov     bp,sp

	mov	bx,[bp+4]	;bx holds the address of the buffer
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

        ret

;prints a character at the top of the screen
;this is used to print out the active processes
_printtop:
	push bp
	push ax
	push bx
	push ds
	push si
	
	mov bp,sp
	mov bl,[bp+12]
	mov si,[bp+14]
	shl si,1

	mov ax,#0xb800
	mov ds,ax
	mov [si],bl
	mov bl,0x100
	mov [si+1],bl

	pop si
	pop ds
	pop bx
	pop ax
	pop bp

	ret

;printhex is used for debugging only
;it prints out the contents of ax in hexadecimal
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

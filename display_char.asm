! Routine to display characters
! =============================

! Note: bitmap = 1 bit/pixel. Bitmap byte= 1 line in a character
!       pixmap = word of 4 pixels (each nibble in word= pixel color) 
! Test (main function)
org x0000

@infinite
entr @colors
entr x020D     
stw ; nop
entr @ascii_char
entr x0020        ! Display 'A'
stw ; nop 
entr @xy          ! Display at y=8, x=0
entr x0800
stw ; nop

@loopm 
call x0100

entr @xy
dup ; gtw
inc ; stl
clh ; nop
comp x0018 
jmpz @next_y
jump @next_char
@next_y
entr @xy
dup ; gtw
cll; nop
entr x0100
add ; swp
drp ; stw

@next_char         ! Prepare to display next char
entr @ascii_char
dup ; gtw 
inc ; stw 
comp x0080
jpnz @loopm 


jump @infinite


! Routine to display one character
! --------------------------------

org x0100
! Variables
@xy=x1900             ! high-byte: y (0 to 15), low-byte: x (0 to 23)
@p_vram=x1901         ! VRAM pointer
@ascii_char=x1902     ! ASCII code of character to display
@p_bmp=x1903          ! character bitmap pointer
@c1=x1904             ! counters
@c2=x1905
@c3=x1906
@colors=x1907         !  high-byte: ink, low-byte: paper

@disp_char

! Find starting address in VRAM where character has to be put
entr @xy
gth ; cll
dup ; ccf    ! multiply y by 8*48=384
rrw ; add
entr @xy
gtl ; clh    ! add 2*x
ccf ; rlw
add ; nop
entr @p_vram
swp ; stw

! Find address of character bitmap
entr @ascii_char
gtl ; clh
ccf ; rlw    ! multiply by 4 (4 16-bit words per character bitmap)
rlw ; nop 
entr x1e00   ! bitmap of character set starts at x1e80 (for char=32)
add ; nop
entr @p_bmp
swp ; stw

! Set counter of pixel lines
entr @c3
entr x0008
stw ; nop

@loop1

! Set counter of pixmap words (for each pixel line)
entr @c2
entr x0002
stw ; nop
  
! Get bitmap (line of 8 pixels) 
! Bitmap is in high-byte if pixel line is even, low-byte if pixel line is odd
! Bitmap put in high-byte of stack register (will be read bit-by-bit with RLW)
entr @c3
gtw ; rrw 
jmpc @get_l
entr @p_bmp
gtw ; gth
cll; nop
jump @next2
@get_l
entr @p_bmp
gtw ; gtl
swa ; cll
entr @p_bmp ! prepare to read next bitmap line
dup ; gtw
inc ; stw 
drp ; drp ! drop to have bitmap at top of stack
@next2

@loop2
! Reset pixmap
entr x0000

! Get ink and paper as separated words, shift them to the highest nibble
entr @colors
gth ; cll     ! get ink
ccf ; rlw 
rlw ; rlw
rlw ; nop
entr @colors
gtl ; clh     ! get paper 
swa ; ccf 
rlw ; rlw
rlw ; rlw

! Set counter of pixels
entr @c1
entr x0004
stw; drp
drp; nop

! Now stack is [paper,ink,pixmap,bitmap]
! Re-shuffle to [bitmap,pixmap,paper,ink]

rd3 ; rd4
 
@loop3
! Set pixel value (nibble) in pixmap according to pixel in bitmap
! bitmap is read from left to right (shifting its MSB into CF)
rlw ; nop
jmpc @ink_dot
ru4; orr      ! pixel=paper
ru3; nop 
jump @next1
@ink_dot
rd4 ; rd3
orr ; swp     ! pixel=ink
rd4 ; nop

@next1
! Now stack is [paper,ink,pixmap,bitmap]
! Shift nibbles of ink and paper
ccf ; rrw
rrw ; rrw
rrw ; swp
rrw ; rrw
rrw ; rrw
! Re-shuffle to [bitmap,pixmap,paper,ink]
swp ; rd4
rd4 ; swp 

! Check if pixmap word full
entr @c1    ! decrement c1 counter
dup ; gtw
dec ; stw
drp ; drp  ! drop to restore stack 
jpnz @loop3

! Store pixmap word in video RAM
swp ; nop
entr @p_vram
gtw ; swp
stw ; swp
inc ; nop ! move to next pixmap word on the right
entr @p_vram
swp ; stw
drp ; drp
drp ; nop ! drop to have bitmap at top of stack

! Check if bitmap line fully read
entr @c2
dup ; gtw
dec ; stw
drp ; drp  ! drop to restore stack (and have bitmap at top)
jpnz @loop2

entr @p_vram ! go to next line of image 
dup ; gtw
entr x02E ! = +46 because we already incremented video RAM address twice
add ; swp
drp ; stw
drp ; drp
 
! Check if character fully displayed
entr @c3
dup ; gtw
dec ; stw
drp ; drp
jpnz @loop1

! End of routine
ret ; nop 

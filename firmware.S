.FEATURE    c_comments

/*

MIT License

Copyright (c) 2024 Oliver Schmidt (https://a2retro.de/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

        .org $C000

        ; Disable interrupts
        sei

        ; Switch to normal speed
        lda $C036       ; CYAREG
        and #%01111111
        sta $C036

        ; Switch to SHR
        lda $C029       ; NEWVIDEO
        ora #%11000000
        sta $C029

        ; Enable SHR shadowing
        lda $C035       ; SHADOW
        and #%11110111
        sta $C035

        ; Prepare SHR access
        sta $C000       ; 80STOREOFF
        sta $C005       ; WRCARDRAM

        ; Init SHR screen 
        lda #<$2000     ; SHR
        ldx #>$2000
        sta $EE         ; Free
        stx $EF         ; Free
        ldx #200

        ; Init one row
:       ldy #320/2-1
        lda #$00        ; Palette 0 (-> Fill)
:       sta ($EE),y
        dey
        bne :-
        lda #$10        ; Palette 1 (-> Black)
        sta ($EE),y

        ; Switch to next row
        lda $EE
        clc
        adc #320/2
        sta $EE
        bcc :+
        inc $EF
:       dex
        bne :---

        ; Set SCBs
        ldy #200-1
        lda #%00100000  ; Fill Mode
:       sta ($EE),y
        dey
        bne :-
        sta ($EE),y

        ; Set palettes
        lda #<$0000     ; Black
        ldx #>$0000
        sta $9E02       ; Palette 1
        stx $9E03
        lda #<$0FFF     ; White
        ldx #>$0FFF
        sta $9E04       ; Palette 2
        stx $9E05

        ; Set DOC regs
        lda #%00100000  ; DOC, auto-inc
        sta $C03C

        ; Halt OSC 0/1 and reset accumulator
        lda #$A0        ; Ctrl
        sta $C03E
        lda #%00000011  ; One-shot, halt
        sta $C03D       ; OSC 0
        sta $C03D       ; OSC 1

        ; Set OSC 0/1 frequency to $0004
        lda #$00        ; Freq lo
        sta $C03E
        lda #<$0004
        sta $C03D       ; OSC 0
        sta $C03D       ; OSC 1
        lda #$20        ; Freq hi
        sta $C03E
        lda #>00004
        sta $C03D       ; OSC 0
        sta $C03D       ; OSC 1

        ; Set OSC 0/1 volume to $FF
        lda #$40        ; Vol
        sta $C03E
        lda #$FF        ; Max volume
        sta $C03D       ; OSC 0
        sta $C03D       ; OSC 1

        ; Set OSC 0 addr to $0000 and OSC 1 addr to $8000
        lda #$80        ; Addr
        sta $C03E
        lda #>$0000
        sta $C03D       ; OSC 0
        lda #>$8000
        sta $C03D       ; OSC 1

        ; Set OSC 0/1 wave form to 32kB and to resolution to 0
        lda #$C0        ; Wave
        sta $C03E
        lda #%00111000  ; 32kB, resolution 0
        sta $C03D       ; OSC 0
        sta $C03D       ; OSC 1

        ; Set DOC RAM
        lda #%01100000  ; RAM, auto-inc
        sta $C03C

        ; Disable all expansion ROMs
        bit $CFFF       ; CLRROM

        ; Go for it
        jmp $C800

        .res $0100-<*

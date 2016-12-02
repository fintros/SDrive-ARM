//
// delay100us.s
// Bob!k & Raster, C.P.U., 2008
//

.global Delay100usX
.global Delay100us
.global Delay200us
.global Delay800us
.global Delay1000us

.func Delay100us
Delay100us:
		ldi		r24,1			;1 * 100us = 100us
		rjmp	Delay100usX
.endfunc

.func Delay200us
Delay200us:
		ldi		r24,2			;2 * 100us = 200us
		rjmp	Delay100usX
.endfunc

.func Delay800us
Delay800us:
		ldi		r24,8			;8 * 100us = 800us
		rjmp	Delay100usX
.endfunc

.func Delay1000us
Delay1000us:
		ldi		r24,10			;10 * 100us = 1000us
		rjmp	Delay100usX
.endfunc

//funkce, ktere je predavan typ word:
//parametr prichazi w r24:r25
// pro takt 14318180 Hz => 1431 taktu je 99.943us
.func Delay100usX
Delay100usX:
dela0:	ldi     r25,238     //1
dela1:	dec		r25			//1
		brne	dela1		//2 => 1 + 3 * 238 - 1 =  714
		ldi		r25,238		//1
dela2:	dec		r25			//1
		brne	dela2		//2 => 1 + 3 * 238 - 1 =  714
		dec		r24			//1
		brne	dela0		//2 => r24 * ( 714+714+3) - 1 = r24 * ( 1431 ) - 1 = r24 * 99.943us - 1
		ret
.endfunc

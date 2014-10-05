.thumb
/* MEM COPY ROUTINES
   d = double aligned
   s = source aligned
*/

.global  memcpy8
.thumb_func

memcpy8:					
		ldrb r2, [r1]						
		strb r2, [r0]
		ldrb r2, [r1,#1]							
		strb r2, [r0,#1]
		ldrb r2, [r1,#2]							
		strb r2, [r0,#2]
		ldrb r2, [r1,#3]							
		strb r2, [r0,#3]
		ldrb r2, [r1,#4]							
		strb r2, [r0,#4]
		ldrb r2, [r1,#5]							
		strb r2, [r0,#5]
		ldrb r2, [r1,#6]							
		strb r2, [r0,#6]
		ldrb r2, [r1,#7]							
		strb r2, [r0,#7]	
		bx      lr

.global  memcpy8d
.thumb_func

memcpy8d:					
		ldr r2, [r1]						
		str r2, [r0]
		ldr r2, [r1,#4]							
		str r2, [r0,#4]	
		bx      lr
		
.global  memcpy8s
.thumb_func

memcpy8s:					
		ldr r2, [r1]
		strb r2, [r0]
		lsr r2, #8 
		strb r2, [r0,#1]
		lsr r2, #8 
		strb r2, [r0,#2]
		lsr r2, #8 						
		strb r2, [r0,#3]
		ldr r2, [r1,#4]
		strb r2, [r0,#4]
		lsr r2, #8 
		strb r2, [r0,#5]
		lsr r2, #8 
		strb r2, [r0,#6]
		lsr r2, #8 						
		strb r2, [r0,#7]
		bx      lr		

.global  memcpyv
.thumb_func

memcpyv:
		cmp r2, #0
		beq end_memcpy		
		loop_memcpy:
			sub r2, r2, #1				
			ldrb r3, [r1,r2]						
			strb r3, [r0,r2]
			cmp r2, #0		 
		bne loop_memcpy
		end_memcpy:
		bx      lr

.global  memcpyvd
.thumb_func

memcpyvd:
		cmp r2, #0
		beq end_memcpy		
		loop_memcpy2:
			sub r2, r2, #4				
			ldr r3, [r1,r2]						
			str r3, [r0,r2]
			cmp r2, #0		 
		bne loop_memcpy
		end_memcpy2:
		bx      lr


.macro  cpy256 from, to
		ldr r3, [r1,#\from*4]		
		str r3, [r0,#\from*4]	
        .if     \to-\from
        cpy256 "(\from+1)",\to
        .endif
.endm


.global  memcpy256d
.thumb_func

memcpy256d:
		cpy256 0, 31
		add r0, r0, #128
		add r1, r1, #128
		cpy256 0, 31
		bx lr
		
.global  memcpy160d
.thumb_func

memcpy160d:
		cpy256 0, 31
		add r0, r0, #128
		add r1, r1, #128
		cpy256 0, 7
		bx lr
		
.global  memcpy16d
.thumb_func

memcpy16d:					
		ldr r2, [r1]						
		str r2, [r0]
		ldr r2, [r1,#4]							
		str r2, [r0,#4]
		ldr r2, [r1]						
		str r2, [r0]
		ldr r2, [r1,#8]							
		str r2, [r0,#8]
		ldr r2, [r1]						
		str r2, [r0]
		ldr r2, [r1,#12]							
		str r2, [r0,#12]	
		bx      lr

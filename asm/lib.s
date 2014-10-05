		.ARM
		.ALIGN
		
.macro  lineJump2 from, to
		ldrb r3, [r1,#\from]
		@mov r3,r3,lsl#1				
		lsl r3, #1
		ldrh r3, [r2,r3]		
		strh r3, [r0,#\from*2]	
        .if     \to-\from
        lineJump2 "(\from+1)",\to
        .endif
.endm
		
.global  blendcpy
.type    blendcpy,function

blendcpy:
		cmp r3, #0
		beq end_blendcpy	
		loop_blendcpy:	
			subs r3, r3, #1	
			ldrb r12, [r1,r3]
			orr r12, r12, r2								
			strb r12, [r0,r3]				 			
		bne loop_blendcpy
		end_blendcpy:
		bx      lr	

.global  priused
.type    priused,function

priused:	/*must be aligned*/
		mov r3, #0x80
	    orr r3, r3, #0x8000
	    orr r3, r3, r3, lsl#16
		ldr r1, [r0]
		ands r1, r1, r3
		bne end_priused
		ldr r1, [r0,#4]
		ands r1, r1, r3
		bne end_priused
		ldr r1, [r0,#8]
		ands r1, r1, r3
		bne end_priused
		ldr r1, [r0,#12]
		ands r1, r1, r3
		bne end_priused
		ldr r1, [r0,#16]
		ands r1, r1, r3
		bne end_priused
		ldr r1, [r0,#20]
		ands r1, r1, r3
		bne end_priused
		ldr r1, [r0,#24]
		ands r1, r1, r3
		bne end_priused
		ldr r1, [r0,#28]
		ands r1, r1, r3
		end_priused:
		mov r0, r1
		bx      lr

.thumb

.global  memset8
.thumb_func

memset8:									
		strb r1, [r0]								
		strb r1, [r0,#1]							
		strb r1, [r0,#2]								
		strb r1, [r0,#3]								
		strb r1, [r0,#4]						
		strb r1, [r0,#5]								
		strb r1, [r0,#6]						
		strb r1, [r0,#7]
		bx      lr	

.global  memset8d
.thumb_func

memset8d:									
		str r1, [r0]								
		str r1, [r0,#4]							
		bx      lr	

.global  blendcpy8
.thumb_func

blendcpy8:							
		ldrb r3, [r1]
		orr r3, r3, r2				
		strb r3, [r0]
		ldrb r3, [r1,#1]
		orr r3, r3, r2				
		strb r3, [r0,#1]
		ldrb r3, [r1,#2]
		orr r3, r3, r2	
		strb r3, [r0,#2]
		ldrb r3, [r1,#3]
		orr r3, r3, r2	
		strb r3, [r0,#3]
		ldrb r3, [r1,#4]
		orr r3, r3, r2
		strb r3, [r0,#4]
		ldrb r3, [r1,#5]
		orr r3, r3, r2
		strb r3, [r0,#5]
		ldrb r3, [r1,#6]
		orr r3, r3, r2		
		strb r3, [r0,#6]
		ldrb r3, [r1,#7]
		orr r3, r3, r2				
		strb r3, [r0,#7]
		bx      lr

.global  blendcpy8d
.thumb_func

blendcpy8d:							
		ldr r3, [r1]
		orr r3, r3, r2				
		strb r3, [r0]
		lsr r3, #8
		strb r3, [r0,#1]
		lsr r3, #8
		strb r3, [r0,#2]
		lsr r3, #8
		strb r3, [r0,#3]
		ldr r3, [r1,#4]
		orr r3, r3, r2				
		strb r3, [r0,#4]
		lsr r3, #8
		strb r3, [r0,#5]
		lsr r3, #8
		strb r3, [r0,#6]
		lsr r3, #8
		strb r3, [r0,#7]
		bx      lr
			
.global  memsetv
.thumb_func

memsetv:
		cmp r2, #0
		beq end_memset			
		loop_memset:	
			sub r2, r2, #1										
			strb r1, [r0,r2]	
			cmp r2, #0				 
		bne loop_memset
		end_memset:
		bx      lr
		
		
.global  refresh_2
.thumb_func

refresh_2:
		lineJump2 0, 31
		add r0, r0, #64	
		add r1, r1, #32
		lineJump2 0, 31
		add r0, r0, #64	
		add r1, r1, #32
		lineJump2 0, 31
		add r0, r0, #64	
		add r1, r1, #32
		lineJump2 0, 31
		add r0, r0, #64	
		add r1, r1, #32
		lineJump2 0, 31	
        bx      lr

.global  recolor
.thumb_func

recolor:
		cmp r2, #0
		beq end_recolor	
		loop_recolor:	
			sub r2, r2, #1	
			ldrb r3, [r0,r2]
			orr r3, r3, r1								
			strb r3, [r0,r2]
			cmp r2, #0					 			
		bne loop_recolor
		end_recolor:
		bx      lr

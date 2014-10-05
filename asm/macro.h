.set PSR_n, 0x80000000
.set PSR_Z, 0x40000000
.set PSR_C, 0x20000000
.set PSR_h, 0x10000000

.macro writeb
	bl _Z6writebth
.endm

.macro writew
	bl _Z6writewtt
.endm

.macro readb
	bl _Z5readbt
.endm

.macro readsb
	bl _Z6readsbt
.endm

.macro readw
	bl _Z5readwt
.endm

.macro encodeFLG		@pack GB-Z80 flags into r0	
	and r0,gb_flg,#0xA0000000	@nC
	and r1,gb_flg,#0x50000000	@Zh
	mov r1,r1,lsr#23
	orr r0,r1,r0,lsr#25
.endm

.macro decodeFLG		@unpack GB-Z80 flags from r0
	and gb_flg,r0,#0xA0			@Zh
	and r0,r0,#0x50				@nC
	mov r0,r0,lsl#25
	orr gb_flg,r0,gb_flg,lsl#23
.endm

.macro end_op 
	b opdone
	/*Called after each instruction to jump to end of inst*/
.endm

.macro readPCWord
	mov r0,gb_pc,lsr#16
	readw
	add gb_pc,gb_pc,#0x00020000
.endm

.macro writeA16 reg
	mov r1,gb_a,lsr#24
	mov r0,\reg,lsr#16
	writeb
.endm

.macro readA16 reg
	mov r0,\reg,lsr#16
	readb
	mov gb_a, r0, lsl#24
.endm

.macro readPCByte
	mov r0,gb_pc,lsr#16
	readb
	add gb_pc,gb_pc,#0x00010000
.endm

.macro readPCSigned
	mov r0,gb_pc,lsr#16
	readsb
	add gb_pc,gb_pc,#0x00010000
.endm

.macro readmemHL
	mov r0,gb_hl,lsr#16
	readb
.endm

.macro writememHL /*MUST put byte to be written in r1*/
	mov r0,gb_hl,lsr#16
	writeb
.endm

.macro opPOP
	ldr r0, [gb_cpu,#SP]
	add r1, r0, #0x00020000
	str r1, [gb_cpu,#SP]
	mov r0, r0, lsr#16
	readw	
.endm

.macro opPUSH
	ldr r0, [gb_cpu,#SP]
	sub r0, r0, #0x00020000
	str r0, [gb_cpu,#SP]
	mov r0, r0, lsr#16
	writew
.endm

.macro fetch x
	mov gb_opc, #\x*2
	end_op
.endm


/*************  
*	 ADC	 *
*************/
.macro opADC
	msr cpsr_f,gb_flg				@get C
	subcs r0,r0,#0x100
	eor r1,gb_a,r0,ror#8			@prepare for check of half carry.
	adcs gb_a,gb_a,r0,ror#8
	eor gb_flg,r1,gb_a
	and gb_flg,gb_flg,#PSR_h		@h
	orrcs gb_flg,gb_flg,#PSR_C		@C
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opADCA
	msr cpsr_f,gb_flg				@get C
	orrcs gb_a,gb_a,#0x00800000
	adds gb_a,gb_a,gb_a
	and gb_flg,gb_a,#PSR_h			@h
	orrcs gb_flg,gb_flg,#PSR_C		@C
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opADCH x
	mov r0,\x,lsr#24
	opADC
.endm

.macro opADCL x
	mov r0,\x,lsr#16
	and r0,r0,#0xFF
	opADC
.endm

.macro opADCb
	opADC
.endm

/*************  
*	 ADD16	 *
*************/
.macro opADD16 x
	and gb_flg,gb_flg,#PSR_Z		@save zero, clear n
	eor r1,gb_hl,\x
	adds gb_hl,gb_hl,\x
	eor r1,r1,gb_hl
	orrcs gb_flg,gb_flg,#PSR_C
	tst r1,#0x10000000				@h, correct.
	orrne gb_flg,gb_flg,#PSR_h
.endm

.macro opADD16_HL
	and gb_flg,gb_flg,#PSR_Z		@save zero, clear n
	adds gb_hl,gb_hl,gb_hl
	orrcs gb_flg,gb_flg,#PSR_C
	tst gb_hl,#0x10000000			@h, correct.
	orrne gb_flg,gb_flg,#PSR_h
.endm

/*************  
*	 ADD	 *
*************/
.macro opADD x,y
	eor r1,gb_a,\x,lsl#\y
	adds gb_a,gb_a,\x,lsl#\y
	eor gb_flg,r1,gb_a
	and gb_flg,gb_flg,#PSR_h		@h
	orrcs gb_flg,gb_flg,#PSR_C		@C
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opADDA
	adds gb_a,gb_a,gb_a
	and gb_flg,gb_a,#PSR_h			@h
	orrcs gb_flg,gb_flg,#PSR_C		@C
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opADDH x
	and r0,\x,#0xFF000000
	opADD r0,0
.endm

.macro opADDL x
	opADD \x,8
.endm

.macro opADDb
	opADD r0,24
.endm

/*************  
*	 AND	 *
*************/
.macro opAND x,y
	mov gb_flg,#PSR_h			@set h, clear C & n.
	ands gb_a,gb_a,\x,lsl#\y
	orreq gb_flg,gb_flg,#PSR_Z	@Z
.endm

.macro opANDA
	opAND gb_a,0
.endm

.macro opANDH x
	opAND \x,0
.endm

.macro opANDL x
	opAND \x,8
.endm

.macro opANDb
	opAND r0,24
.endm

/*************  
*	 BIT	 *
*************/
.macro opBITH x y
	and gb_flg,gb_flg,#PSR_C	@keep C
	orr gb_flg,gb_flg,#PSR_h	@set h
	tst \x,#0x01000000<<\y			
	orreq gb_flg,gb_flg,#PSR_Z	@Z
.endm

.macro opBITL x y
	and gb_flg,gb_flg,#PSR_C	@keep C
	orr gb_flg,gb_flg,#PSR_h	@set h
	tst \x,#0x00010000<<\y			
	orreq gb_flg,gb_flg,#PSR_Z	@Z
.endm

.macro opBITb x y
	and gb_flg,gb_flg,#PSR_C	@keep C
	orr gb_flg,gb_flg,#PSR_h	@set h
	tst \x,#0x00000001<<\y			
	orreq gb_flg,gb_flg,#PSR_Z	@Z
.endm

/*************  
*	 CP		 *
*************/
.macro opCP x,y
	eor r1,gb_a,\x,lsl#\y			@prepare for check of half carry.
	subs r0,gb_a,\x,lsl#\y
	eor gb_flg,r1,r0
	and gb_flg,gb_flg,#PSR_h		@h
	orr gb_flg,gb_flg,#PSR_n		@n
	orrcc gb_flg,gb_flg,#PSR_C		@C
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opCPA
	mov gb_flg,#PSR_n|PSR_Z			@set n & Z
.endm

.macro opCPH x
	and r0,\x,#0xFF000000
	opCP r0,0
.endm

.macro opCPL x
	opCP \x,8
.endm

.macro opCPb
	opCP r0,24
.endm

/*************  
*	 DEC	 *
*************/
.macro opDEC8 x
	and gb_flg,gb_flg,#PSR_C	@save carry
	orr gb_flg,gb_flg,#PSR_n	@set n
	tst \x,#0x0f000000			@h
	orreq gb_flg,gb_flg,#PSR_h
	subs \x,\x,#0x01000000
	orreq gb_flg,gb_flg,#PSR_Z
.endm

.macro opDEC8A
	opDEC8 gb_a
.endm

.macro opDEC8H x
	and gb_flg,gb_flg,#PSR_C	@save carry
	orr gb_flg,gb_flg,#PSR_n	@set n
	tst \x,#0x0f000000			@h
	orreq gb_flg,gb_flg,#PSR_h
	sub \x,\x,#0x01000000
	tst \x,#0xff000000			@Z
	orreq gb_flg,gb_flg,#PSR_Z
.endm

.macro opDEC8L x
	mov r0,\x,lsl#8
	opDEC8 r0
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsr#8
.endm

.macro opDEC8b
	and gb_flg,gb_flg,#PSR_C	@save carry
	orr gb_flg,gb_flg,#PSR_n	@set n
	tst r0,#0x0f				@h
	orreq gb_flg,gb_flg,#PSR_h
	sub r0,r0,#0x01
	ands r1,r0,#0xff				@Z
	orreq gb_flg,gb_flg,#PSR_Z
.endm

.macro opDEC16 x
	sub \x,\x,#0x00010000
.endm

/*************  
*	 INC	 *
*************/
.macro opINC8 x
	and gb_flg,gb_flg,#PSR_C	@save carry, clear n
	adds \x,\x,#0x01000000
	orreq gb_flg,gb_flg,#PSR_Z
	tst \x,#0x0f000000			@h
	orreq gb_flg,gb_flg,#PSR_h
.endm

.macro opINC8A
	opINC8 gb_a
.endm

.macro opINC8H x
	and gb_flg,gb_flg,#PSR_C	@save carry, clear n
	add \x,\x,#0x01000000
	tst \x,#0xff000000			@Z
	orreq gb_flg,gb_flg,#PSR_Z
	tst \x,#0x0f000000			@h
	orreq gb_flg,gb_flg,#PSR_h
.endm

.macro opINC8L x
	mov r0,\x,lsl#8
	opINC8 r0
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsr#8
.endm

.macro opINC8b
	and gb_flg,gb_flg,#PSR_C	@save carry, clear n
	add r0,r0,#0x01
	ands r1,r0,#0xff			@Z
	orreq gb_flg,gb_flg,#PSR_Z
	tst r1,#0x0f				@h
	orreq gb_flg,gb_flg,#PSR_h
.endm

.macro opINC16 x
	add \x,\x,#0x00010000
.endm

/*************  
*	 OR		 *
*************/
.macro opOR x,y
	mov gb_flg,#0				@clear flags.
	orrs gb_a,gb_a,\x,lsl#\y
	orreq gb_flg,gb_flg,#PSR_Z	@Z
.endm

.macro opORA
	opOR gb_a,0
.endm

.macro opORH x
	and r0,\x,#0xFF000000
	opOR r0,0
.endm

.macro opORL x
	opOR \x,8
.endm

.macro opORb
	opOR r0,24
.endm

/*************  
*	 RES	 *
*************/
.macro opRESH x, y				
	bic \x,\x,#0x01000000<<\y
.endm

.macro opRESL x, y				
	bic \x,\x,#0x00010000<<\y
.endm

.macro opRESb x, y, z				
	bic \y,\x,#0x00000001<<\z
.endm

/*************  
*	 RL 	 *
*************/
.macro opRL x
	tst gb_flg,#PSR_C				@check C
	orrne \x,\x,#0x00800000
	adds \x,\x,\x
	mrs gb_flg,cpsr					@C & Z
	and gb_flg,gb_flg,#PSR_Z|PSR_C	@only keep C & Z
.endm

.macro opRLH x
	and r0,\x,#0xFF000000
	opRL r0
	and \x,\x,#0x00FF0000
	orr \x,\x,r0
.endm

.macro opRLL x
	mov r0,\x,lsl#8
	opRL r0
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsr#8
.endm

.macro opRLb
	mov r0,r0,lsl#24
	opRL r0
	mov r1,r0,lsr#24
.endm

/*************  
*	 RLC 	 *
*************/
.macro opRLC x
	mov gb_flg,#0				@clear flags
	adds \x,\x,\x
	orrcs gb_flg,gb_flg,#PSR_C	@set C
	orrcss \x,\x,#0x01000000
	orreq gb_flg,gb_flg,#PSR_Z	@set Z
.endm

.macro opRLCH x
	and r0,\x,#0xFF000000
	opRLC r0
	and \x,\x,#0x00FF0000
	orr \x,\x,r0
.endm

.macro opRLCL x
	mov r0,\x,lsl#8
	opRLC r0
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsr#8
.endm

.macro opRLCb
	mov r0,r0,lsl#24
	opRLC r0
	mov r1,r0,lsr#24
.endm

/*************  
*	 RR 	 *
*************/
.macro opRR x
	movs gb_flg,gb_flg,lsr#30	@get C, clear flags
	mov \x,\x,rrx
	tst \x,#0x00800000
	orrne gb_flg,gb_flg,#PSR_C
	ands \x,\x,#0xFF000000
	orreq gb_flg,gb_flg,#PSR_Z
.endm

.macro opRRH x
	and r0,\x,#0xFF000000
	opRR r0
	and \x,\x,#0x00FF0000
	orr \x,\x,r0
.endm

.macro opRRL x
	mov r0,\x,lsl#8
	opRR r0
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsr#8
.endm

.macro opRRb
	mov r0,r0,lsl#24
	opRR r0
	mov r1,r0,lsr#24
.endm

/*************  
*	 RRC 	 *
*************/
.macro opRRC x,y
	mov gb_flg,#0				@clear flags
	movs \x,\x,lsr#\y
	orrcs gb_flg,gb_flg,#PSR_C	@set C
	orrcss \x,\x,#0x00000080
	orreq gb_flg,gb_flg,#PSR_Z	@set Z
.endm

.macro opRRCH x
	and r0,\x,#0xFF000000
	opRRC r0,25
	and \x,\x,#0x00FF0000
	orr \x,\x,r0,lsl#24
.endm

.macro opRRCL x
	and r0,\x,#0x00FF0000
	opRRC r0,17
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsl#16
.endm

.macro opRRCb
	mov gb_flg,#0				@clear flags
	movs r0,r0,lsr#1
	orrcs gb_flg,gb_flg,#PSR_C	@set C
	orrcss r0,r0,#0x00000080
	orreq gb_flg,gb_flg,#PSR_Z	@set Z
	mov r1, r0
.endm

/*************  
*	 SBC 	 *
*************/
.macro opSBC x,y
	eor gb_flg,gb_flg,#PSR_C		@invert C.
	movs gb_flg,gb_flg,lsr#30		@get C, clear flags.
	eor r1,gb_a,\x,lsl#\y			@prepare for check of half carry.
	sbcs gb_a,gb_a,\x,lsl#\y
	eor gb_flg,r1,gb_a
	and gb_flg,gb_flg,#PSR_h		@h
	orr gb_flg,gb_flg,#PSR_n		@n
	orrcc gb_flg,gb_flg,#PSR_C		@C
	ands gb_a,gb_a,#0xFF000000
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opSBCA
	eor gb_flg,gb_flg,#PSR_C		@invert C.
	movs gb_flg,gb_flg,lsr#30		@get C, clear flags.
	sbcs gb_a,gb_a,gb_a
	and gb_flg,gb_a,#PSR_h			@h
	orr gb_flg,gb_flg,#PSR_n		@n
	orrcc gb_flg,gb_flg,#PSR_C		@C
	ands gb_a,gb_a,#0xFF000000
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opSBCH x
	and r0,\x,#0xFF000000
	opSBC r0,0
.endm

.macro opSBCL x
	opSBC \x,8
.endm

.macro opSBCb
	opSBC r0,24
.endm

/*************  
*	 SET 	 *
*************/
.macro opSETH x, y				
	orr \x,\x,#(0x01000000<<\y)
.endm

.macro opSETL x, y				
	orr \x,\x,#(0x00010000<<\y)
.endm

.macro opSETb x, y, z				
	orr \y,\x,#(0x00000001<<\z)
.endm

/*************  
*	 SLA 	 *
*************/
.macro opSLA x,y,z
	movs \y,\x,lsl#\z
	mrs gb_flg,cpsr          		@C & Z
	and gb_flg,gb_flg,#PSR_Z|PSR_C	@only keep C & Z
.endm

.macro opSLAA
	opSLA gb_a,gb_a,1
.endm

.macro opSLAH x
	and r0,\x,#0xFF000000
	opSLA r0,r0,1
	and \x,\x,#0x00FF0000
	orr \x,\x,r0
.endm

.macro opSLAL x
	opSLA \x,r0,9
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsr#8
.endm

.macro opSLAb
	opSLA r0,r0,25
	mov r1,r0,lsr#24
.endm

/*************  
*	 SRA 	 *
*************/

.macro opSRA x
	movs r0,\x,asr#25
	mrs gb_flg,cpsr          		@C & Z
	and gb_flg,gb_flg,#PSR_Z|PSR_C	@only keep C & Z
.endm

.macro opSRAA
	opSRA gb_a
	mov gb_a,r0,lsl#24
.endm

.macro opSRAH x
	opSRA \x
	and \x,\x,#0x00FF0000
	orr \x,\x,r0,lsl#24
.endm

.macro opSRAL x
	mov r0,\x,lsl#8
	opSRA r0
	and r0,r0,#0xff
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsl#16
.endm

.macro opSRAb
	mov r0,r0,lsl#24
	movs r1,r0,asr#25
	mrs gb_flg,cpsr          		@C & Z
	and gb_flg,gb_flg,#PSR_Z|PSR_C	@only keep C & Z
.endm

/*************  
*	 SRL 	 *
*************/
.macro opSRLx x,y,z
	movs \y,\x,lsr#\z
	mrs gb_flg,cpsr          		@C & Z
	and gb_flg,gb_flg,#PSR_Z|PSR_C	@only keep C & Z
.endm

.macro opSRLA
	opSRLx gb_a,gb_a,25
	mov gb_a,gb_a,lsl#24
.endm

.macro opSRLH x
	opSRLx \x,r0,25
	and \x,\x,#0x00FF0000
	orr \x,\x,r0,lsl#24
.endm

.macro opSRLL x
	and r0,\x,#0x00FF0000
	opSRLx r0,r0,17
	and \x,\x,#0xFF000000
	orr \x,\x,r0,lsl#16
.endm

/*************  
*	 SUB 	 *
*************/
.macro opSUB x,y
	eor r1,gb_a,\x,lsl#\y
	subs gb_a,gb_a,\x,lsl#\y
	eor gb_flg,r1,gb_a
	and gb_flg,gb_flg,#PSR_h		@h
	orr gb_flg,gb_flg,#PSR_n		@n
	orrcc gb_flg,gb_flg,#PSR_C		@C
	orreq gb_flg,gb_flg,#PSR_Z		@Z
.endm

.macro opSUBA
	mov gb_a,#0
	mov gb_flg,#PSR_n|PSR_Z			@n & Z
.endm

.macro opSUBH x
	and r0,\x,#0xFF000000
	opSUB r0,0
.endm

.macro opSUBL x
	opSUB \x,8
.endm

.macro opSUBb
	opSUB r0,24
.endm

/*************  
*	 SWAP 	 *
*************/
.macro opSWAP x
	mov \x,\x,ror#28
	orr \x,\x,\x,lsl#24
	ands \x,\x,#0xFF000000
	mrs gb_flg,cpsr          	@Z
	and gb_flg,gb_flg,#PSR_Z	@only keep Z
.endm

.macro opSWAPA
	opSWAP gb_a
.endm

.macro opSWAPL x
	and r0,\x,#0x00FF0000		@mask low to r0
	and \x,\x,#0xFF000000		@mask out high
	mov r0,r0,ror#20
	orrs r0,r0,r0,lsl#24
	mrs gb_flg,cpsr          	@Z
	and gb_flg,gb_flg,#PSR_Z	@only keep Z
	orr \x,\x,r0,lsr#8
.endm

.macro opSWAPH x
	and r0,\x,#0xFF000000		@mask high to r0
	opSWAP r0
	and \x,\x,#0x00FF0000		@mask out low
	orr \x,\x,r0
.endm

.macro opSWAPb
	mov r0,r0,ror#4
	orr r0,r0,r0,lsl#24
	movs r1,r0,lsr#24
	mrs gb_flg,cpsr          	@Z
	and gb_flg,gb_flg,#PSR_Z	@only keep Z
.endm

/*************  
*	 XOR 	 *
*************/
.macro opXOR x,y
	mov gb_flg,#0				@clear flags.
	eors gb_a,gb_a,\x,lsl#\y
	orreq gb_flg,gb_flg,#PSR_Z	@Z
.endm

.macro opXORA
	mov gb_a,#0					@clear A.
	mov gb_flg,#PSR_Z			@Z
.endm

.macro opXORH x
	and r0,\x,#0xFF000000
	opXOR r0,0
.endm

.macro opXORL x
	opXOR \x,8
.endm

.macro opXORb
	opXOR r0,24
.endm


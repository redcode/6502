/* MOS 6502 CPU Emulator
  ____    ____    ___ ___     ___
 / __ \  / ___\  / __` __`\  / __`\
/\ \/  \/\ \__/_/\ \/\ \/\ \/\  __/
\ \__/\_\ \_____\ \_\ \_\ \_\ \____\
 \/_/\/_/\/_____/\/_/\/_/\/_/\/____/
Copyright © 1999-2015 Manuel Sainz de Baranda y Goñi.
Released under the terms of the GNU General Public License v3. */

#ifdef CPU_6502_BUILDING_DYNAMIC
#	define CPU_6502_API Z_API_EXPORT
#else
#	define CPU_6502_API
#endif

#ifdef CPU_6502_USE_LOCAL_HEADER
#	include "6502.h"
#else
#	include <emulation/CPU/6502.h>
#endif


/* MARK: - Types */

typedef struct {
	zuint8 cycles;
	zuint8 (* read )(M6502 *object);
	void   (* write)(M6502 *object, zuint8 value);
} ReadWriteEA;

typedef struct {
	zuint8 cycles;
	zuint8 (* read )(M6502 *object);
} ReadEA;

typedef struct {
	zuint8 cycles;
	void   (* write)(M6502 *object, zuint8 value);
} WriteEA;

typedef zuint8 (* Instruction)(M6502 *object);


/* MARK: - Macros & Functions: Callback */

#ifdef CPU_6502_USE_SLOTS
#	define CB_ACTION(name) object->cb.name.action
#	define CB_OBJECT(name) object->cb.name.object
#else
#	define CB_ACTION(name) object->cb.name
#	define CB_OBJECT(name) object->cb_context
#endif

#define READ_8(address)		CB_ACTION(read )(CB_OBJECT(read ), (address))
#define WRITE_8(address, value)	CB_ACTION(write)(CB_OBJECT(write), (address), (value))


Z_INLINE zuint16 read_16bit(M6502 *object, zuint16 address)
	{return (READ_8(address + 1) << 8) | READ_8(address);}


#define READ_16( address)	   read_16bit (object, address)
#define READ_POINTER(pointer_name) READ_16(Z_6502_ADDRESS_##pointer_name##_POINTER)


/* MARK: - Macros: Registers */

#define PC object->state.Z_6502_STATE_MEMBER_PC
#define S  object->state.Z_6502_STATE_MEMBER_S
#define P  object->state.Z_6502_STATE_MEMBER_P
#define A  object->state.Z_6502_STATE_MEMBER_A
#define X  object->state.Z_6502_STATE_MEMBER_X
#define Y  object->state.Z_6502_STATE_MEMBER_Y


/* MARK: - Macros: Flags */

#define NP 128
#define VP  64
#define XP  32
#define BP  16
#define DP   8
#define IP   4
#define ZP   2
#define CP   1

#define NZP  (NP | ZP)
#define NZCP (NP | ZP | CP)

#define ZP_ZERO( value) (!(value) << 1)
#define SET_P_NZ(value) P = (P & ~NZP) | ((value) ? ((value) & NP) : ZP)


/* MARK: - Macros: Internal State */

#define NMI object->nmi
#define IRQ object->irq


/* MARK: - Macros: Execution Temporal Data */

#define OPCODE object->opcode
#define G_EA   object->g_ea
#define TICKS  object->cycles


/* MARK: - Macros: Stack */

#define PUSH_8(value) WRITE_8(Z_6502_ADDRESS_STACK + (zuint8)S--, value);
#define POP_8	      READ_8 (Z_6502_ADDRESS_STACK + (zuint8)++S)


Z_INLINE void push_16bit(M6502 *object, zuint16 value)
	{
	WRITE_8(Z_6502_ADDRESS_STACK | S, value >> 8);
	WRITE_8(Z_6502_ADDRESS_STACK | (zuint8)(S - 1), (zuint8)value);
	S -= 2;
	}

Z_INLINE zuint16 pop_16bit(M6502 *object)
	{
	zuint16 result =
	  READ_8(Z_6502_ADDRESS_STACK | (zuint8)(S + 1)) |
	((READ_8(Z_6502_ADDRESS_STACK | (zuint8)(S + 2))) << 8);

	S += 2;
	return result;
	}


#define PUSH_16(value) push_16bit(object, value)
#define POP_16	       pop_16bit(object)


/* MARK: - Addressing Helpers */

#define READ_BYTE_OPERAND READ_8 ((PC += 2) - 1)
#define READ_WORD_OPERAND READ_16((PC += 3) - 2)

#define   ZERO_PAGE_ADDRESS READ_BYTE_OPERAND
#define ZERO_PAGE_X_ADDRESS (zuint8)(READ_BYTE_OPERAND + X)
#define ZERO_PAGE_Y_ADDRESS (zuint8)(READ_BYTE_OPERAND + Y)
#define    ABSOLUTE_ADDRESS READ_WORD_OPERAND
#define  ABSOLUTE_X_ADDRESS READ_WORD_OPERAND + X
#define  ABSOLUTE_Y_ADDRESS READ_WORD_OPERAND + Y
#define  INDIRECT_X_ADDRESS READ_16((zuint8)(READ_BYTE_OPERAND + X))
#define  INDIRECT_Y_ADDRESS READ_16(READ_BYTE_OPERAND) + Y

#define EA_READER(name) static zuint8 read_##name (M6502 *object)
#define EA_WRITER(name) static void   write_##name(M6502 *object, zuint8 value)

EA_READER(accumulator)	 {PC++; return A;			    }
EA_READER(immediate)	 {return READ_BYTE_OPERAND;		    }
EA_READER(zero_page)	 {return READ_8(ZERO_PAGE_ADDRESS	  );}
EA_READER(zero_page_x)	 {return READ_8(ZERO_PAGE_X_ADDRESS	  );}
EA_READER(zero_page_y)	 {return READ_8(ZERO_PAGE_Y_ADDRESS	  );}
EA_READER(absolute)	 {return READ_8(ABSOLUTE_ADDRESS	  );}
EA_READER(indirect_x)	 {return READ_8(INDIRECT_X_ADDRESS	  );}
EA_READER(g_zero_page)   {return READ_8(G_EA = ZERO_PAGE_ADDRESS  );}
EA_READER(g_zero_page_x) {return READ_8(G_EA = ZERO_PAGE_X_ADDRESS);}
EA_READER(g_absolute)    {return READ_8(G_EA = ABSOLUTE_ADDRESS	  );}
EA_READER(g_absolute_x)  {return READ_8(G_EA = ABSOLUTE_X_ADDRESS );}


EA_READER(penalized_absolute_x)
	{
	zuint16 address = READ_WORD_OPERAND;

	if ((address & 0xFF) + X > 255) TICKS++;
	return READ_8(address + X);
	}


EA_READER(penalized_absolute_y)
	{
	zuint16 address = READ_WORD_OPERAND;

	if ((address & 0xFF) + Y > 255) TICKS++;
	return READ_8(address + Y);
	}


EA_READER(penalized_indirect_y)
	{
	zuint16 address = READ_16(READ_BYTE_OPERAND);

	if ((address & 0xFF) + Y > 255) TICKS++;
	return READ_8(address + Y);
	}


EA_WRITER(zero_page)   {WRITE_8(ZERO_PAGE_ADDRESS,   value);}
EA_WRITER(zero_page_x) {WRITE_8(ZERO_PAGE_X_ADDRESS, value);}
EA_WRITER(zero_page_y) {WRITE_8(ZERO_PAGE_Y_ADDRESS, value);}
EA_WRITER(absolute)    {WRITE_8(ABSOLUTE_ADDRESS,    value);}
EA_WRITER(absolute_x)  {WRITE_8(ABSOLUTE_X_ADDRESS,  value);}
EA_WRITER(absolute_y)  {WRITE_8(ABSOLUTE_Y_ADDRESS,  value);}
EA_WRITER(indirect_x)  {WRITE_8(INDIRECT_X_ADDRESS,  value);}
EA_WRITER(indirect_y)  {WRITE_8(INDIRECT_Y_ADDRESS,  value);}


/* MARK: - J/K Addressing Tables

		     .-----------------------------------------------.
		     | 000 | 001 | 010 | 011 | 100 | 101 | 110 | 111 |
		     |-----+-----+-----+-----+-----+-----+-----+-----|
		     | ora | and | eor | adc | sta | lda | cmp | sbc |
		     |-----+-----+-----+-----+-----+-----+-----+-----|
		     |	J  |  J  |  J  |  J  |	K  |  J  |  J  |  J  |
.--------------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 000 | (Indirect,X) |	6  |  6  |  6  |  6  |	6  |  6  |  6  |  6  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 001 | Zero Page    |	3  |  3  |  3  |  3  |	3  |  3  |  3  |  3  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 010 | #Immediate   |	2  |  2  |  2  |  2  |	   |  2  |  2  |  2  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 011 | Absolute     |	4  |  4  |  4  |  4  |	4  |  4  |  4  |  4  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 100 | (Indirect),Y | 5+1 | 5+1 | 5+1 | 5+1 |	6  | 5+1 | 5+1 | 5+1 |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 101 | Zero Page,X  |	4  |  4  |  4  |  4  |	4  |  4  |  4  |  4  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 110 | Absolute,Y   | 4+1 | 4+1 | 4+1 | 4+1 |	5  | 4+1 | 4+1 | 4+1 |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 111 | Absolute,X   | 4+1 | 4+1 | 4+1 | 4+1 |	5  | 4+1 | 4+1 | 4+1 |
'-------------------------------------------------------------------*/

static ReadEA const j_table[8] = {
	{6, read_indirect_x	     },
	{3, read_zero_page	     },
	{2, read_immediate	     },
	{4, read_absolute	     },
	{5, read_penalized_indirect_y},
	{4, read_zero_page_x	     },
	{4, read_penalized_absolute_y},
	{4, read_penalized_absolute_x}
};

static WriteEA const k_table[8] = {
	{6, write_indirect_x },
	{3, write_zero_page  },
	{0, NULL	     },
	{4, write_absolute   },
	{6, write_indirect_y },
	{4, write_zero_page_x},
	{5, write_absolute_y },
	{5, write_absolute_x }
};


/* MARK: - G/P Addressing Tables

		      .---------------------------------------------------------------.
		      |  000  |  001  |  010  |  011  |  100  |  101  |  110  |  111  |
		      |-------+-------+-------+-------+-------+-------+-------+-------|
		      |  asl  |  rol  |  lsr  |  ror  |  stx  |  ldx  |  dec  |  inc  |
		      |-------+-------+-------+-------+-------+-------+-------+-------|
		      |   G   |   G   |   G   |   G   |   H   |   H   |   G   |   G   |
.---------------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 000 | Immediate     |       |       |       |       |       |   2   |       |       |
|-----+---------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 001 | Zero Page     |   5   |   5   |   5   |   5   |   3   |   3   |   5   |   5   |
|-----+---------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 010 | Accumulator   |   2   |   2   |   2   |   2   |       |       |       |       |
|-----+---------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 011 | Absolute      |   6   |   6   |   6   |   6   |   4   |   4   |   6   |   6   |
|-----+---------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 100 |		      |	      |       |       |       |       |       |       |       |
|-----+---------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 101 | Zero Page,X/Y |  6/x  |  6/x  |  6/x  |  6/x  |  4/y  |  4/y  |   6   |   6   |
|-----+---------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 110 |		      |	      |       |       |       |       |       |       |       |
|-----+---------------+-------+-------+-------+-------+-------+-------+-------+-------|
| 111 | Absolute,X/Y  |   7   |   7   |   7   |   7   |       | 4+1/y |   7   |   7   |
'------------------------------------------------------------------------------------*/

static ReadEA const g_table[8] = {
	{0, NULL	      },
	{5, read_g_zero_page  },
	{2, read_accumulator  },
	{6, read_g_absolute   },
	{0, NULL	      },
	{6, read_g_zero_page_x},
	{0, NULL	      },
	{7, read_g_absolute_x }
};

static ReadWriteEA const h_table[8] = {
	{2, read_immediate,	       NULL		},
	{3, read_zero_page,	       write_zero_page	},
	{0, NULL,		       NULL		},
	{4, read_absolute,	       write_absolute	},
	{0, NULL,		       NULL		},
	{4, read_zero_page_y,	       write_zero_page_y},
	{0, NULL,		       NULL		},
	{4, read_penalized_absolute_y, NULL		}
};


/* MARK: - Q Addressing Table

		     .-----------------------------------------------.
		     | 000 | 001 | 010 | 011 | 100 | 101 | 110 | 111 |
		     |-----+-----+-----+-----+-----+-----+-----+-----|
		     |	   | bit |     |     | sty | ldy | cpy | cpx |
.--------------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 000 | Immediate    |	   |	 |     |     |	   |  2  |  2  |  2  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 001 | Zero Page    |	   |  3  |     |     |	3  |  3  |  3  |  3  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 010 |		     |	   |	 |     |     |	   |	 |     |     |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 011 | Absolute     |	   |  4  |     |     |	4  |  4  |  4  |  4  |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 100 |		     |	   |	 |     |     |	   |	 |     |     |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 101 | Zero Page,X  |	   |	 |     |     |	4  |  4  |     |     |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 110 |		     |	   |	 |     |     |	   |	 |     |     |
|-----+--------------+-----+-----+-----+-----+-----+-----+-----+-----|
| 111 | Absolute,X   |	   |	 |     |     |	   | 4+1 |     |     |
'-------------------------------------------------------------------*/

static ReadWriteEA const q_table[8] = {
	{2, read_immediate,	       NULL		},
	{3, read_zero_page,	       write_zero_page	},
	{0, NULL,		       NULL		},
	{4, read_absolute,	       write_absolute	},
	{0, NULL,		       NULL		},
	{4, read_zero_page_x,	       write_zero_page_x},
	{0, NULL,		       NULL		},
	{4, read_penalized_absolute_x, NULL		}
};


/* MARK: - Addressing Accessors */

#define SET_EA(type, table) type const *ea = &table##_table[(OPCODE & 28) >> 2];
#define J		    SET_EA(ReadEA,	j)
#define K		    SET_EA(WriteEA,	k)
#define G		    SET_EA(ReadEA,	g)
#define H		    SET_EA(ReadWriteEA, h)
#define Q		    SET_EA(ReadWriteEA, q)
#define READ_EA		    ea->read(object)
#define WRITE_EA(value)	    ea->write(object, value)
#define EA_CYCLES	    ea->cycles
#define WRITE_G_EA(value)   if (EA_CYCLES == 2) A = value; else WRITE_8(G_EA, value);


/* MARK: - Reusable Code */

#define COMPARE(register)							\
	zuint8 v = READ_EA;							\
	zuint8 result = register - v;						\
										\
	P =	(P & ~NZCP)	     /* VP, XP, BP, DP, IP unchanged	     */	\
		| (result & NP)	     /* NP = result.7			     */	\
		| ZP_ZERO(result)    /* ZP = 1 if result = 0, else ZP = 0    */	\
		| !!(register >= v); /* CP = 1 if register >= v, else CP = 0 */	\
										\
	return EA_CYCLES;


#define BRANCH(flag_mask, condition_logic)	 \
	zuint8 cycles = 2;			 \
						 \
	if (condition_logic(P & flag_mask))	 \
		{				 \
		zuint16 pc = PC + 2;		 \
		zint8 offset = READ_8(PC + 1);	 \
		zuint16 t = pc + offset;	 \
						 \
		if (t >> 8 == pc >> 8) cycles++; \
		else cycles += 2;		 \
		PC = t;				 \
		}				 \
						 \
	else PC += 2;				 \
						 \
	return cycles;


#define BRANCH_IF_CLEAR(flag_mask) BRANCH(flag_mask, !)
#define BRANCH_IF_SET(  flag_mask) BRANCH(flag_mask,  )


#define INC_DEC(operation)		\
	G;				\
	zuint8 t = READ_EA operation 1;	\
					\
	WRITE_G_EA(t);			\
	SET_P_NZ(t);			\
	return EA_CYCLES;


#define INSTRUCTION(name) static zuint8 name(M6502 *object)


/* MARK: - Instructions: Load/Store Operations
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  lda J	101jjj01  n.....z.  J	    |
|  ldx H	101ppp10  ........  H	    |
|  ldy Q	101qqq00  ........  Q	    |
|  sta K	100kkk01  ........  K	    |
|  stx H	100ppp10  ........  H	    |
|  sty Q	100qqq00  ........  Q	    |
'------------------------------------------*/

INSTRUCTION(lda_J) {J; A = READ_EA; SET_P_NZ(A); return EA_CYCLES;}
INSTRUCTION(ldx_H) {H; X = READ_EA; SET_P_NZ(X); return EA_CYCLES;}
INSTRUCTION(ldy_Q) {Q; Y = READ_EA; SET_P_NZ(Y); return EA_CYCLES;}
INSTRUCTION(sta_K) {K; WRITE_EA(A);		 return EA_CYCLES;}
INSTRUCTION(stx_H) {H; WRITE_EA(X);		 return EA_CYCLES;}
INSTRUCTION(sty_Q) {Q; WRITE_EA(Y);		 return EA_CYCLES;}


/* MARK: - Instructions: Register Transfers
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  tax		<  AA  >  n.....z.  2	    |
|  tay		<  A8  >  n.....z.  2	    |
|  txa		<  8A  >  n.....z.  2	    |
|  tya		<  98  >  n.....z.  2	    |
'------------------------------------------*/

INSTRUCTION(tax) {PC++; X = A; SET_P_NZ(X); return 2;}
INSTRUCTION(tay) {PC++; Y = A; SET_P_NZ(Y); return 2;}
INSTRUCTION(txa) {PC++; A = X; SET_P_NZ(A); return 2;}
INSTRUCTION(tya) {PC++; A = Y; SET_P_NZ(A); return 2;}


/* MARK: - Instructions: Stack Operations
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  tsx		<  BA  >  n.....z.  2	    |
|  txs		<  9A  >  ........  2	    |
|  pha		<  48  >  ........  3	    |
|  php		<  08  >  ........  3	    |
|  pla		<  68  >  n.....z.  4	    |
|  plp		<  28  >  ********  4	    |
'------------------------------------------*/

INSTRUCTION(tsx) {PC++; X = S; SET_P_NZ(X);	return 2;}
INSTRUCTION(txs) {PC++; S = X;			return 2;}
INSTRUCTION(pha) {PC++; PUSH_8(A);		return 3;}
INSTRUCTION(php) {PC++; PUSH_8(P);		return 3;}
INSTRUCTION(pla) {PC++; A = POP_8; SET_P_NZ(A); return 4;}
INSTRUCTION(plp) {PC++; P = POP_8;		return 4;}


/* MARK: - Instructions: Logical
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  and J	001jjj01  n.....z.  J	    |
|  eor J	010jjj01  n.....z.  J	    |
|  ora J	000jjj01  n.....z.  J	    |
|  bit Q	001qqq00  **....z.  Q	    |
'------------------------------------------*/

INSTRUCTION(and_J) {J; A &= READ_EA; SET_P_NZ(A); return EA_CYCLES;}
INSTRUCTION(eor_J) {J; A ^= READ_EA; SET_P_NZ(A); return EA_CYCLES;}
INSTRUCTION(ora_J) {J; A |= READ_EA; SET_P_NZ(A); return EA_CYCLES;}


INSTRUCTION(bit_Q)
	{
	Q;
	zuint8 v = READ_EA;

	P =	(P & ~(NP | VP | ZP))
		| (v & (NP | VP)); /* TODO: Check if this is correct. */

	if (!(v & A)) P |= ZP;
	return EA_CYCLES;
	}


/* MARK: - Instructions: Arithmetic
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  cmp J	110jjj01  n.....zc  J	    |
|  cpx Q	111qqq00  n.....zc  Q	    |
|  cpy Q	110qqq00  n.....zc  Q	    |
|  adc J	011jjj01  nv....zc  J	    |
|  sbc J	111jjj01  nv....zc  J	    |
'------------------------------------------*/

INSTRUCTION(cmp_J) {J; COMPARE(A)}
INSTRUCTION(cpx_Q) {Q; COMPARE(X)}
INSTRUCTION(cpy_Q) {Q; COMPARE(Y)}


INSTRUCTION(adc_J)
	{
	J;
	zuint8 v = READ_EA;
	zuint8 c = P & CP;

	if (P & DP)
		{
		zuint16 l = (A & 0x0F) + (v & 0x0F) + c;
		zuint16 h = (A & 0xF0) + (v & 0xF0);

		P &= ~(VP | CP | NP | ZP);

		if (!((l + h) & 0xFF)) P |= ZP;

		if (l > 0x09)
			{
			h += 0x10;
			l += 0x06;
			}

		if (h & 0x80)		     P |= NP;
		if (~(A ^ v) & (A ^ h) & NP) P |= VP;
		if (h > 0x90)		     h += 0x60;
		if (h >> 8)		     P |= CP;

		A = (l & 0x0F) | (h & 0xF0);
		}

	else	{
		zuint16 t = A + v + c;

					     P &= ~(VP | CP);
		if (~(A ^ v) & (A ^ t) & NP) P |= VP;
		if (t >> 8)		     P |= CP;

		A = (zuint8) t;
		SET_P_NZ(A);
		}

	return EA_CYCLES;
	}


INSTRUCTION(sbc_J)
	{
	J;
	zuint8	v = READ_EA;
	zuint8	c = !(P & CP);
	zuint16	t = A - v - c;

	if (P & DP)
		{
		int l = (A & 0x0F) - (v & 0x0F) - c;
		int h = (A & 0xF0) - (v & 0xF0);

		if (l & 0x10)
			{
			l -= 6;
			h--;
			}

		P &= ~(VP | CP | ZP | NP);

		if ((A ^ v) & (A ^ t) & NP) P |= VP;
		if (!(t >> 8))		    P |= CP;
		if (!(t << 8))		    P |= ZP;
		if (t & 0x80)		    P |= NP;
		if (h & 0x0100)		    h -= 0x60;

		A = (l & 0x0F) | (h & 0xF0);
		}

	else	{
		zuint16 t = A - v - c;

					    P &= ~(VP | CP);
		if ((A ^ v) & (A ^ t) & NP) P |= VP;
		if (!(t >> 8))		    P |= CP;

		A = (zuint8) t;
		SET_P_NZ(A);
		}

	return EA_CYCLES;
	}


/* MARK: - Instructions: Increments & Decrements
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  inc G	111ggg10  n.....z.  G	    |
|  inx		<  E8  >  n.....z.  2	    |
|  iny		<  C8  >  n.....z.  2	    |
|  dec G	110ggg10  n.....z.  G	    |
|  dex		<  CA  >  n.....z.  2	    |
|  dey		<  88  >  n.....z.  2	    |
'------------------------------------------*/

INSTRUCTION(inc_G) {INC_DEC(+)			     }
INSTRUCTION(inx)   {PC++; X++; SET_P_NZ(X); return 2;}
INSTRUCTION(iny)   {PC++; Y++; SET_P_NZ(Y); return 2;}
INSTRUCTION(dec_G) {INC_DEC(-)			     }
INSTRUCTION(dex)   {PC++; X--; SET_P_NZ(X); return 2;}
INSTRUCTION(dey)   {PC++; Y--; SET_P_NZ(Y); return 2;}


/* MARK: - Instructions: Shifts
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  asl G	000ggg10  n.....z*  G	    |
|  lsr G	010ggg10  0.....z*  G	    |
|  rol G	001ggg10  n.....z*  G	    |
|  ror G	011ggg10  n.....z*  G	    |
'------------------------------------------*/

/*#define ASL_LSR_ROL_ROR(t_value, c_value)		     \
	G;						     \
	zuint8 v = READ_EA, t = t_value;		     \
							     \
	P =	(t	? (P & ~(NP | ZP | CP)) | ((t) & NP) \
			: (P & ~(NP |	   CP)) | ZP)	     \
		| c_value;				     \
							     \
	WRITE_G_EA(t);					     \
	return EA_CYCLES;*/


INSTRUCTION(asl_G)
	{
	G;
	zuint v = READ_EA, t = v << 1;

	WRITE_G_EA(t);
	P = (P & ~NZCP) | (t & NP) | ZP_ZERO(t) | (v >> 7);
	return EA_CYCLES;
	}


INSTRUCTION(lsr_G)
	{
	G;
	zuint v = READ_EA, t = v >> 1;

	WRITE_G_EA(t);
	P = (P & ~NZCP) | ZP_ZERO(t) | (v & CP);
	return EA_CYCLES;
	}


INSTRUCTION(rol_G)
	{
	G;
	zuint v = READ_EA, t = (v << 1) | (P & CP);

	WRITE_G_EA(t);
	P = (P & ~NZCP) | (t & NP) | ZP_ZERO(t) | (v >> 7);
	return EA_CYCLES;
	}


INSTRUCTION(ror_G)
	{
	G;
	zuint v = READ_EA, t = (v >> 1) | ((P & CP) << 7);

	WRITE_G_EA(t);
	P = (P & ~NZCP) | (t & NP) | ZP_ZERO(t) | (v & CP);
	return EA_CYCLES;
	}


/* MARK: - Instructions: Jumps & Calls
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  jmp WORD	<  4C  >  ........  3	    |
|  jmp (WORD)	<  6C  >  ........  5	    |
|  jsr WORD	<  20  >  ........  6	    |
|  rts		<  60  >  ........  6	    |
'------------------------------------------*/

INSTRUCTION(jmp_WORD)  {PC = READ_16(PC + 1);		       return 3;}
INSTRUCTION(jmp_vWORD) {PC = READ_16(READ_16(PC + 1));	       return 5;}
INSTRUCTION(jsr_WORD)  {PUSH_16(PC + 2); PC = READ_16(PC + 1); return 6;}
INSTRUCTION(rts)       {PC = POP_16 + 1;		       return 6;}


/* MARK: - Instructions: Branches
.----------------------------------------------.
|		Opcode	  Flags		       |
|  Instruction	76543210  nvxbdizc  Cycles     |
|  ------------------------------------------  |
|  bcc OFFSET	<  90  >  ........  2 / 3 / 4  |
|  bcs OFFSET	<  B0  >  ........  2 / 3 / 4  |
|  beq OFFSET	<  F0  >  ........  2 / 3 / 4  |
|  bmi OFFSET	<  30  >  ........  2 / 3 / 4  |
|  bne OFFSET	<  D0  >  ........  2 / 3 / 4  |
|  bpl OFFSET	<  10  >  ........  2 / 3 / 4  |
|  bvc OFFSET	<  50  >  ........  2 / 3 / 4  |
|  bvs OFFSET	<  70  >  ........  2 / 3 / 4  |
'---------------------------------------------*/

INSTRUCTION(bcc_OFFSET) {BRANCH_IF_CLEAR(CP);}
INSTRUCTION(bcs_OFFSET) {BRANCH_IF_SET	(CP);}
INSTRUCTION(beq_OFFSET) {BRANCH_IF_SET	(ZP);}
INSTRUCTION(bmi_OFFSET) {BRANCH_IF_SET	(NP);}
INSTRUCTION(bne_OFFSET) {BRANCH_IF_CLEAR(ZP);}
INSTRUCTION(bpl_OFFSET) {BRANCH_IF_CLEAR(NP);}
INSTRUCTION(bvc_OFFSET) {BRANCH_IF_CLEAR(VP);}
INSTRUCTION(bvs_OFFSET) {BRANCH_IF_SET	(VP);}


/* MARK: - Instructions: Status Flag Changes
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  clc		<  18  >  .......0  2	    |
|  cld		<  D8  >  ....0...  2	    |
|  cli		<  58  >  .....0..  2	    |
|  clv		<  B8  >  .0......  2	    |
|  sec		<  38  >  .......1  2	    |
|  sed		<  F8  >  ....1...  2	    |
|  sei		<  78  >  .....1..  2	    |
'------------------------------------------*/

INSTRUCTION(clc) {PC++; P &= ~CP; return 2;}
INSTRUCTION(cld) {PC++; P &= ~DP; return 2;}
INSTRUCTION(cli) {PC++; P &= ~IP; return 2;}
INSTRUCTION(clv) {PC++; P &= ~VP; return 2;}
INSTRUCTION(sec) {PC++; P |=  CP; return 2;}
INSTRUCTION(sed) {PC++; P |=  DP; return 2;}
INSTRUCTION(sei) {PC++; P |=  IP; return 2;}


/* MARK: - Instructions: System Functions
.-------------------------------------------.
|		Opcode	  Flags		    |
|  Instruction	76543210  nvxbdizc  Cycles  |
|  ---------------------------------------  |
|  nop		<  EA  >  ........  2	    |
|  rti		<  40  >  ********  6	    |
|  brk		<  00  >  .....1..  7	    |
'------------------------------------------*/

INSTRUCTION(nop) {PC++;			  return 2;}
INSTRUCTION(rti) {P = POP_8; PC = POP_16; return 6;}


INSTRUCTION(brk)
	{
	READ_8 (PC + 1); /* BRK padding byte, ignored but the access is emulated */
	PUSH_16(PC + 2);
	PUSH_8(P | BP);
	P |=  BP | IP;
	PC = READ_POINTER(BRK);
	return 7;
	}


INSTRUCTION(illegal) {return 2;}


/* MARK: - Instruction Function Table */

static Instruction const instruction_table[256] = {
/* 	0	    1	   2	    3	     4	      5	     6	    7	     8	  9	   A	    B	     C		D      E	F	*/
/* 0 */	brk,	    ora_J, illegal, illegal, illegal, ora_J, asl_G, illegal, php, ora_J,   asl_G,   illegal, illegal,	ora_J, asl_G,	illegal,
/* 1 */	bpl_OFFSET, ora_J, illegal, illegal, illegal, ora_J, asl_G, illegal, clc, ora_J,   illegal, illegal, illegal,	ora_J, asl_G,	illegal,
/* 2 */	jsr_WORD,   and_J, illegal, illegal, bit_Q,   and_J, rol_G, illegal, plp, and_J,   rol_G,   illegal, bit_Q,	and_J, rol_G,	illegal,
/* 3 */	bmi_OFFSET, and_J, illegal, illegal, illegal, and_J, rol_G, illegal, sec, and_J,   illegal, illegal, illegal,	and_J, rol_G,	illegal,
/* 4 */	rti,	    eor_J, illegal, illegal, illegal, eor_J, lsr_G, illegal, pha, eor_J,   lsr_G,   illegal, jmp_WORD,	eor_J, lsr_G,	illegal,
/* 5 */	bvc_OFFSET, eor_J, illegal, illegal, illegal, eor_J, lsr_G, illegal, cli, eor_J,   illegal, illegal, illegal,	eor_J, lsr_G,	illegal,
/* 6 */	rts,	    adc_J, illegal, illegal, illegal, adc_J, ror_G, illegal, pla, adc_J,   ror_G,   illegal, jmp_vWORD,	adc_J, ror_G,	illegal,
/* 7 */	bvs_OFFSET, adc_J, illegal, illegal, illegal, adc_J, ror_G, illegal, sei, adc_J,   illegal, illegal, illegal,	adc_J, ror_G,	illegal,
/* 8 */	illegal,    sta_K, illegal, illegal, sty_Q,   sta_K, stx_H, illegal, dey, illegal, txa,	    illegal, sty_Q,	sta_K, stx_H,	illegal,
/* 9 */	bcc_OFFSET, sta_K, illegal, illegal, sty_Q,   sta_K, stx_H, illegal, tya, sta_K,   txs,	    illegal, illegal,	sta_K, illegal,	illegal,
/* A */	ldy_Q,	    lda_J, ldx_H,   illegal, ldy_Q,   lda_J, ldx_H, illegal, tay, lda_J,   tax,	    illegal, ldy_Q,	lda_J, ldx_H,	illegal,
/* B */	bcs_OFFSET, lda_J, illegal, illegal, ldy_Q,   lda_J, ldx_H, illegal, clv, lda_J,   tsx,	    illegal, ldy_Q,	lda_J, ldx_H,	illegal,
/* C */	cpy_Q,	    cmp_J, illegal, illegal, cpy_Q,   cmp_J, dec_G, illegal, iny, cmp_J,   dex,	    illegal, cpy_Q,	cmp_J, dec_G,	illegal,
/* D */	bne_OFFSET, cmp_J, illegal, illegal, illegal, cmp_J, dec_G, illegal, cld, cmp_J,   illegal, illegal, illegal,	cmp_J, dec_G,	illegal,
/* E */	cpx_Q,	    sbc_J, illegal, illegal, cpx_Q,   sbc_J, inc_G, illegal, inx, sbc_J,   nop,	    illegal, cpx_Q,	sbc_J, inc_G,	illegal,
/* F */	beq_OFFSET, sbc_J, illegal, illegal, illegal, sbc_J, inc_G, illegal, sed, sbc_J,   illegal, illegal, illegal,	sbc_J, inc_G,	illegal
};


/* MARK: - Main Functions */

CPU_6502_API zsize m6502_run(M6502 *object, zsize cycles)
	{
	/*------------.
	| Clear ticks |
	'------------*/
	TICKS = 0;

	/*------------------------------.
	| Execute until cycles consumed |
	'------------------------------*/
	while (TICKS < cycles)
		{
		/*--------------------------------------.
		| Jump to NMI handler if NMI pending... |
		'--------------------------------------*/
		if (NMI)
			{
			NMI = FALSE;		/* Clear the NMI pulse.				       */
			P &= ~BP;
			PUSH_16(PC);		/* Save return addres in the stack.		       */
			PUSH_8(P);		/* Save current status in the stack.		       */
			PC = READ_POINTER(NMI); /* Make PC point to the NMI routine.		       */
			P |= IP;		/* Disable interrupts to don't bother the NMI routine. */
			TICKS += 7;		/* Accepting a NMI consumes 7 ticks.		       */
			continue;
			}

		/*--------------------------.
		| Execute IRQ if pending... |
		'--------------------------*/
		if (IRQ && !(P & IP))
			{
			P &= ~BP;
			PUSH_16(PC);
			PUSH_8(P);
			PC = READ_POINTER(IRQ);
			P |= IP;
			TICKS += 7;

#			ifdef CPU_6502_AUTOCLEAR_IRQ_LINE
				IRQ = FALSE;
#			endif

			continue;
			}

		/*-----------------------------------------------.
		| Execute instruction and update consumed cycles |
		'-----------------------------------------------*/
		TICKS += instruction_table[OPCODE = READ_8(PC)](object);
		}

	return TICKS;
	}


CPU_6502_API void m6502_reset(M6502 *object)
	{
	PC = 0; //READ_POINTER(RESET);
	S = 0xFF;
	P = IP;
	A = 0;
	X = 0;
	Y = 0;
	IRQ = FALSE;
	NMI = FALSE;
	}


CPU_6502_API void m6502_power(M6502 *object, zboolean state) {if (state) m6502_reset(object);}
CPU_6502_API void m6502_nmi  (M6502 *object)		     {NMI = TRUE ;}
CPU_6502_API void m6502_irq  (M6502 *object, zboolean state) {IRQ = state;}


#ifdef CPU_6502_BUILDING_MODULE

	#include <Z/ABIs/emulation.h>

	static ZEmulatorExport const exports[5] = {
		{Z_EMULATOR_ACTION_POWER, (ZDo)m6502_power},
		{Z_EMULATOR_ACTION_RESET, (ZDo)m6502_reset},
		{Z_EMULATOR_ACTION_RUN,	  (ZDo)m6502_run  },
		{Z_EMULATOR_ACTION_NMI,	  (ZDo)m6502_nmi  },
		{Z_EMULATOR_ACTION_INT,	  (ZDo)m6502_irq  }
	};

	#define SLOT_OFFSET(name) Z_OFFSET_OF(M6502, cb.name)

	static ZEmulatorSlotLinkage const slot_linkages[2] = {
		{Z_EMULATOR_OBJECT_MEMORY, Z_EMULATOR_ACTION_READ_8BIT,  SLOT_OFFSET(read )},
		{Z_EMULATOR_OBJECT_MEMORY, Z_EMULATOR_ACTION_WRITE_8BIT, SLOT_OFFSET(write)}
	};

	CPU_6502_API ZCPUEmulatorABI const abi_cpu_6502 = {
		0, NULL, 5, exports, {sizeof(M6502), Z_OFFSET_OF(M6502, state), 2, slot_linkages}
	};

#endif


/* 6502.c EOF */

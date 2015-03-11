/* MOS 6502 CPU Emulator v1.0
  ____    ____    ___ ___     ___
 / __ \  / ___\  / __` __`\  / __`\
/\ \/  \/\ \__/_/\ \/\ \/\ \/\  __/
\ \__/\_\ \_____\ \_\ \_\ \_\ \____\
 \/_/\/_/\/_____/\/_/\/_/\/_/\/____/
Copyright © 1999-2015 Manuel Sainz de Baranda y Goñi.
Released under the terms of the GNU General Public License v3. */

#ifndef __emulation_CPU_6502_H__
#define __emulation_CPU_6502_H__

#include <Q/hardware/CPU/architecture/650x.h>
#include <Q/types/generic functions.h>

#ifdef EMULATION_CPU_6502_NO_SLOTS
#	define M6502_CB(Type) Type
#else
#	include <Q/macros/slot.h>
#	define M6502_CB QSlot
#endif

#if defined(BUILDING_DYNAMIC_EMULATION_CPU_6052)
#	define M6502_API Q_API_EXPORT
#elif defined(BUILDING_STATIC_EMULATION_CPU_6502)
#	define M6502_API Q_PUBLIC
#elif defined(USE_STATIC_EMULATION_CPU_6502)
#	define M6502_API
#else
#	define M6502_API Q_API
#endif

typedef struct {
	qsize	   ticks;
	Q6502State state;
	quint8	   opcode;
	quint16	   g_ea;
	quint8	   cycles;
	qboolean   nmi;
	qboolean   irq;

#	ifdef EMULATION_CPU_6502_NO_SLOTS
		void* cb_context;
#	endif

	struct {M6502_CB(Q16BitAddressRead8Bit ) read;
		M6502_CB(Q16BitAddressWrite8Bit) write;
	} cb;
} M6502;

Q_C_SYMBOLS_BEGIN

M6502_API qsize m6502_run   (M6502*   object,
			     qsize    cycles);

M6502_API void  m6502_power (M6502*   object,
			     qboolean state);

M6502_API void  m6502_reset (M6502*   object);

M6502_API void  m6502_nmi   (M6502*   object);

M6502_API void  m6502_irq   (M6502*   object,
			     qboolean state);

Q_C_SYMBOLS_END

#endif /* __emulation_CPU_6502_H__ */

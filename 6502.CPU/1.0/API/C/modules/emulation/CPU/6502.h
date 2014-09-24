/* MOS 6502 CPU Emulator v1.0
  ____    ____    ___ ___     ___
 / __ \  / ___\  / __` __`\  / __`\
/\ \/  \/\ \__/_/\ \/\ \/\ \/\  __/
\ \__/\_\ \_____\ \_\ \_\ \_\ \____\
 \/_/\/_/\/_____/\/_/\/_/\/_/\/____/
Copyright © 2009 Manuel Sainz de Baranda y Goñi.
Released under the terms of the GNU General Public License v3. */

#include <Q/hardware/CPU/architecture/650x.h>
#include <Q/types/generic functions.h>

#ifndef __modules_emulation_CPU_6502_H__
#define __modules_emulation_CPU_6502_H__

typedef struct {
	Q6502State state;
	quint8	   opcode;
	quint16	   g_ea;
	qsize	   ticks;
	quint8	   cycles;
	qboolean   nmi;
	qboolean   irq;
	void*	   cb_context;

	struct {Q16BitAddressRead8Bit  read;
		Q16BitAddressWrite8Bit write;
	} cb;
} M6502;

#ifndef BUILDING_CPU_6502

#	ifdef __cplusplus
	extern "C" {
#	endif

#	ifdef USING_STATIC_CPU_6502
#		define CPU_6502_API
#	else
#		define CPU_6502_API Q_API
#	endif
		
	CPU_6502_API qsize m6502_run   (M6502*	 object,
					qsize	 cycles);

	CPU_6502_API void  m6502_power (M6502*	 object,
					qboolean state);

	CPU_6502_API void  m6502_reset (M6502*	 object);

	CPU_6502_API void  m6502_nmi   (M6502*	 object);

	CPU_6502_API void  m6502_irq   (M6502*	 object,
					qboolean state);

#ifdef __cplusplus
}
#endif

#endif /* __modules_emulation_CPU_6502_H__ */

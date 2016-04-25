/* MOS 6502 CPU Emulator C API
  ____    ____    ___ ___     ___
 / __ \  / ___\  / __` __`\  / __`\
/\ \/  \/\ \__/_/\ \/\ \/\ \/\  __/
\ \__/\_\ \_____\ \_\ \_\ \_\ \____\
 \/_/\/_/\/_____/\/_/\/_/\/_/\/____/
Copyright © 1999-2016 Manuel Sainz de Baranda y Goñi.
Released under the terms of the GNU General Public License v3. */

#ifndef __emulation_CPU_6502_H__
#define __emulation_CPU_6502_H__

#include <Z/hardware/CPU/architecture/650x.h>
#include <Z/ABIs/generic/emulation.h>

#ifdef CPU_6502_USE_SLOTS
#	include <Z/macros/slot.h>
#endif

typedef struct {
	zsize	   cycles;
	Z6502State state;
	zuint8	   opcode;
	zuint16	   g_ea;
	zboolean   nmi;
	zboolean   irq;

#	ifdef CPU_6502_USE_SLOTS
		struct {ZSlot(ZContext16BitAddressRead8Bit ) read;
			ZSlot(ZContext16BitAddressWrite8Bit) write;
		} cb;
#	else
		void* cb_context;

		struct {ZContext16BitAddressRead8Bit  read;
			ZContext16BitAddressWrite8Bit write;
		} cb;
#	endif
} M6502;

Z_C_SYMBOLS_BEGIN

#ifndef CPU_6502_ABI
#	ifdef CPU_6502_AS_STATIC
#		define CPU_6502_ABI
#	else
#		define CPU_6502_ABI Z_API
#	endif
#endif

CPU_6502_ABI extern ZCPUEmulatorABI const abi_emulation_cpu_6520;

#ifndef CPU_6502_API
#	ifdef CPU_6502_AS_STATIC
#		define CPU_6502_API
#	else
#		define CPU_6502_API Z_API
#	endif
#endif

CPU_6502_API zsize m6502_run   (M6502*	 object,
				zsize	 cycles);

CPU_6502_API void  m6502_power (M6502*	 object,
				zboolean state);

CPU_6502_API void  m6502_reset (M6502*	 object);

CPU_6502_API void  m6502_nmi   (M6502*	 object);

CPU_6502_API void  m6502_irq   (M6502*	 object,
				zboolean state);

Z_C_SYMBOLS_END

#endif /* __emulation_CPU_6502_H__ */

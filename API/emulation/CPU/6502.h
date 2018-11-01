/* MOS Technology 6502 CPU Emulator API
  ____    ____    ___ ___     ___
 / __ \  / ___\  / __` __`\  / __`\
/\ \/  \/\ \__/_/\ \/\ \/\ \/\  __/
\ \__/\_\ \_____\ \_\ \_\ \_\ \____\
 \/_/\/_/\/_____/\/_/\/_/\/_/\/____/
Copyright (C) 1999-2018 Manuel Sainz de Baranda y Goñi.
Released under the terms of the GNU General Public License v3.

This library  is free software: you  can redistribute it and/or  modify it under
the terms of  the GNU General Public  License as published by  the Free Software
Foundation,  either version  3 of  the License,  or (at  your option)  any later
version.

This library is distributed in the hope  that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty  of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should  have received a  copy of the GNU  General Public License  along with
this library. If not, see <http://www.gnu.org/licenses/>. */

#ifndef _emulation_CPU_6502_H_
#define _emulation_CPU_6502_H_

#include <Z/hardware/CPU/architecture/6502.h>
#include <Z/ABIs/generic/emulation.h>

typedef struct {
	zusize	   cycles;
	Z6502State state;
	zuint8	   opcode;
	zuint16	   g_ea;
	zboolean   nmi;
	zboolean   irq;
	void*	   callback_context;

	zuint8 (* read )(void *context, zuint16 address);
	void   (* write)(void *context, zuint16 address, zuint8 value);
} M6502;

Z_C_SYMBOLS_BEGIN

#ifndef CPU_6502_ABI
#	ifdef CPU_6502_STATIC
#		define CPU_6502_ABI
#	else
#		define CPU_6502_ABI Z_API
#	endif
#endif

CPU_6502_ABI extern ZCPUEmulatorABI const abi_emulation_cpu_6502;

#ifndef CPU_6502_API
#	ifdef CPU_6502_STATIC
#		define CPU_6502_API
#	else
#		define CPU_6502_API Z_API
#	endif
#endif

CPU_6502_API void   m6502_power(M6502 *object, zboolean state);
CPU_6502_API void   m6502_reset(M6502 *object);
CPU_6502_API zusize m6502_run  (M6502 *object, zusize cycles);
CPU_6502_API void   m6502_nmi  (M6502 *object);
CPU_6502_API void   m6502_irq  (M6502 *object, zboolean state);

Z_C_SYMBOLS_END

#endif /* _emulation_CPU_6502_H_ */

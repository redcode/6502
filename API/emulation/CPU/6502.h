/*   ____  ______  ______   ____
    /  _/_/\  __/_/\     \ /\_, \
   /\  __ \ \___  \ \  \  \\//  /__
   \ \_____\/\____/\ \_____\/\_____\
MOS \/_____/\/___/  \/_____/\/_____/ CPU Emulator
Copyright (C) 1999-2018 Manuel Sainz de Baranda y Goñi.

This emulator is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published  by the Free Software
Foundation, either  version 3 of  the License, or  (at your option)  any later
version.

This emulator is distributed  in the hope that it will  be useful, but WITHOUT
ANY WARRANTY; without even the  implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received  a copy of the GNU General Public License  along with
this emulator. If not, see <http://www.gnu.org/licenses/>. */

#ifndef _emulation_CPU_6502_H_
#define _emulation_CPU_6502_H_

#ifdef CPU_6502_DEPENDENCIES_H
#	include CPU_6502_DEPENDENCIES_H
#else
#	include <Z/hardware/CPU/architecture/6502.h>
#endif

/** 6502 emulator instance object.
  * @details This structure contains the state of the emulated CPU and callback
  * pointers necessary to interconnect the emulator with external logic. There
  * is no constructor function, so, before using an object of this type, some
  * of its members must be initialized, in particular the following:
  * @c context, @c read and @c write. */

typedef struct {

	/** Number of cycles executed in the current call to @c m6502_run.
	  * @details @c m6502_run sets this variable to 0 before starting to
	  * execute instructions and its value persists after returning. The
	  * callbacks can use this variable to know during what cycle they are
	  * being called. */

	zusize cycles;

	/** The value used as the first argument when calling a callback.
	  * @details This variable should be initialized before using the
	  * emulator and can be used to reference the context/instance of
	  * the machine being emulated. */

	void *context;

	/** Callback: Called when the CPU needs to read 8 bits from memory.
	  * @param context The value of the member @c context.
	  * @param address The memory address to read from.
	  * @return The 8 bits read from memory. */

	zuint8 (* read)(void *context, zuint16 address);

	/** Callback: Called when the CPU needs to write 8 bits to memory.
	  * @param context The value of the member @c context.
	  * @param address The memory address to write to.
	  * @param value The value to write. */

	void (* write)(void *context, zuint16 address, zuint8 value);

	/** CPU registers and internal bits.
	  * @details It contains the state of the registers and the interrupt
	  * flags. This is what a debugger should use as its data source. */

	Z6502State state;

	/** Temporary storage for memory addressing mode selection.
	  * @details This is an internal private variable. */

	zuint8 opcode;

	/** Temporary storage for memory addressing.
	  * @details This is an internal private variable. */

	zuint16 ea;
	zuint8  ea_cycles;
} M6502;

Z_C_SYMBOLS_BEGIN

#ifndef CPU_6502_API
#	ifdef CPU_6502_STATIC
#		define CPU_6502_API
#	else
#		define CPU_6502_API Z_API
#	endif
#endif

/** Changes the CPU power status.
  * @param object A pointer to a 6502 emulator instance object.
  * @param state @c TRUE = power ON; @c FALSE = power OFF. */

CPU_6502_API void m6502_power(M6502 *object, zboolean state);

/** Resets the CPU.
  * @details This is equivalent to a pulse in the RESET line of a real 6502.
  * @param object A pointer to a 6502 emulator instance object. */

CPU_6502_API void m6502_reset(M6502 *object);

/** Runs the CPU for a given number of @p cycles.
  * @param object A pointer to a 6502 emulator instance object.
  * @param cycles The number of cycles to be executed.
  * @return The number of cycles executed.
  * @note Given the fact that one 6502 instruction needs between 2 and 7 cycles
  * to be executed, it's not always possible to run the CPU the exact number of
  * @p cycles specfified. */

CPU_6502_API zusize m6502_run(M6502 *object, zusize cycles);

/** Performs a non-maskable interrupt (NMI).
  * @details This is equivalent to a pulse in the NMI line of a real 6502.
  * @param object A pointer to a 6502 emulator instance object. */

CPU_6502_API void m6502_nmi(M6502 *object);

/** Changes the state of the maskable interrupt (IRQ).
  * @details This is equivalent to a change in the IRQ line of a real 6502.
  * @param object A pointer to a 6502 emulator instance object.
  * @param state @c TRUE = line high; @c FALSE = line low. */

CPU_6502_API void m6502_irq(M6502 *object, zboolean state);

Z_C_SYMBOLS_END

#ifdef CPU_6502_WITH_ABI

#	ifndef CPU_6502_DEPENDENCIES_H
#		include <Z/ABIs/generic/emulation.h>
#	endif

	Z_C_SYMBOLS_BEGIN

#	ifndef CPU_6502_ABI
#		ifdef CPU_6502_STATIC
#			define CPU_6502_ABI
#		else
#			define CPU_6502_ABI Z_API
#		endif
#	endif

	CPU_6502_ABI extern ZCPUEmulatorABI const abi_emulation_cpu_6502;

	Z_C_SYMBOLS_END

#endif

#endif /* _emulation_CPU_6502_H_ */

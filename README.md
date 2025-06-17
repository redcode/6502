![al-tag](http://upload.wikimedia.org/wikipedia/commons/4/49/MOS_6502AD_4585_top.jpg)

<br>

# MOS Technology 6502 CPU Emulator
Copyright © 1999-2025 Manuel Sainz de Baranda y Goñi.  
Released under the terms of the [GNU Lesser General Public License](https://www.gnu.org/licenses/lgpl-3.0.html).

This is a very accurate [6502](http://en.wikipedia.org/wiki/MOS_Technology_6502) [emulator](http://en.wikipedia.org/wiki/Emulator) I wrote many years ago. It has been used in several machine emulators by other people and it has been extensivelly tested. It is fast, small (17 KB when compiled as a x86-64 dynamic library), easy to understand, and the code is commented.

If you are looking for a MOS 6502 CPU emulator for your project maybe you have found the correct one. I use this core in the [Nintendo Entertainment System emulator](http://github.com/redcode/NEStalin) I started as hobby.

<br>

## Building

You must first install [Z](https://github.com/redcode/Z), a **header-only** library that provides types and macros. This is the only dependency, the emulator does not use the C standard library or its headers. Then add `6502.h` and `6502.c` to your project and configure its build system so that `CPU_6502_STATIC` and `CPU_6502_USE_LOCAL_HEADER` are predefined when compiling the sources.

If you preffer to build the emulator as a library, you can use [premake4](http://premake.github.io):
```console
$ cd building
$ premake4 gmake                         # generate Makefile
$ make help                              # list available targets
$ make [config=<configuration>] [target] # build the emulator
```

There is also an Xcode project in `development/Xcode` with several targets:

Target | Description
--- | ---
6502 (dynamic) | Shared library.
6502 (dynamic module)  | Shared library with a generic module ABI to be used in modular multi-machine emulators.
6502 (static) | Static library.
6502 (static module) | Static library with a generic CPU emulator ABI to be used in monolithic multi-machine emulators.

<br>

## Code configuration

There are some predefined macros that control the compilation:

Name | Description
--- | ---
`CPU_6502_DEPENDENCIES_H` | If defined, it replaces the inclusion of any external header with this one. If you don't want to use Z, you can provide your own header with the types and macros used by the emulator.
`CPU_6502_HIDE_ABI` | Makes the generic CPU emulator ABI private.
`CPU_6502_HIDE_API` | Makes the public functions private.
`CPU_6502_STATIC` | You need to define this to compile or use the emulator as a static library or if you have added `6502.h` and `6502.c` to your project.
`CPU_6502_USE_LOCAL_HEADER` | Use this if you have imported `6502.h` and `6502.c` to your project. `6502.c` will `#include "6502.h"` instead of `<emulation/CPU/6502.h>`.
`CPU_6502_WITH_ABI` | Builds the generic CPU emulator ABI and declares its prototype in `6502.h`.
`CPU_6502_WITH_MODULE_ABI` | Builds the generic module ABI. This macro also enables `CPU_6502_WITH_ABI`, so the generic CPU emulator ABI will be built too. This option is intended to be used when building a true module loadable at runtime with `dlopen()`, `LoadLibrary()` or similar. The ABI module can be accessed via the [weak symbol](http://en.wikipedia.org/wiki/Weak_symbol) `__module_abi__`.

<br>

## API: `M6502` emulator instance

This structure contains the state of the emulated CPU and callback pointers necessary to interconnect the emulator with external logic. There is no constructor function, so, before using an object of this type, some of its members must be initialized, in particular the following: `context`, `read` and `write`.  

```C
zusize cycles;
```
**Description**  
Number of cycles executed in the current call to `m6502_run`.  
**Details**  
`m6502_run` sets this variable to `0` before starting to execute instructions and its value persists after returning. The callbacks can use this variable to know during what cycle they are being called.  

```C
void *context;
```
**Description**  
The value used as the first argument when calling a callback.  
**Details**  
This variable should be initialized before using the emulator and can be used to reference the context/instance of the machine being emulated.  

```C
zuint8 (* read)(void *context, zuint16 address);
```
**Description**  
Callback: Called when the CPU needs to read 8 bits from memory.  
**Parameters**  
`context` → The value of the member `context`.  
`address` → The memory address to read from.  
**Returns**  
The 8 bits read from memory.  

```C
void (* write)(void *context, zuint16 address, zuint8 value);
```
**Description**  
Callback: Called when the CPU needs to write 8 bits to memory.  
**Parameters**  
`context` → The value of the member `context`.  
`address` → The memory address to write to.  
`value` → The value to write.  

```C
Z6502State state;
```
**Description**  
CPU registers and internal bits.  
**Details**  
It contains the state of the registers and the interrupt flags. This is what a debugger should use as its data source.  

```C
zuint8 opcode;
```
**Description**  
Temporary storage for memory address resolution.  
**Details**  
This is an internal private variable.  

```C
zuint8 ea_cycles;
```
**Description**  
Temporary storage for the number of cycles consumed by instructions requiring memory address resolution.  
**Details**  
This is an internal private variable.  

```C
zuint16 ea;
```
**Description**  
Temporary storage for the resolved memory address.  
**Details**  
This is an internal private variable.  

<br>

## API: Public Functions

```C
void m6502_power(M6502 *object, zboolean state);
```
**Description**  
Changes the CPU power status.  
**Parameters**  
`object` → A pointer to a 6502 emulator instance.  
`state` → `TRUE` = power ON; `FALSE` = power OFF.  

```C
void m6502_reset(M6502 *object);
```
**Description**  
Resets the CPU.  
**Details**  
This is equivalent to a pulse on the `RESET` line of a real 6502.  
**Parameters**  
`object` → A pointer to a 6502 emulator instance.  

```C
zusize m6502_run(M6502 *object, zusize cycles);
```
**Description**  
Runs the CPU for a given number of `cycles`.  
**Note**  
Given the fact that one 6502 instruction needs between 2 and 7 cycles to be executed, it's not always possible to run the CPU the exact number of `cycles` specfified.  
**Parameters**  
`object` → A pointer to a 6502 emulator instance.  
`cycles` → The number of cycles to be executed.  
**Returns**  
The number of cycles executed.  

```C
void m6502_nmi(M6502 *object);
```
**Description**  
Performs a non-maskable interrupt (NMI).  
**Details**  
This is equivalent to a pulse on the `NMI` line of a real 6502.  
**Parameters**  
`object` → A pointer to a 6502 emulator instance.  

```C
void m6502_irq(M6502 *object, zboolean state);
```
**Description**  
Changes the state of the maskable interrupt (IRQ).  
**Details**  
This is equivalent to a change on the `IRQ` line of a real 6502.  
**Parameters**  
`object` → A pointer to a 6502 emulator instance.  
`state` → `TRUE` = line high; `FALSE` = line low.  

<br>

## Use in Proprietary Software

This library is released under the terms of the [GNU Lesser General Public License](https://www.gnu.org/licenses/lgpl-3.0.html) as published by the [Free Software Foundation](https://www.fsf.org).

# MOS Technology 6502 CPU Emulator
Copyright © 1999-2018 Manuel Sainz de Baranda y Goñi.  
Released under the terms of the [GNU General Public License v3](http://www.gnu.org/copyleft/gpl.html).

This is a very accurate [6502](http://en.wikipedia.org/wiki/MOS_Technology_6502) [emulator](http://en.wikipedia.org/wiki/Emulator) I wrote many years ago. It's very fast and small (although there are faster ones written in assembly), its structure is very clear and the code is commented.

If you are looking for an accurate MOS Technology 6502 CPU emulator for your project maybe you have found the correct one. I use this core in the [Nintendo Entertainment System emulator](http://github.com/redcode/NEStalin) I started as hobby.


## Building

In order to compile you must install [Z](http://github.com/redcode/Z), a **header only** library which provides types, macros, inline functions, and a lot of utilities to detect the particularities of the compiler and the target system. This is the only dependency, the standard C library and its headers are not used and the emulator doesn't need to be dynamically linked with any library.

A Xcode project is provided to build the emulator. It has the following targets:

#### Targets

Name | Description
--- | ---
dynamic | Shared library.
dynamic module  | Shared library with a module ABI to be used in modular multi-machine emulators.
static | Static library.
static module | Static library with a descriptive ABI to be used in monolithic multi-machine emulators.

#### Constants used in 6502.h

Name | Description
--- | ---
CPU_6502_STATIC | You need to define this if you are using the emulator as a static library or if you have added its sources to your project.

#### Constants used in 6502.c
Name | Description
--- | ---
CPU_6502_BUILD_ABI | Builds the ABI of type `ZCPUEmulatorABI` declared in the header with the identifier `abi_emulation_cpu_z80`.
CPU_6502_BUILD_MODULE_ABI | Builds a generic module ABI of type `ZModuleABI`. This constant enables `CPU_6502_BUILD_ABI` automatically so `abi_emulation_cpu_z80` will be build too. This option is intended to be used when building a true module loadable at runtime with `dlopen()`, `LoadLibrary()` or similar. The module ABI can be accessed retrieving the **weak** symbol `__module_abi__`.
CPU_6502_HIDE_API | Makes the API functions private.
CPU_6502_HIDE_ABI | Makes the `abi_emulation_cpu_6502` private.
CPU_6502_USE_LOCAL_HEADER | Use this if you have imported _6502.h_ and _6502.c_ to your project. _6502.c_ will include `"6502.h"` instead of `<emulation/CPU/6502.h>`.


## API

#### `m6502_power`

**Description**  
Switchs the CPU power status.   

**Prototype**  
```C
void m6502_power(M6502 *object, zboolean state);
```
**Parameters**  
`object` → A pointer to an emulator instance.  
`state` → `ON` / `OFF`  

#### `m6502_reset`

**Description**  
Resets the CPU by reinitializing its variables and sets its registers to the state they would be in a real 6502 CPU after a pulse in the `RESET` line.   

**Prototype**
```C
void m6502_reset(M6502 *object);
```

**Parameters**  
`object` → A pointer to an emulator instance.  

#### `m6502_run`

**Description**  
Runs the CPU for the given number of ```cycles```.   

**Prototype**  
```C
zusize m6502_run(M6502 *object, zusize cycles);
```

**Parameters**  
`object` → A pointer to an emulator instance.  
`cycles` → The number of cycles to be executed.  

**Return value**  
The number of cycles executed.   

**Discusion**  
Given the fact that one 6502 instruction needs between 2 and 7 cycles to be executed, it is not always possible to run the CPU the exact number of cycles specfified.   

#### `m6502_nmi`

**Description**  
Performs a non-maskable interrupt. This is equivalent to a pulse in the `NMI` line of a real 6502 CPU.   

**Prototype**  
```C
void m6502_nmi(M6502 *object);
```

**Parameters**  
`object` → A pointer to an emulator instance.  

#### `m6502_irq`

**Description**  
Switchs the state of the maskable interrupt. This is equivalent to a change in the `IRQ` line of a real 6502 CPU.   

**Prototype**  
```C
void m6502_irq(M6502 *object, zboolean state);
```

**Parameters**  
`object` → A pointer to an emulator instance.  
`state` → `ON` = set line high, `OFF` = set line low  


## Callbacks

Before using an instance of the 6502 emulator, its callback pointers must be initialized with the pointers to the functions that your program must provide in order to make possible for the CPU to access the emulated machine's resources.

#### `read` 

**Description**  
Called when the CPU needs to read 8 bits from memory.   

**Prototype**  
```C
typedef zuint8 (* ZContext16BitAddressRead8Bit)(void *context, zuint16 address);
ZContext16BitAddressRead8Bit read;
```

**Parameters**  
`context` → A pointer to the calling emulator instance.  
`address` → The memory address to read.  

**Return value**  
The 8 bits read from memory.   

#### `write`

**Description**  
Called when the CPU needs to write 8 bits to memory.   

**Prototype**  
```C
typedef void (* ZContext16BitAddressWrite8Bit)(void *context, zuint16 address, zuint8 value);
ZContext16BitAddressWrite8Bit write;
```

**Parameters**  
`context` → A pointer to the calling emulator instance.  
`address` → The memory address to write.  
`value` → The value to write in `address`.  

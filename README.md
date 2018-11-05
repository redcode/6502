![al-tag](https://upload.wikimedia.org/wikipedia/commons/4/49/MOS_6502AD_4585_top.jpg)

<br>

# MOS Technology 6502 CPU Emulator
Copyright © 1999-2018 Manuel Sainz de Baranda y Goñi.  
Released under the terms of the [GNU General Public License v3](http://www.gnu.org/copyleft/gpl.html).

This is a very accurate [6502](http://en.wikipedia.org/wiki/MOS_Technology_6502) [emulator](http://en.wikipedia.org/wiki/Emulator) I wrote many years ago. It's very fast and small (although there are faster ones written in assembly), its structure is very clear and the code is commented.

If you are looking for an accurate MOS Technology 6502 CPU emulator for your project maybe you have found the correct one. I use this core in the [Nintendo Entertainment System emulator](http://github.com/redcode/NEStalin) I started as hobby.


## Building

In order to compile you must install [Z](http://github.com/redcode/Z), a **header only** library which provides types, macros, inline functions, and a lot of utilities to detect the particularities of the compiler and the target system at compile time. This is the only dependency, the standard C library and its headers are not used and the emulator doesn't need to be dynamically linked against any library.

A Xcode project is provided to build the emulator. It has the following targets:

#### Targets

Name | Description
--- | ---
dynamic | Shared library.
dynamic module  | Shared library with a module ABI to be used in modular multi-machine emulators.
static | Static library.
static module | Static library with a descriptive ABI to be used in monolithic multi-machine emulators.

#### Code configuration

There are some predefined macros that control the compilation:

Name | Description
--- | ---
`CPU_6502_DEPENDENCIES_H` | If defined, `6502.h` will `#include` only this header as dependency. If you don't want to use Z, you can provide your own header with the types and macros used by the emulator.
`CPU_6502_HIDE_ABI` | Makes the generic CPU emulator ABI private.
`CPU_6502_HIDE_API` | Makes the public functions private.
`CPU_6502_STATIC` | You need to define this to compile or use the emulator as a static library or if you have added `6502.h` and `6502.c` to your project.
`CPU_6502_USE_LOCAL_HEADER` | Use this if you have imported `6502.h` and `6502.c` to your project. `6502.c` will `#include "6502.h"` instead of `<emulation/CPU/6502.h>`.
`CPU_6502_WITH_ABI` | Builds the generic CPU emulator ABI and declares its prototype in `6502.h`.
`CPU_6502_WITH_MODULE_ABI` | Builds the generic module ABI. This macro also enables CPU_6502_BUILD_ABI, so the generic CPU emulator ABI will be built too. This option is intended to be used when building a true module loadable at runtime with `dlopen()`, `LoadLibrary()` or similar. The ABI module can be accessed via the [weak symbol](https://en.wikipedia.org/wiki/Weak_symbol) `__module_abi__`.


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
`state` → `TRUE` for power ON, `FALSE` otherwise.  

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
`state` → `TRUE` = set line high, `FALSE` = set line low  


## Callbacks

Before using an instance of the 6502 emulator, its callback pointers must be initialized with the pointers to the functions that your program must provide in order to make possible for the CPU to access the emulated machine's resources.

#### `read` 

**Description**  
Called when the CPU needs to read 8 bits from memory.   

**Prototype**  
```C
zuint8 (* read )(void *context, zuint16 address);
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
void (* write)(void *context, zuint16 address, zuint8 value);
```

**Parameters**  
`context` → A pointer to the calling emulator instance.  
`address` → The memory address to write.  
`value` → The value to write in `address`.  

# cppAT
A lightweight C++ AT command library for embedded applications.

## Requirements

Compiler: C++ 20
Tested with GCC 10.3.1 arm-none-eabi gcc/g++ and GCC 11.4.0 x86_64-linux-gnu gcc/g++.

## Remapping Printf

Put the following snippet somewhere in your code where you have included `cpp_at.hh` so that CppAT can have access to
a printf function for writing its output. If you don't want to use the stdlib printf, you could consider a lightweight
embedded printf library [like this one](https://github.com/mpaland/printf). Any printf implementation should be fine
as long as it includes an implementation of vprintf, which accepts a variable length list of arguments.

```c++
// For mapping cpp_at_printf
#include <cstdarg>
#include <cstdio>

int CppAT::cpp_at_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int res = vprintf(format, args);
    va_end(args);
    return res;
}
```

## Helpful Hints

Example of binding a callback function to a particular instance of a class using std::bind.

```c++
ATCommandDef_t def = {
    .command = "+MTLREAD",
    .min_args = 0,
    .max_args = 0,
    .help_string = "Read ADC counts and mV values for high and low MTL thresholds. Call with no ops nor arguments, AT+MTLREAD.\r\n",
    .callback = std::bind(
        &ADSBee::ATMTLReadCallback,
        this,
        std::placeholders::_1, 
        std::placeholders::_2,
        std::placeholders::_3
    )
}
```
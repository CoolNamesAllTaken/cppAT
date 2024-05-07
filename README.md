# cppAT
A lightweight C++ AT command library for embedded applications.

## Requirements

Compiler: C++ 20
Tested with GCC 10.3.1 arm-none-eabi gcc/g++ and GCC 11.4.0 x86_64-linux-gnu gcc/g++.

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
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
#include "printf.h" // for using custom printf defined in printf.h
// #include <cstdio> // for using regular printf

int CppAT::cpp_at_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int res = vprintf(format, args);
    va_end(args);
    return res;
}
```

## Binding Callbacks to ATCommandDef_t's

### Creating a callback function

Callback functions are executed when an AT command matching a given ATCommandDef_t is received. Callback functions can be freestanding (not inside a class), or a member function of a class.

#### Freestanding callback function example:

This function parses an AT command for a single argument, which it stores in the global variable at_config_mode.

```c++
/**
 * AT+CONFIG Callback
 * AT+CONFIG=<config_mode:uint16_t>
 *  config_mode = 0 (print as normal), 1 (suppress non-configuration print messages)
 */
CPP_AT_CALLBACK(ATConfigCallback) {
    if (op == '?') {
        // AT+CONFIG mode query.
        CPP_AT_PRINTF("=%d", at_config_mode);
    } else if (op == '=') {
        // AT+CONFIG set command.
        if (!CPP_AT_HAS_ARG(0)) {
            CPP_AT_ERROR("Need to specify a config mode to run.")
        }
        ATConfigMode new_mode;
        CPP_AT_TRY_ARG2NUM(0, (uint16_t &)new_mode);
        if (new_mode >= ATConfigMode::kInvalid) {
            CPP_AT_ERROR("%d is not a valid config mode.", (uint16_t)new_mode);
        }

        at_config_mode = new_mode;
        CPP_AT_SUCCESS()
    }
    return true;
}

ATCommandDef_t def = {
    .command = "+CONFIG",
    .min_args = 0,
    .max_args = 0,
    .help_string = "AT+CONFIG=<at_config_mode>: Set the value of at_config_mode.\r\n",
    .callback = ATConfigCallback
}
```

#### Member function callback example:

This function parses an AT command for a single argument, which it stores in the member variable at_config_mode_.

```c++
/**
 * AT+CONFIG Callback
 * AT+CONFIG=<config_mode:uint16_t>
 *  config_mode = 0 (print as normal), 1 (suppress non-configuration print messages)
 */
bool CommsManager::ATConfigCallback(CPP_AT_CALLBACK_ARGS) {
    if (op == '?') {
        // AT+CONFIG mode query.
        CPP_AT_PRINTF("=%d", at_config_mode_);
    } else if (op == '=') {
        // AT+CONFIG set command.
        if (!CPP_AT_HAS_ARG(0)) {
            CPP_AT_ERROR("Need to specify a config mode to run.")
        }
        ATConfigMode new_mode;
        CPP_AT_TRY_ARG2NUM(0, (uint16_t &)new_mode);
        if (new_mode >= ATConfigMode::kInvalid) {
            CPP_AT_ERROR("%d is not a valid config mode.", (uint16_t)new_mode);
        }

        at_config_mode_ = new_mode;
        CPP_AT_SUCCESS()
    }
    return true;
}

// Note that instance_of_comms_manager is a CommsManager instance.
ATCommandDef_t def = {
    .command = "+CONFIG",
    .min_args = 0,
    .max_args = 0,
    .help_string = "AT+CONFIG=<at_config_mode>: Set the value of at_config_mode.\r\n",
    .callback = CPP_AT_BIND_MEMBER_CALLBACK(CommsManager::ATMTLReadCallback, instance_of_comms_manager)
}
```

## Troubleshooting

* During linking, receive an error saying "Undefined reference to `CppAT::cpp_at_printf(char const*, ...)`".
    * Make sure to implement the cpp_at_printf function! See the [Remapping Printf](#remapping-printf) section.
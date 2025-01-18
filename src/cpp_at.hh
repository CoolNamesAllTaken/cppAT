#ifndef _CPP_AT_HH_
#define _CPP_AT_HH_

#include <cctype> // for std::isspace()
#include <functional>
#include <string_view>
#include <vector>
#include <type_traits> // For checking tyupe of a template.
#include "cpp_at_settings.hh"
#include "stdint.h"

class CppAT
{
public:
    static constexpr uint16_t kATCommandMaxLen = CPP_AT_COMMAND_MAX_LEN;
    // Initialized in .cc file.
    static const char kATPrefix[];
    static const uint16_t kATPrefixLen;
    static const char kATAllowedOpChars[];
    static constexpr uint16_t kHelpStringMaxLen = CPP_AT_HELP_STR_MAX_LEN;
    static constexpr uint16_t kArgMaxLen = CPP_AT_ARG_MAX_LEN;
    static constexpr char kArgDelimiter = ',';
    static constexpr uint16_t kMaxNumArgs = CPP_AT_MAX_NUM_ARGS;
    static const char kATMessageEndStr[];

    struct ATCommandDef_t
    {
        char command_buf[kATCommandMaxLen + 1] = ""; // leave room for '\0'
        std::string_view command = {command_buf};    // Letters that come after the "AT+" prefix.
        uint16_t min_args = 0;                       // Minimum number of arguments to expect after AT+<command>.
        uint16_t max_args = 100;                     // Maximum number of arguments to expect after AT+<command>.
        char help_string_buf[kHelpStringMaxLen + 1] =
            "Help string not defined."; // Text to print when listing available AT commands.
        std::string_view help_string = {help_string_buf};
        std::function<void(void)> help_callback = nullptr; // Optional function to use for printing help string instead of help_string.
        std::function<bool(const ATCommandDef_t &, char, const std::string_view[], uint16_t)> callback =
            nullptr; // Function to call with list of arguments when an AT command is received.
    };

    CppAT(); // default constructor

    /**
     * @brief Constructor.
     * @param[in] at_command_list_in Array of ATCommandDef_t's that define what AT commands are supported
     * as well as their corresponding callback functions.
     * @param[in] num_at_comands_in Length of at_command_list_in.
     * @param[in] at_command_list_is_static Optional boolean indicating whether the at_command_list is statically
     * allocated and can be used directly, or whether new space needs to be allocated for it in dynamic memory. NOTE: An
     * AT+HELP function will not be automatically generated if this is set to true.
     * @retval Your shiny new CppAT object.
     */
    CppAT(const ATCommandDef_t *at_command_list_in, uint16_t num_at_commands_in,
          bool at_command_list_is_static = false); // Constructor.

    /**
     * @brief Destructor. Deallocates dynamically allocated memory.
     */
    ~CppAT();

    /**
     * @brief Helper function that clears existing AT commands and populates with a new list of AT Command definitions.
     * Adds a definition for AT+HELP.
     * @param[in] at_command_list_in Array of ATCommandDef_t's that define what AT commands are supported
     * as well as their corresponding callback functions.
     * @param[in] num_at_commands Number of elements in at_command_list_in array.
     * @param[in] at_command_list_is_static Optional boolean indicating whether at_command_list can be referenced in
     * place. If not, memory will be dynamically allocated to store the contents of at_command_list.
     * @retval True if set successfully, false if failed.
     */
    bool SetATCommandList(const ATCommandDef_t *at_command_list_in, uint16_t num_at_commands_in,
                          bool at_command_list_is_static = false);

    /**
     * @brief Returns the number of supported AT commands, not counting the auto-generated AT+HELP command.
     * @retval Size of at_command_list_.
     */
    uint16_t GetNumATCommands();

    /**
     * @brief Returns a pointer to the first ATCommandDef_t object that matches the text command provided.
     * @param[in] command String containing command text to look for.
     * @retval Pointer to corresponding ATCommandDef_t within the at_command_list_, or nullptr if not found.
     */
    const ATCommandDef_t *LookupATCommand(std::string_view command);

    /**
     * @brief Parses a message to find the AT command, match it with the relevant ATCommandDef_t, parse
     * out the arguments and execute the corresponding callback function.
     * @param[in] std::string_view containing text to parse.
     * @retval True if parsing successful, false otherwise.
     */
    bool ParseMessage(std::string_view message);

    bool is_valid = false;

    /**
     * @brief Turns a string_view argument into a float or int / unsigned int.
     * @param[in] arg string_view containing the value to parse.
     * @param[out] number Reference to a uint, int, or float to write the value into.
     * @param[in] base Optional argument indicating the base to use when parsing. Defaults to 10 (decimal).
     * @retval True if parsing was successful, false otherwise.
     */
    template <typename T>
    static inline bool ArgToNum(const std::string_view arg, T &number, uint16_t base = 10)
    {
        char *end_ptr;
        const char *arg_ptr = arg.data();

        // Remove leading whitespace.
        while (std::isspace(*arg_ptr))
        {
            ++arg_ptr;
        }

        // Try parsing integer.
        bool arg_is_signed = false;
        int32_t parsed_int;
        uint32_t parsed_uint;
        if (*arg_ptr == '-')
        {
            // Parse signed integer.
            arg_is_signed = true;
            if (!std::is_signed_v<T>)
            {
                return false; // Can't store negative value in unsigned variable.
            }
            parsed_int = strtol(arg_ptr, &end_ptr, base);
        }
        else
        {
            // Parse unsigned integer.
            parsed_uint = strtoul(arg_ptr, &end_ptr, base);
        }
        if (end_ptr != arg_ptr)
        {
            while (std::isspace(*end_ptr))
            {
                ++end_ptr;
            }
            if (*end_ptr == '\0')
            {
                // NOTE: This may cause unexpected results if type is unsigned but parsed value is signed!
                number = static_cast<T>(arg_is_signed ? parsed_int : parsed_uint);
                if ((arg_is_signed && static_cast<int32_t>(number) != parsed_int)       // Overflowed signed integer.
                    || (!arg_is_signed && static_cast<uint32_t>(number) != parsed_uint) // Overflowed unsigned integer.
                )
                {
                    // Value overflowed the bounds of number.
                    return false;
                }
                return true;
            }
            else
            {
                // There are numbers or text after the decimal, try parsing float.
                float parsed_float = strtof(arg_ptr, &end_ptr);
                if (end_ptr != arg_ptr && *end_ptr == '\0')
                {
                    number = static_cast<T>(parsed_float);
                    return true;
                }
            }
        }
        // Arg is blank or failed to parse.
        return false;
    }

    bool ATHelpCallback(const ATCommandDef_t &def, char op, const std::string_view args[], uint16_t num_args);
    const ATCommandDef_t at_help_command = {
        .command_buf = "+HELP",
        .min_args = 0,
        .max_args = 0,
        .help_string_buf = "Display this menu.\r\n",
        .callback = std::bind(&CppAT::ATHelpCallback, this, std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4)};

    /**
     * @brief printf handle used by CppAT.
     * @param[in] Format string used for printing.
     * @param[in] ... Variable length list of arguments.
     * @retval The number of characters printed, or EOF if an error occurred.
     */
    static int cpp_at_printf(const char *format, ...);

private:
    // Non readonly handle for at_command_list_ used when it is dynamically allocated into memory.
    ATCommandDef_t *at_command_list_ = nullptr;
    // Readonly handle for at_command_list_ used everywhere except where it is set.
    const ATCommandDef_t *at_command_list_ro_ = nullptr;
    uint16_t num_at_commands_;
};

/** CppAT Convenience Macros */
// NOTE: do {} while (false) structure is used on standalone multi-line macros to create a single block and force use of
// a semicolon after calling the macro.

#define CPP_AT_HAS_ARG(n) (num_args > (n) && !args[(n)].empty())

#define CPP_AT_TRY_ARG2NUM(args_index, num)                                          \
    do                                                                               \
    {                                                                                \
        if (!CppAT::ArgToNum(args[(args_index)], (num)))                             \
        {                                                                            \
            CppAT::cpp_at_printf("Error converting argument %d.\r\n", (args_index)); \
            return false;                                                            \
        }                                                                            \
    } while (false)

#define CPP_AT_TRY_ARG2NUM_BASE(args_index, num, base)                                                    \
    do                                                                                                    \
    {                                                                                                     \
        if (!CppAT::ArgToNum(args[(args_index)], (num), (base)))                                          \
        {                                                                                                 \
            CppAT::cpp_at_printf("Error converting argument %d with base %d.\r\n", (args_index), (base)); \
            return false;                                                                                 \
        }                                                                                                 \
    } while (false)

#define CPP_AT_SUCCESS()                \
    do                                  \
    {                                   \
        CppAT::cpp_at_printf("OK\r\n"); \
        return true;                    \
    } while (false)

#define CPP_AT_SILENT_SUCCESS() return true

#define CPP_AT_ERROR(format, ...)                                                \
    do                                                                           \
    {                                                                            \
        CppAT::cpp_at_printf("ERROR " format "\r\n" __VA_OPT__(, ) __VA_ARGS__); \
        return false;                                                            \
    } while (false)

#define CPP_AT_CALLBACK(callback_name) \
    bool callback_name(const CppAT::ATCommandDef_t &def, char op, const std::string_view args[], uint16_t num_args)

#define CPP_AT_HELP_CALLBACK(callback_name) void callback_name()

#define CPP_AT_BIND_MEMBER_CALLBACK(callback, instance)                           \
    std::bind(&callback, &instance, std::placeholders::_1, std::placeholders::_2, \
              std::placeholders::_3, std::placeholders::_4)

#define CPP_AT_BIND_MEMBER_HELP_CALLBACK(callback, instance) \
    std::bind(&callback, &instance)

#define CPP_AT_CMD_PRINTF(format, ...) \
    CppAT::cpp_at_printf("%s" format "\r\n", def.command.data() __VA_OPT__(, ) __VA_ARGS__)

#define CPP_AT_PRINTF(format, ...) \
    CppAT::cpp_at_printf(format __VA_OPT__(, ) __VA_ARGS__)

#endif /* _CPP_AT_HH_ */
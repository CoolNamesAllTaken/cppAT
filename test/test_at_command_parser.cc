#include "gtest/gtest.h"
#include "at_command_parser.hh"
#include <string_view>

bool callback1_was_called = false;
bool callback2_was_called = false;

bool Callback1(char op, const std::string_view args[], uint16_t num_args) {
    callback1_was_called = true;
    return true;
}

bool Callback2(char op, const std::string_view args[], uint16_t num_args) {
    callback2_was_called = true;
    return true;
}

TEST(ATCommandParser, SingleATCommand) {
    ATCommandParser::ATCommandDef_t at_command_list[] = {
        {
            .command = "+TEST",
            .min_args = 0,
            .max_args = 1,
            .help_string = "This is a test.",
            .callback = Callback1
        }
    };
    ATCommandParser parser = ATCommandParser(at_command_list, sizeof(at_command_list)/sizeof(at_command_list[0]));
    ASSERT_EQ(parser.GetNumATCommands(), 2); // HELP command automatically added.
    
    // Looking up a fake command should fail.
    ASSERT_TRUE(parser.LookupATCommand("+Blah") == nullptr);

    // Looking up a real command should work.
    ATCommandParser::ATCommandDef_t * returned_command = parser.LookupATCommand("+TEST");
    ASSERT_NE(returned_command, nullptr);
    ASSERT_TRUE(returned_command->command.compare("+TEST") == 0);
    callback1_was_called = false;
    std::string_view args[] = {{"arg1"}, {"arg2"}};
    uint16_t num_args = 2;
    returned_command->callback('=', args, num_args);
    ASSERT_TRUE(callback1_was_called);
    ASSERT_TRUE(returned_command->help_string.compare("This is a test.") == 0);
}

ATCommandParser BuildExampleParser1() {
    uint16_t num_at_commands = 2;
    ATCommandParser::ATCommandDef_t at_command_list[] = {
        {
            .command = "+TEST",
            .min_args = 0,
            .max_args = 1,
            .help_string = "This is a test.",
            .callback = Callback1
        },
        {
            .command = "+CFG",
            .min_args = 1,
            .max_args = 3,
            .help_string = "Configuration. Takes between 1 and 3 arguments.",
            .callback = Callback2
        }
    };
    return ATCommandParser(at_command_list, num_at_commands);
}

TEST(ATCommandParser, HelpString) {
    ATCommandParser parser = BuildExampleParser1();
    ASSERT_TRUE(parser.ParseMessage("AT+HELP\r\n"));
    // This test doesn't do anything other than make sure it doesn't crash when calling the callbacks in AT+HELP.
}

TEST(ATCommandParser, TwoATCommands) {
    ATCommandParser parser = BuildExampleParser1();
    ASSERT_TRUE(parser.is_valid);
    ASSERT_EQ(parser.GetNumATCommands(), 3);

    // Looking up a fake command should fail.
    ASSERT_TRUE(parser.LookupATCommand("+Potatoes") == nullptr);

    // Looking up real command 1 should work.
    ATCommandParser::ATCommandDef_t * returned_command = parser.LookupATCommand("+TEST");
    ASSERT_NE(returned_command, nullptr);
    ASSERT_TRUE(returned_command->command.compare("+TEST") == 0);
    callback1_was_called = false;
    std::string_view args1[] = {"arg1", "arg2"};
    uint16_t num_args1 = 2;
    returned_command->callback('=', args1, num_args1);
    ASSERT_TRUE(callback1_was_called);
    ASSERT_TRUE(returned_command->help_string.compare("This is a test.") == 0);

    // Looking up real command 2 should work.
    returned_command = parser.LookupATCommand("+CFG");
    ASSERT_NE(returned_command, nullptr);
    ASSERT_TRUE(returned_command->command.compare("+CFG") == 0);
    callback2_was_called = false;
    std::string_view args2[] = {"arg1", "arg2"};
    uint16_t num_args2 = 2;
    returned_command->callback('=', args2, num_args2);
    ASSERT_TRUE(callback2_was_called);
    ASSERT_TRUE(returned_command->help_string.compare("Configuration. Takes between 1 and 3 arguments.") == 0);
}

TEST(ATCommandParser, RejectMessageWithNoAT) {
    ATCommandParser parser = BuildExampleParser1();

    ASSERT_FALSE(parser.ParseMessage("Potatoes potatoes potatoes I love potatoes."));
    ASSERT_FALSE(parser.ParseMessage("A T just kidding."));
}

TEST(ATCommandParser, RejectMessageWithZeroLengthCommand) {
    ATCommandParser parser = BuildExampleParser1();

    ASSERT_FALSE(parser.ParseMessage("AT+ other words"));
    ASSERT_FALSE(parser.ParseMessage("AT+,other words"));
    ASSERT_FALSE(parser.ParseMessage("AT+=CFG"));
    ASSERT_FALSE(parser.ParseMessage("AT+\n"));
}

TEST(ATCommandParser, RejectMessageWithCommandTooLong) {
    // Build a parser that contains a command that is too long.
    ATCommandParser::ATCommandDef_t at_command_list[] = {
        {
            .command = "+HIHIHIHIHIHIHIHIHIHITOOLONG"
        }
    };
    ATCommandParser parser = ATCommandParser(at_command_list, sizeof(at_command_list)/sizeof(at_command_list[0]));
    ASSERT_FALSE(parser.is_valid);

    ASSERT_FALSE(parser.ParseMessage("AT+HIHIHIHIHIHIHIHIHIHITOOLONG"));
}

TEST(ATCommandParser, FailToInitWithHelpStringTooLong) {
    // Build a parser that contains a command that is too long.
    ATCommandParser::ATCommandDef_t at_command_list[] = {
        {
            .help_string = "NARRATOR:"
                "(Black screen with text; The sound of buzzing bees can be heard)"
                "According to all known laws"
                "of aviation,"
                ":"
                "there is no way a bee"
                "should be able to fly."
                ":"
                "Its wings are too small to get"
                "its fat little body off the ground."
                ":"
                "The bee, of course, flies anyway"
                ":"
                "because bees don't care"
                "what humans think is impossible."
                "BARRY BENSON:"
                "(Barry is picking out a shirt)"
                "Yellow, black. Yellow, black."
                "Yellow, black. Yellow, black."
                ":"
                "Ooh, black and yellow!"
                "Let's shake it up a little."
                "JANET BENSON:"
                "Barry! Breakfast is ready!"
                "BARRY:"
                "Coming!"
                ":"
                "Hang on a second."
                "(Barry uses his antenna like a phone)"
                ":"
                "Hello?"
                "ADAM FLAYMAN:"
                ""
                "(Through phone)"
                "- Barry?"
                "BARRY:"
                "- Adam?"
                "ADAM:"
                "- Can you believe this is happening?"
                "BARRY:"
                "- I can't. I'll pick you up."
                "(Barry flies down the stairs)"
                ":"
                "MARTIN BENSON:"
                "Looking sharp."
                "JANET:"
                "Use the stairs. Your father"
                "paid good money for those."
                "BARRY:"
                "Sorry. I'm excited."
                "MARTIN:"
                "Here's the graduate."
                "We're very proud of you, son."
                ":"
                "A perfect report card, all B's."
                "JANET:"
                "Very proud."
                "(Rubs Barry's hair)"
                "BARRY="
                "Ma! I got a thing going here."
                "JANET:"
                "- You got lint on your fuzz."
                "BARRY:"
                "- Ow! That's me!"
                ""
                "JANET:"
                "- Wave to us! We'll be in row 118,000."
                "- Bye!"
                "(Barry flies out the door)"
                "JANET:"
                "Barry, I told you,"
                "stop flying in the house!"
                "(Barry drives through the hive,and is waved at by Adam who is reading a"
                "newspaper)"
                "BARRY=="
                "- Hey, Adam."
                "ADAM:"
                "- Hey, Barry."
                "(Adam gets in Barry's car)"
                ":"
                "- Is that fuzz gel?"
                "BARRY:"
                "- A little. Special day, graduation."
                "ADAM:"
                "Never thought I'd make it."
                "(Barry pulls away from the house and continues driving)"
                "BARRY:"
                "Three days grade school,"
                "three days high school..."
                "ADAM:"
                "Those were awkward."
                "BARRY:"
                "Three days college. I'm glad I took"
                "a day and hitchhiked around the hive."
        }
    };
    ATCommandParser parser = ATCommandParser(at_command_list, sizeof(at_command_list)/sizeof(at_command_list[0]));
    ASSERT_FALSE(parser.is_valid);
}

TEST(ATCommandParser, RejectMessageWithNoMatchingATCommand) {
    ATCommandParser parser = BuildExampleParser1();

    ASSERT_FALSE(parser.ParseMessage("AT+WRONG"));
    ASSERT_FALSE(parser.ParseMessage("AT+\r\n"));
}

TEST(ATCommandParser, RejectMessageWithIncorrectNumberOfArgs) {
    ATCommandParser parser = BuildExampleParser1();

    // AT+TEST takes between 0-1 args.
    ASSERT_FALSE(parser.ParseMessage("AT+TEST=a,b")); // two args
    ASSERT_TRUE(parser.ParseMessage("AT+TEST=a")); // one arg
    ASSERT_TRUE(parser.ParseMessage("AT+TEST")); // no args
}

/**
 * @brief Callback function that returns true if the args are "potato" or "potato,bacon"
*/
bool MustBePotatoBacon(char op, const std::string_view args[], uint16_t num_args) {
    if (args[0].compare("potato") == 0) {
        if (num_args == 1) {
            return true;
        } else {
            return args[1].compare("bacon") == 0;
        }
    }
    return false;
}

ATCommandParser BuildPotatoBaconParser() {
    ATCommandParser::ATCommandDef_t at_command_list[] =  {
        {
            .command = "+POTATOBACON",
            .min_args = 1,
            .max_args = 2,
            .help_string = "Acceptable args are \"potato\" or \" potato,bacon\".",
            .callback = MustBePotatoBacon
        }
    };
    return ATCommandParser(at_command_list, sizeof(at_command_list)/sizeof(at_command_list[0]));
}

TEST(ATCommandParser, TwoArgsPotatoBacon) {
    ATCommandParser parser = BuildPotatoBaconParser();
    ASSERT_TRUE(parser.ParseMessage("AT+POTATOBACON=potato"));
    ASSERT_FALSE(parser.ParseMessage("AT+POTATOBACON=bacon"));
    ASSERT_TRUE(parser.ParseMessage("AT+POTATOBACON=potato,bacon"));
    ASSERT_FALSE(parser.ParseMessage("AT+POTATOBACON=potato,potato"));
}

bool PickyOpCallback(char op, const std::string_view args[], uint16_t num_args) {
    if (op == ' ' || op == '?') {
        return true;
    }
    return false;
}

TEST(ATCommandParser, PickyOpCallback) {
    ATCommandParser::ATCommandDef_t at_command_list[] = {
        {
            .command = "+PICKYOP",
            .min_args = 0,
            .max_args = 100,
            .help_string = "Doot doot whatever but make the op ' ' or '?'.",
            .callback = PickyOpCallback
        }
    };
    ATCommandParser parser = ATCommandParser(at_command_list, sizeof(at_command_list)/sizeof(at_command_list[0]));
    ASSERT_FALSE(parser.ParseMessage("AT+PICKYOP=doot\r\n"));
    ASSERT_TRUE(parser.ParseMessage("AT+PICKYOP doot\r\n"));
    ASSERT_TRUE(parser.ParseMessage("AT+PICKYOP?\r\n"));
    ASSERT_FALSE(parser.ParseMessage("AT+PICKYOP\r\n"));
}

std::vector<std::string_view> stored_args;
char stored_op;
bool StoreArgsCallback(char op, const std::string_view args[], uint16_t num_args) {
    stored_args.clear();
    stored_op = op;
    for (uint16_t i = 0; i < num_args; i++) {
        stored_args.push_back(args[i]);
    }
    return true;
}

ATCommandParser BuildStoreArgParser() {
    ATCommandParser::ATCommandDef_t at_command_list[] = {
        {
            .command = "+STORE",
            .min_args = 0,
            .max_args = 50,
            .help_string = "Stores all arguments it receives.",
            .callback = StoreArgsCallback
        }
    };
    return ATCommandParser(at_command_list, sizeof(at_command_list)/sizeof(at_command_list[0]));
}

TEST(ATCommandParser, StoreArgsWithoutReturns) {
    ATCommandParser parser = BuildStoreArgParser();
    
    // No Args
    parser.ParseMessage("AT+STORE\r\n");
    ASSERT_EQ(stored_args.size(), 0u);
    ASSERT_EQ(stored_op, '\0');
    
    // Question mark without newline.
    parser.ParseMessage("AT+STORE?");
    ASSERT_EQ(stored_args.size(), 0u);
    ASSERT_EQ(stored_op, '?');

    // Question mark with newline.
    parser.ParseMessage("AT+STORE?\r\n");
    ASSERT_EQ(stored_args.size(), 0u);
    ASSERT_EQ(stored_op, '?');

    // Question with space in an arg.
    parser.ParseMessage("AT+STORE?hello, potato");
    ASSERT_EQ(stored_args.size(), 2u);
    ASSERT_EQ(stored_op, '?');
    ASSERT_EQ(stored_args[0].compare("hello"), 0);
    ASSERT_EQ(stored_args[1].compare(" potato"), 0);

    // QUestion with ignored arg.
    parser.ParseMessage("AT+STORE=hello\r\n, bacon");
    ASSERT_EQ(stored_args.size(), 1u);
    ASSERT_EQ(stored_op, '=');
    ASSERT_EQ(stored_args[0].compare("hello"), 0);
}

TEST(ATCommandParser, AllowBlankArgs) {
    ATCommandParser parser = BuildStoreArgParser();

    parser.ParseMessage("AT+STORE=,,5,");
    ASSERT_EQ(stored_args.size(), (size_t)4);
    ASSERT_STREQ(stored_args[0].data(), "");
    ASSERT_STREQ(stored_args[1].data(), "");
    ASSERT_STREQ(stored_args[2].data(), "5");
    ASSERT_STREQ(stored_args[3].data(), "");
}
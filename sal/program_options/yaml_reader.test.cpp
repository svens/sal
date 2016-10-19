#include <sal/program_options/yaml_reader.hpp>
#include <sal/program_options/common.test.hpp>
#include <sstream>


namespace {

namespace po = sal::program_options;


struct yaml_reader
  : public sal_test::with_value<bool>
{
  void parse_and_check (size_t line,
    const std::string &content,
    const std::vector<std::pair<std::string, std::string>> &expected)
  {
    std::istringstream iss;
    iss.str(content);
    po::yaml_reader_t parser{iss};

    po::option_set_t options;
    std::string option, argument;
    std::vector<std::pair<std::string, std::string>> result;
    while (parser(options, &option, &argument))
    {
      result.emplace_back(option, argument);
      option.clear();
      argument.clear();
    }

    EXPECT_EQ(expected, result) << "(line " << line << ')';
  }
};


INSTANTIATE_TEST_CASE_P(program_options, yaml_reader, testing::Values(true));


// simple successful cases: https://goo.gl/MMRfgG


TEST_P(yaml_reader, comment) //{{{1
{
  parse_and_check(__LINE__, R"(
#
# to be ignored
#

)",
  {
  });
}


TEST_P(yaml_reader, option_no_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option:

)",
  {
    { "option", "" },
  });
}


TEST_P(yaml_reader, option_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_argument: argument

)",
  {
    { "option_argument", "argument" },
  });
}


TEST_P(yaml_reader, option_newline_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_newline_argument:
  argument

)",
  {
    { "option_newline_argument", "argument" },
  });
}


TEST_P(yaml_reader, option_argument_newline_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_argument_newline_argument: 1
  2

)",
  {
    { "option_argument_newline_argument", "1 2" },
  });
}


TEST_P(yaml_reader, option_multiline_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_multiline_argument:
  1
  2

)",
  {
    { "option_multiline_argument", "1 2" },
  });
}


TEST_P(yaml_reader, option_indented_multiline_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_indented_multiline_argument:
  1
    2
  3
  4

)",
  {
    { "option_indented_multiline_argument", "1 2 3 4" },
  });
}


TEST_P(yaml_reader, option_multiline_literal_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_multiline_literal_argument: |
  1
  2

)",
  {
    { "option_multiline_literal_argument", "1\n2\n" },
  });
}


TEST_P(yaml_reader, option_indented_multiline_literal_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_indented_multiline_literal_argument: |
  1
    2
  3
  4

)",
  {
    { "option_indented_multiline_literal_argument", "1\n  2\n3\n4\n" },
  });
}


TEST_P(yaml_reader, option_multiline_literal_keep_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_multiline_literal_keep_argument: |+
  1
  2

)",
  {
    { "option_multiline_literal_keep_argument", "1\n2\n\n" },
  });
}


TEST_P(yaml_reader, option_multiline_literal_strip_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_multiline_literal_strip_argument: |-
  1
  2

)",
  {
    { "option_multiline_literal_strip_argument", "1\n2" },
  });
}


TEST_P(yaml_reader, option_multiline_folded_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_multiline_folded_argument: >
  1
  2

)",
  {
    { "option_multiline_folded_argument", "1 2\n" },
  });
}


TEST_P(yaml_reader, option_indented_multiline_folded_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_indented_multiline_folded_argument: >
  1
    2
  3
  4

)",
  {
    { "option_indented_multiline_folded_argument", "1\n  2\n3 4\n" },
  });
}


TEST_P(yaml_reader, option_multiline_folded_keep_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_multiline_folded_keep_argument: >+
  1
  2

)",
  {
    { "option_multiline_folded_keep_argument", "1 2\n\n" },
  });
}


TEST_P(yaml_reader, option_multiline_folded_strip_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_multiline_folded_strip_argument: >-
  1
  2

)",
  {
    { "option_multiline_folded_strip_argument", "1 2" },
  });
}


#if 0
TEST_P(yaml_reader, option_list_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_list_argument:
  - 1
  - 2

)",
  {
    { "option_list_argument", "1" },
    { "option_list_argument", "2" },
  });
}


TEST_P(yaml_reader, option_list_argument_no_indent) //{{{1
{
  parse_and_check(__LINE__, R"(
option_list_argument_no_indent:
- 1
- 2

)",
  {
    { "option_list_argument_no_indent", "1" },
    { "option_list_argument_no_indent", "2" },
  });
}
#endif


TEST_P(yaml_reader, option_list_literal_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_list_literal_argument: |
  - 1
  - 2

)",
  {
    { "option_list_literal_argument", "- 1\n- 2\n" },
  });
}


TEST_P(yaml_reader, option_list_folded_argument) //{{{1
{
  parse_and_check(__LINE__, R"(
option_list_folded_argument: >
  - 1
  - 2

)",
  {
    { "option_list_folded_argument", "- 1 - 2\n" },
  });
}


//}}}1



// complex successful cases


TEST_P(yaml_reader, indented_comment) //{{{1
{
  parse_and_check(__LINE__, R"( # comment)", {});
}


TEST_P(yaml_reader, comment_after_colon) //{{{1
{
  parse_and_check(__LINE__, R"(
comment_after_colon: # comment
  argument
)",
  {
    { "comment_after_colon", "argument" },
  });
}


TEST_P(yaml_reader, comment_at_end_block)
{
  parse_and_check(__LINE__, R"(
comment_at_end_of_block:
  argument
  # comment
)",
  {
    { "comment_at_end_of_block", "argument" }
  });
}


TEST_P(yaml_reader, space_before_colon) //{{{1
{
  parse_and_check(__LINE__, R"(
space_before_colon : argument
)",
  {
    { "space_before_colon", "argument" },
  });
}


TEST_P(yaml_reader, option_literal_option) //{{{1
{
  parse_and_check(__LINE__, R"(
option_literal_option: >

option: argument
)",
  {
    { "option_literal_option", "" },
    { "option", "argument" },
  });
}


TEST_P(yaml_reader, chomp_without_style)
{
  parse_and_check(__LINE__, R"(
option1: +
option2: -
)",
  {
    { "option1", "+" },
    { "option2", "-" },
  });
}


//}}}1


// parser error cases


TEST_P(yaml_reader, error_bad_option_indent)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, " option: argument", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_bad_argument_indent)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, R"(
option: 1
2
)", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_start_tab)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, "\t", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_tab_indent)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, "option:\n\targument", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_double_colon)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, "option::", {})
    , po::parser_error
  );
}


TEST_P(yaml_reader, error_fold_no_colon)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, "option>", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_multiple_styles)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: ||", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: >|", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: |>", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: >>", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_invalid_chomp)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: |x", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_style_and_argument)
{
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: | argument", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: |+ argument", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: |- argument", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: > argument", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: >+ argument", {}),
    po::parser_error
  );
  EXPECT_THROW(
    parse_and_check(__LINE__, "option: >- argument", {}),
    po::parser_error
  );
}


TEST_P(yaml_reader, error_comment_inbetween_block_argument)
{
  EXPECT_THROW(parse_and_check(__LINE__, R"(
option:
  line_1
  # comment
  line_2
)", {}),
    po::parser_error
  );
}



TEST_P(yaml_reader, devel) //{{{1
{
  parse_and_check(__LINE__, R"(
a:
  
  X


  Z

b: >+ # comm
  X
  Z

c: |+

  X


  Z

x: |
  # komment
  proov

ends: here
)",
  {
    { "a", "X\n\nZ" },
    { "b", "X Z\n\n" },
    { "c", "\nX\n\n\nZ\n\n" },
    { "x", "# komment\nproov\n" },
    { "ends", "here" },
  });
}


// }}}1


} // namespace

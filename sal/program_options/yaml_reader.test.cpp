#include <sal/program_options/yaml_reader.hpp>
#include <sal/program_options/common.test.hpp>
#include <sstream>


namespace {

namespace po = sal::program_options;


struct yaml_reader
  : public sal_test::with_value<bool>
{
  using data_list = std::vector<std::pair<std::string, std::string>>;

  std::ostringstream oss;
  std::streambuf *old_buf;


  void enable_output ()
  {
    std::cout.rdbuf(old_buf);
  }


  void disable_output ()
  {
    std::cout.rdbuf(oss.rdbuf());
  }


  void SetUp ()
  {
    old_buf = std::cout.rdbuf();
    disable_output();
  }


  void TearDown ()
  {
    enable_output();
  }


  data_list parse (const std::string &content)
  {
    std::cout << "-----" << content << "-----\n\n*\n* begin\n*\n";

    std::istringstream iss{content};
    po::yaml_reader_t parser{iss};

    po::option_set_t options;
    std::string key, value;
    data_list result;
    while (parser(options, &key, &value))
    {
      result.emplace_back(key, value);
      key.clear();
      value.clear();
    }

    std::cout << "\n\n*\n* end\n*\n\n";
    return result;
  }
};


INSTANTIATE_TEST_CASE_P(program_options, yaml_reader, testing::Values(true));


// supported constructs: https://goo.gl/2nGFs8


TEST_P(yaml_reader, comment) //{{{1
{
  auto cf = R"(
#
# to be ignored
#
)";

  EXPECT_EQ(data_list{}, parse(cf));
}


TEST_P(yaml_reader, simple) //{{{1
{
  auto cf = R"(
simple: value
)";

  data_list expected =
  {
    { "simple", "value" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, nested) //{{{1
{
  auto cf = R"(
nested:
  nested_1: value
  nested_2: value

end: here
)";

  data_list expected =
  {
    { "nested.nested_1", "value" },
    { "nested.nested_2", "value" },
    { "end", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, deep_nested) //{{{1
{
  auto cf = R"(
deep_nested:
  deep_nested_1:
    deep_nested_1_1: value_1_1
    deep_nested_1_2: value_1_2
  deep_nested_2:
    deep_nested_2_1: value_2_1
    deep_nested_2_2: value_2_2
)";

  data_list expected =
  {
    { "deep_nested.deep_nested_1.deep_nested_1_1", "value_1_1" },
    { "deep_nested.deep_nested_1.deep_nested_1_2", "value_1_2" },
    { "deep_nested.deep_nested_2.deep_nested_2_1", "value_2_1" },
    { "deep_nested.deep_nested_2.deep_nested_2_2", "value_2_2" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, list) //{{{1
{
  auto cf = R"(
list:
  - value_1
  - value_2
)";

  data_list expected =
  {
    { "list", "value_1" },
    { "list", "value_2" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, nested_list) //{{{1
{
  auto cf = R"(
nested_list:
  nested_list_1:
    - value_1
    - value_2
  nested_list_2:
    - value_1
    - value_2
)";

  data_list expected =
  {
    { "nested_list.nested_list_1", "value_1" },
    { "nested_list.nested_list_1", "value_2" },
    { "nested_list.nested_list_2", "value_1" },
    { "nested_list.nested_list_2", "value_2" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, reference) //{{{1
{
  auto cf = R"(
reference: &reference_ref value
dereference: *reference_ref
)";

  data_list expected =
  {
    { "reference", "value" },
    { "dereference", "value" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, DISABLED_list_item_reference) //{{{1
{
  auto cf = R"(
list_item_reference:
  - &list_item_reference_ref value
  - *list_item_reference_ref
list_item_reference_deref: *list_item_reference_ref
)";

  data_list expected =
  {
    { "list_item_reference", "value" },
    { "list_item_reference", "value" },
    { "list_item_reference_deref", "value" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, double_quote) //{{{1
{
  auto cf = R"(
double_quote: "value_1\nvalue_2"
)";

  data_list expected =
  {
    { "double_quote", "value_1\nvalue_2" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, single_quote) //{{{1
{
  auto cf = R"(
single_quote: 'value_1\nvalue_2'
)";

  data_list expected =
  {
    { "single_quote", "value_1\\nvalue_2" },
  };

  EXPECT_EQ(expected, parse(cf));
}


//}}}1


// corner cases


TEST_P(yaml_reader, colon) //{{{1
{
  EXPECT_THROW(parse(":"), po::parser_error);
}


TEST_P(yaml_reader, empty) //{{{1
{
  EXPECT_EQ(data_list{}, parse(""));
}


TEST_P(yaml_reader, missing_final_newline) //{{{1
{
  auto cf = R"(
key: value
stop: here)";

  data_list expected =
  {
    { "key", "value" },
    { "stop", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, indented_root_node) //{{{1
{
  auto cf = R"(
  key: value
  stop: here
)";

  data_list expected =
  {
    { "key", "value" },
    { "stop", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, outdented_root_node) //{{{1
{
  auto cf = R"(
  key: value
stop: here
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, unexpected_tab) //{{{1
{
  EXPECT_THROW(parse("\t#\n"), po::parser_error);
}


TEST_P(yaml_reader, indented_node_with_tab) //{{{1
{
  EXPECT_THROW(parse("a:\n\tb: c"), po::parser_error);
}


TEST_P(yaml_reader, unsupported_block_style) //{{{1
{
  EXPECT_THROW(parse("key: >\n  value"), po::parser_error);
  EXPECT_THROW(parse("key: |\n  value"), po::parser_error);
}


TEST_P(yaml_reader, invalid_characters_before_colon) //{{{1
{
  EXPECT_THROW(parse("key X: value"), po::parser_error);
}


TEST_P(yaml_reader, missing_colon_space) ///{{{1
{
  EXPECT_THROW(parse("key value"), po::parser_error);
}


TEST_P(yaml_reader, missing_colon_newline) ///{{{1
{
  EXPECT_THROW(parse("key\nvalue"), po::parser_error);
}


TEST_P(yaml_reader, missing_value) //{{{1
{
  auto cf = R"(
key:
stop: here
)";

  data_list expected =
  {
    { "key", "" },
    { "stop", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, missing_value_with_comment) //{{{1
{
  auto cf = R"(
key: # comment
stop: here
)";

  data_list expected =
  {
    { "key", "" },
    { "stop", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}



TEST_P(yaml_reader, space_before_colon) //{{{1
{
  auto cf = R"(
key :value
)";

  data_list expected =
  {
    { "key", "value" }
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, tab_before_colon) //{{{1
{
  auto cf = "key\t:value\n";

  data_list expected =
  {
    { "key", "value" }
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, space_after_value) //{{{1
{
  data_list expected = { { "key", "value" } };
  EXPECT_EQ(expected, parse("key: value\t # comment"));
}


TEST_P(yaml_reader, invalid_reference_name) ///{{{1
{
  EXPECT_THROW(parse("key: &ref{r}ence value"), po::parser_error);
}


TEST_P(yaml_reader, reference_without_value) //{{{1
{
  auto cf = R"(
reference: &reference_ref
dereference: *reference_ref
)";

  data_list expected =
  {
    { "reference", "" },
    { "dereference", "" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, duplicate_reference) //{{{1
{
  auto cf = R"(
key_1: &a value_1
key_2: &a value_2
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, unknown_reference) //{{{1
{
  auto cf = R"(
key: *a
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, trailing_characters_after_reference) //{{{1
{
  auto cf = R"(
key_1: &a value
key_2: *a X
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, trailing_comment_after_reference) //{{{1
{
  auto cf = R"(
key_1: &a value
key_2: *a # comment
)";

  data_list expected =
  {
    { "key_1", "value" },
    { "key_2", "value" },
  };

  EXPECT_EQ(expected, parse(cf));
}


//}}}1


TEST_P(yaml_reader, double_quote_with_embedded_trailing_spaces) //{{{1
{
  auto cf = "\na: \"b\t \"\n";

  data_list expected =
  {
    { "a", "b\t " },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, single_quote_with_embedded_trailing_spaces) //{{{1
{
  auto cf = "\na: 'b\t '\n";

  data_list expected =
  {
    { "a", "b\t " },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, double_quote_with_embedded_comment) //{{{1
{
  auto cf = R"(
a: "b # c"
)";

  data_list expected =
  {
    { "a", "b # c" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, single_quote_with_embedded_comment) //{{{1
{
  auto cf = R"(
a: 'b # c'
)";

  data_list expected =
  {
    { "a", "b # c" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, double_quote_with_embedded_single_quote) //{{{1
{
  auto cf = R"(
a: "b ' c"
)";

  data_list expected =
  {
    { "a", "b ' c" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, single_quote_with_embedded_double_quote) //{{{1
{
  auto cf = R"(
a: 'b " c'
)";

  data_list expected =
  {
    { "a", "b \" c" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, double_quote_unexpected_newline) //{{{1
{
  auto cf = R"(
a: "b
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, single_quote_unexpected_newline) //{{{1
{
  auto cf = R"(
a: 'b
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, double_quote_unexpected_single_quote) //{{{1
{
  auto cf = R"(
a: "b'
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, single_quote_unexpected_double_quote) //{{{1
{
  auto cf = R"(
a: 'b"
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, double_quote_trailing_characters) //{{{1
{
  auto cf = R"(
a: "b c" d
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, single_quote_trailing_characters) //{{{1
{
  auto cf = R"(
a: 'b c' d
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, double_quote_trailing_comment) //{{{1
{
  auto cf = R"(
a: "b c" # comment
)";

  data_list expected =
  {
    { "a", "b c" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, single_quote_trailing_comment) //{{{1
{
  auto cf = R"(
a: 'b c' # comment
)";

  data_list expected =
  {
    { "a", "b c" },
  };

  EXPECT_EQ(expected, parse(cf));
}


//}}}1
TEST_P(yaml_reader, double_quote_escaped_characters) //{{{1
{
  auto cf = R"(
all: "\a\b\t\n\v\f\r\"\/\\"
)";

  data_list expected =
  {
    { "all", "\a\b\t\n\v\f\r\"/\\" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, single_quote_escaped_characters) //{{{1
{
  auto cf = R"(
all: '\a\b\t\n\v\f\r\"\/\\'
)";

  data_list expected =
  {
    { "all", "\\a\\b\\t\\n\\v\\f\\r\\\"\\/\\\\" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, double_quote_invalid_escaped_characters) //{{{1
{
  auto cf = R"(
all: "\x"
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, single_quote_invalid_escaped_characters) //{{{1
{
  auto cf = R"(
all: '\x'
)";

  data_list expected =
  {
    { "all", "\\x" },
  };

  EXPECT_EQ(expected, parse(cf));
}


//}}}1


TEST_P(yaml_reader, space_in_list_item) //{{{1
{
  auto cf = "list:\n - value 1\n - value\t2\n - value 3 \t\n";

  data_list expected =
  {
    { "list", "value 1" },
    { "list", "value\t2" },
    { "list", "value 3" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, empty_lines_between_list_key_and_items) //{{{1
{
  auto cf = R"(
list:

 - val1

 - val2

end: here
)";

  data_list expected =
  {
    { "list", "val1" },
    { "list", "val2" },
    { "end", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, tabbed_line_between_list_key_and_items) //{{{
{
  auto cf = "list:\n\t\n - val1\n - val2\n";
  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, tabbed_line_between_list_items) //{{{
{
  auto cf = "list:\n - val1\n\t\n - val2\n";
  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, multiple_spaces_between_list_marker_and_value) //{{{1
{
  auto cf = R"(
list:
 -   val1
 -  val2

end: here
)";

  data_list expected =
  {
    { "list", "val1" },
    { "list", "val2" },
    { "end", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, tab_between_list_marker_and_value) //{{{1
{
  auto cf = "list:\n -\tval1\n - val2\nend:here";

  data_list expected =
  {
    { "list", "val1" },
    { "list", "val2" },
    { "end", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, empty_list_value) //{{{1
{
  auto cf = R"(
list:
 -
 - val

end: here
)";

  data_list expected =
  {
    { "list", "" },
    { "list", "val" },
    { "end", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, empty_last_list_value) //{{{1
{
  auto cf = R"(
list:
 - val
 -

end: here
)";

  data_list expected =
  {
    { "list", "val" },
    { "list", "" },
    { "end", "here" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, empty_list_value_with_spaces) //{{{1
{
  auto cf = "list:\n -  \n - val\n";

  data_list expected =
  {
    { "list", "" },
    { "list", "val" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, unsupported_multiline_list_item) //{{{1
{
  auto cf = R"(
list:
 - val1
   val2
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, unsupported_list_item_block_style) //{{{1
{
  EXPECT_THROW(parse("l:\n - >\n  1\n"), po::parser_error);
  EXPECT_THROW(parse("l:\n - |\n  1\n"), po::parser_error);
}


TEST_P(yaml_reader, list_item_invalid_indent_more) //{{{1
{
  auto cf = R"(
list:
 - val1
  - val2
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, list_item_invalid_indent_less) //{{{1
{
  auto cf = R"(
list:
  - val1
 - val2
)";

  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, list_item_indented_with_tab) //{{{1
{
  auto cf = "list:\n\t- val1\n\t- val2";
  EXPECT_THROW(parse(cf), po::parser_error);
}


TEST_P(yaml_reader, list_with_missing_final_newline) //{{{1
{
  auto cf = "list:\n - val";

  data_list expected =
  {
    { "list", "val" },
  };

  EXPECT_EQ(expected, parse(cf));
}


TEST_P(yaml_reader, empty_last_list_value_with_missing_final_newline) //{{{1
{
  auto cf = R"(
list:
 - val
 -)";

  data_list expected =
  {
    { "list", "val" },
    { "list", "" },
  };

  EXPECT_EQ(expected, parse(cf));
}


//}}}1


} // namespace

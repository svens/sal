#include <sal/program_options/config_reader.hpp>
#include <sal/program_options/common.test.hpp>
#include <sstream>


namespace {

namespace po = sal::program_options;


struct config_reader
  : public sal_test::with_value<bool>
{
  using data_list = std::vector<std::pair<std::string, std::string>>;


  const data_list &empty ()
  {
    static const data_list empty_{};
    return empty_;
  }


  data_list parse (const std::string &content)
  {
    std::istringstream iss{content};
    po::config_reader_t parser{iss};

    po::option_set_t options;
    std::string key, value;
    data_list result;
    while (parser(options, &key, &value))
    {
      result.emplace_back(key, value);
      key.clear();
      value.clear();
    }

    return result;
  }
};


INSTANTIATE_TEST_CASE_P(program_options, config_reader, testing::Values(true));


TEST_P(config_reader, empty) //{{{1
{
  EXPECT_EQ(empty(), parse(""));
}


TEST_P(config_reader, empty_newline) //{{{1
{
  EXPECT_EQ(empty(), parse("\n\n"));
}


TEST_P(config_reader, empty_newline_with_blanks) //{{{1
{
  EXPECT_EQ(empty(), parse(" \t \n"));
}


TEST_P(config_reader, empty_root) //{{{1
{
  EXPECT_EQ(empty(), parse("{}"));
}


//}}}1


TEST_P(config_reader, comment_line) //{{{1
{
  EXPECT_EQ(empty(), parse("// comment"));
}


TEST_P(config_reader, comment_block) //{{{1
{
  auto input = R"(
/**
 * comment
 */
)";
  EXPECT_EQ(empty(), parse(input));
}


TEST_P(config_reader, comment_line_after_value) //{{{1
{
  static const data_list expected =
  {
    { "key", "1" },
  };

  EXPECT_EQ(expected, parse("key=1//comment"));
}


TEST_P(config_reader, comment_block_after_value) //{{{1
{
  static const data_list expected =
  {
    { "key", "1" },
  };

  EXPECT_EQ(expected, parse("key=1/*comment*/"));
}


TEST_P(config_reader, comment_before_and_after_every_token) //{{{1
{
  static const data_list expected =
  {
    { "a", "1" },
    { "x", "a/b" },
  };

  EXPECT_EQ(expected, parse("/**/a/**/=/**/1/**/,x=a/b//comment"));
}


TEST_P(config_reader, comment_line_inside_block_comment) //{{{1
{
  static const data_list expected =
  {
    { "key", "1" },
  };

  EXPECT_EQ(expected, parse("/* head // tail */key = 1"));
}


TEST_P(config_reader, comment_block_inside_block_comment) //{{{1
{
  static const data_list expected =
  {
    { "key", "1" },
  };

  EXPECT_EQ(expected, parse("/* head /* comment */ tail */key = 1"));
}


TEST_P(config_reader, comment_uncommented) //{{{1
{
  auto input = R"(
///*
key = 1
//*/
)";

  static const data_list expected =
  {
    { "key", "1" },
  };

  EXPECT_EQ(expected, parse(input));
}


//}}}1
TEST_P(config_reader, comment_unexpected_end) //{{{1
{
  EXPECT_THROW(parse("/*"), po::parser_error);
}


//}}}1


TEST_P(config_reader, assign) //{{{1
{
  static const data_list expected =
  {
    { "k", "v" },
  };
  EXPECT_EQ(expected, parse("k=v"));
}


TEST_P(config_reader, assign_invalid) //{{{1
{
  EXPECT_THROW(parse("k"), po::parser_error);
  EXPECT_THROW(parse("k v"), po::parser_error);
  EXPECT_THROW(parse("+ = a"), po::parser_error);
  EXPECT_THROW(parse("k+ = a"), po::parser_error);
}


TEST_P(config_reader, assign_to_empty) //{{{1
{
  EXPECT_THROW(parse("=v"), po::parser_error);
}


TEST_P(config_reader, assign_using_colon) //{{{1
{
  static const data_list expected =
  {
    { "k", "v" },
  };
  EXPECT_EQ(expected, parse("k:v"));
}


TEST_P(config_reader, assign_empty) //{{{1
{
  static const data_list expected =
  {
    { "k", "" },
  };
  EXPECT_EQ(expected, parse("k="));
}


TEST_P(config_reader, assign_with_newline) //{{{1
{
  auto input = R"(
a = 1
b = 2
)";

  static const data_list expected =
  {
    { "a", "1" },
    { "b", "2" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, assign_with_comma) //{{{1
{
  auto input = R"(
a = 1, b = 2,
c = 3,
)";

  static const data_list expected =
  {
    { "a", "1" },
    { "b", "2" },
    { "c", "3" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, assign_with_multiple_commas) //{{{1
{
  auto input = R"(
a = 1,, b = 2
)";

  EXPECT_THROW(parse(input), po::parser_error);
}


TEST_P(config_reader, assign_with_whitespace) //{{{1
{
  auto input = "a = 1 b = 2\tc = 3 d\n=\n4";

  static const data_list expected =
  {
    { "a", "1" },
    { "b", "2" },
    { "c", "3" },
    { "d", "4" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, assign_with_comment) //{{{1
{
  static const data_list expected =
  {
    { "a", "1" },
    { "b", "2" },
  };
  EXPECT_EQ(expected, parse("a=1/**/b=2"));
}


//}}}1


TEST_P(config_reader, object_root) //{{{1
{
  static const data_list expected =
  {
    { "k", "v" },
  };
  EXPECT_EQ(expected, parse("{ k = v }"));
}


TEST_P(config_reader, object_root_empty) //{{{1
{
  EXPECT_EQ(empty(), parse("{}"));
}


TEST_P(config_reader, object_root_empty_interleaved_with_comments) //{{{1
{
  EXPECT_EQ(empty(), parse("/**/{/**/}//comment"));
}


TEST_P(config_reader, object_root_nested_in_root) //{{{1
{
  EXPECT_THROW(parse("{{}}"), po::parser_error);
}


TEST_P(config_reader, object_root_invalid_close) //{{{1
{
  EXPECT_THROW(parse("}"), po::parser_error);
}


TEST_P(config_reader, object_root_not_closed) //{{{1
{
  EXPECT_THROW(parse("{"), po::parser_error);
}


TEST_P(config_reader, object_empty_root_invalid_close) //{{{1
{
  EXPECT_THROW(parse("{]"), po::parser_error);
}


TEST_P(config_reader, object_empty) //{{{1
{
  EXPECT_EQ(empty(), parse("x = {}"));
}


TEST_P(config_reader, object_empty_not_closed) //{{{1
{
  EXPECT_THROW(parse("x = {"), po::parser_error);
}


TEST_P(config_reader, object_empty_multiple_close) //{{{1
{
  EXPECT_THROW(parse("x = {}}"), po::parser_error);
}


TEST_P(config_reader, object_empty_invalid_close) //{{{1
{
  EXPECT_THROW(parse("x = {]"), po::parser_error);
}


TEST_P(config_reader, object_nested) //{{{1
{
  auto input = R"(
x = {
  y = {
    z = { a = 1 }
    b = 2
  }
  c = 3
}
)";

  static const data_list expected =
  {
    { "x.y.z.a", "1" },
    { "x.y.b", "2" },
    { "x.c", "3" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, object_nested_not_closed) //{{{1
{
  auto input = R"(
x = {
  y = {
    z = { a = 1 }
    b = 2
  }
  c = 3
)";

  EXPECT_THROW(parse(input), po::parser_error);
}


TEST_P(config_reader, object_nested_invalid_close) //{{{1
{
  EXPECT_THROW(parse("x={ a=1 ]"), po::parser_error);
}


TEST_P(config_reader, object_nested_with_commas) //{{{1
{
  auto input = R"(
x = {
  y = {
    z = { a = 1, }
    b = 2,
  },
  c = 3,
},
)";

  static const data_list expected =
  {
    { "x.y.z.a", "1" },
    { "x.y.b", "2" },
    { "x.c", "3" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, object_nested_in_root) //{{{1
{
  auto input = R"(
{
  x = {
    y = {
      z = { a = 1 }
      b = 2
    }
    c = 3
  }
}
)";

  static const data_list expected =
  {
    { "x.y.z.a", "1" },
    { "x.y.b", "2" },
    { "x.c", "3" },
  };

  EXPECT_EQ(expected, parse(input));
}


//}}}1


TEST_P(config_reader, array) //{{{1
{
  static const data_list expected =
  {
    { "a", "1" },
    { "a", "2" },
  };
  EXPECT_EQ(expected, parse("a=[1,2]"));
}


TEST_P(config_reader, array_single) //{{{1
{
  static const data_list expected =
  {
    { "a", "1" },
  };
  EXPECT_EQ(expected, parse("a=[1]"));
}


TEST_P(config_reader, array_empty) //{{{1
{
  EXPECT_EQ(empty(), parse("a=[]"));
}


TEST_P(config_reader, array_not_closed) //{{{1
{
  EXPECT_THROW(parse("a=["), po::parser_error);
}


TEST_P(config_reader, array_invalid_close) //{{{1
{
  EXPECT_THROW(parse("a=[1}"), po::parser_error);
}


TEST_P(config_reader, array_with_multiple_commas) //{{{1
{
  EXPECT_THROW(parse("a=[1,,2]"), po::parser_error);
}


TEST_P(config_reader, array_with_newline) //{{{1
{
  auto input = R"(
a = [
  1
  2
]
)";

  static const data_list expected =
  {
    { "a", "1" },
    { "a", "2" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, array_with_newline_and_comma) //{{{1
{
  auto input = R"(
a = [
  1,
  2,
]
)";

  static const data_list expected =
  {
    { "a", "1" },
    { "a", "2" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, array_in_root) //{{{1
{
  static const data_list expected =
  {
    { "a", "1" },
    { "a", "2" },
  };
  EXPECT_EQ(expected, parse("{a=[1,2]}"));
}


TEST_P(config_reader, array_in_root_invalid_close) //{{{1
{
  EXPECT_THROW(parse("{a=[1}"), po::parser_error);
}


TEST_P(config_reader, array_in_object) //{{{1
{
  static const data_list expected =
  {
    { "x.a", "1" },
    { "x.a", "2" },
  };
  EXPECT_EQ(expected, parse("x={a=[1,2]}"));
}


TEST_P(config_reader, array_of_mixed_values) //{{{1
{
  auto input = R"(
x=[ a, "b", 'c', """d""", '''e''',
"""
one
two
""",
'''
three
four
'''
]
)";

  static const data_list expected =
  {
    { "x", "a" },
    { "x", "b" },
    { "x", "c" },
    { "x", "d" },
    { "x", "e" },
    { "x", "one\ntwo\n" },
    { "x", "three\nfour\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, array_of_objects) //{{{1
{
  EXPECT_THROW(parse("x=[{a=1},{b=2}]"), po::parser_error);
}


TEST_P(config_reader, array_of_arrays) //{{{1
{
  EXPECT_THROW(parse("x=[[1,2],[3,4]]"), po::parser_error);
}


//}}}1


TEST_P(config_reader, key_bare) //{{{1
{
  auto input = R"(
K_e-y=1
123=2
)";

  static const data_list expected =
  {
    { "K_e-y", "1" },
    { "123", "2" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, key_bare_invalid) //{{{1
{
  EXPECT_THROW(parse("key.a=v"), po::parser_error);
}


TEST_P(config_reader, key_quoted) //{{{1
{
  auto input = R"(
'first key' = 1
"second key" = 2
'third "key"' = 3
"fourth 'key'" = 4
"fifth \"key\"" = 5
"sixth # key" = 6
)";

  static const data_list expected =
  {
    { "first key", "1" },
    { "second key", "2" },
    { "third \"key\"", "3" },
    { "fourth 'key'", "4" },
    { "fifth \"key\"", "5" },
    { "sixth # key", "6" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, key_quoted_empty) //{{{1
{
  auto input = R"(
"" = 1
'' = 2
)";

  static const data_list expected =
  {
    { "", "1" },
    { "", "2" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, key_quoted_with_escaped_characters) //{{{1
{
  auto input = R"(
"\ttabbed key" = 1
'\tweird key' = 2
)";

  static const data_list expected =
  {
    { "\ttabbed key", "1" },
    { "\\tweird key", "2" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, key_multiline_string_invalid) //{{{1
{
  EXPECT_THROW(parse("'''key''' = 1"), po::parser_error);
  EXPECT_THROW(parse(R"("""key""" = 1)"), po::parser_error);
}


//}}}1


TEST_P(config_reader, basic_string) //{{{1
{
  static const data_list expected =
  {
    { "key", "value" },
  };

  EXPECT_EQ(expected, parse(R"(key = "value")"));
}


TEST_P(config_reader, basic_string_with_multiple_quotes) //{{{1
{
  EXPECT_THROW(parse(R"(key = ""value"")"), po::parser_error);
}


TEST_P(config_reader, basic_string_with_trailing_characters) //{{{1
{
  EXPECT_THROW(parse(R"(key = "value" trail)"), po::parser_error);
}


TEST_P(config_reader, basic_string_with_trailing_comment) //{{{1
{
  static const data_list expected =
  {
    { "key", "value" },
  };
  EXPECT_EQ(expected, parse(R"(key = "value" // comment)"));
}


TEST_P(config_reader, basic_string_with_comment_in_value) //{{{1
{
  auto input = R"(
x = "a // b"
y = "c /**/ d"
)";

  static const data_list expected =
  {
    { "x", "a // b" },
    { "y", "c /**/ d" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, basic_string_with_quotes) //{{{1
{
  static const data_list expected =
  {
    { "key", "'value\"" },
  };
  EXPECT_EQ(expected, parse(R"(key = "'value\"")"));
}


TEST_P(config_reader, basic_string_unexpected_newline) //{{{1
{
  EXPECT_THROW(parse(R"(key = "value)"), po::parser_error);
}


TEST_P(config_reader, basic_string_unescape) //{{{1
{
  auto input = R"(
key = "\b\t\n\f\r\"\\"
)";

  static const data_list expected =
  {
    { "key", "\b\t\n\f\r\"\\" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, basic_string_invalid_escape) //{{{1
{
  EXPECT_THROW(parse(R"(key = "\value")"), po::parser_error);
}


TEST_P(config_reader, basic_string_unexpected_end_during_escape) //{{{1
{
  EXPECT_THROW(parse("key=\"value\\"), po::parser_error);
}


//}}}1


TEST_P(config_reader, basic_multiline_string) //{{{1
{
  static const data_list expected =
  {
    { "key", "value" },
  };
  EXPECT_EQ(expected, parse(R"(key = """value""")"));
}


TEST_P(config_reader, basic_multiline_string_empty) //{{{1
{
  static const data_list expected =
  {
    { "key", "" },
  };
  EXPECT_EQ(expected, parse(R"(key = """""")"));
}


TEST_P(config_reader, basic_multiline_string_with_quotes) //{{{1
{
  static const data_list expected =
  {
    { "key", "va\"l\"ue" },
  };
  EXPECT_EQ(expected, parse(R"(key = """va"l"ue""")"));
}


TEST_P(config_reader, basic_multiline_string_with_escaped_quotes) //{{{1
{
  auto input = R"(
key = """value\"\"\""""
)";

  static const data_list expected =
  {
    { "key", "value\"\"\"" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, basic_multiline_string_with_literal_string_end) //{{{1
{
  static const data_list expected =
  {
    { "key", "value'''" },
  };
  EXPECT_EQ(expected, parse(R"(key = """value'''""")"));
}


TEST_P(config_reader, basic_multiline_string_with_invalid_literal_string_end) //{{{1
{
  EXPECT_THROW(parse(R"(key = """value''')"), po::parser_error);
}


TEST_P(config_reader, basic_multiline_string_with_newline) //{{{1
{
  auto input = R"(
key = """one
 two
"""
)";

  static const data_list expected =
  {
    { "key", "one\n two\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, basic_multiline_string_with_immediate_newline) //{{{1
{
  auto input = R"(
key = """
one
two
"""
)";

  static const data_list expected =
  {
    { "key", "one\ntwo\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, basic_multiline_string_with_multiple_newline) //{{{1
{
  auto input = R"(
key = """

one
two
"""
)";

  static const data_list expected =
  {
    { "key", "\none\ntwo\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, basic_multiline_string_with_continuation) //{{{1
{
  auto input = R"(
key = """\
  one \

  two
"""
)";

  static const data_list expected =
  {
    { "key", "one two\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


//}}}1


TEST_P(config_reader, literal_string) //{{{1
{
  auto input = R"(
key = 'value'
)";

  static const data_list expected =
  {
    { "key", "value" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, literal_string_with_multiple_quotes) //{{{1
{
  auto input = R"(
key = ''trail''
)";

  EXPECT_THROW(parse(input), po::parser_error);
}


TEST_P(config_reader, literal_string_with_trailing_characters) //{{{1
{
  auto input = R"(
key = 'value' trail
)";

  EXPECT_THROW(parse(input), po::parser_error);
}


TEST_P(config_reader, literal_string_with_trailing_comment) //{{{1
{
  auto input = R"(
key = 'value' // comment
)";

  static const data_list expected =
  {
    { "key", "value" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, literal_string_with_comment_in_value) //{{{1
{
  auto input = R"(
x = 'a // b'
y = 'c /**/ d'
)";

  static const data_list expected =
  {
    { "x", "a // b" },
    { "y", "c /**/ d" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, literal_string_with_quotes) //{{{1
{
  auto input = R"(
key = '"value"'
)";

  static const data_list expected =
  {
    { "key", "\"value\"" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, literal_string_unexpected_newline) //{{{1
{
  auto input = R"(
key = 'value
)";

  EXPECT_THROW(parse(input), po::parser_error);
}


TEST_P(config_reader, literal_string_unescape) //{{{1
{
  auto input = R"(
key = '\b\t\n\f\r\"\\'
)";

  static const data_list expected =
  {
    { "key", "\\b\\t\\n\\f\\r\\\"\\\\" },
  };

  EXPECT_EQ(expected, parse(input));
}


//}}}1


TEST_P(config_reader, literal_multiline_string) //{{{1
{
  static const data_list expected =
  {
    { "key", "value" },
  };
  EXPECT_EQ(expected, parse(R"(key = '''value''')"));
}


TEST_P(config_reader, literal_multiline_string_empty) //{{{1
{
  static const data_list expected =
  {
    { "key", "" },
  };
  EXPECT_EQ(expected, parse(R"(key = '''''')"));
}


TEST_P(config_reader, literal_multiline_string_with_quotes) //{{{1
{
  static const data_list expected =
  {
    { "key", "va'l'ue" },
  };
  EXPECT_EQ(expected, parse(R"(key = '''va'l'ue''')"));
}


TEST_P(config_reader, literal_multiline_string_with_escaped_quotes) //{{{1
{
  static const data_list expected =
  {
    { "key", "value\\'\\'\\" },
  };
  EXPECT_EQ(expected, parse(R"(key = '''value\'\'\''')"));
}


TEST_P(config_reader, literal_multiline_string_with_basic_string_end) //{{{1
{
  static const data_list expected =
  {
    { "key", "value\"\"\"" },
  };
  EXPECT_EQ(expected, parse(R"(key = '''value"""''')"));
}


TEST_P(config_reader, literal_multiline_string_with_invalid_basic_string_end) //{{{1
{
  EXPECT_THROW(parse(R"(key = '''value""")"), po::parser_error);
}


TEST_P(config_reader, literal_multiline_string_with_newline) //{{{1
{
  auto input = R"(
key = '''one
 two
'''
)";

  static const data_list expected =
  {
    { "key", "one\n two\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, literal_multiline_string_with_immediate_newline) //{{{1
{
  auto input = R"(
key = '''
 one
two
'''
)";

  static const data_list expected =
  {
    { "key", " one\ntwo\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, literal_multiline_string_with_multiple_newline) //{{{1
{
  auto input = R"(
key = '''

one
two
'''
)";

  static const data_list expected =
  {
    { "key", "\none\ntwo\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


TEST_P(config_reader, literal_multiline_string_with_continuation) //{{{1
{
  auto input = R"(
key = '''\
one \
two
'''
)";

  static const data_list expected =
  {
    { "key", "\\\none \\\ntwo\n" },
  };

  EXPECT_EQ(expected, parse(input));
}


//}}}1


} // namespace

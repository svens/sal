#include <sal/program_options/command_line.hpp>
#include <sal/program_options/common.test.hpp>


namespace {

namespace po = sal::program_options;


struct command_line
  : public sal_test::with_value<bool>
{
  po::option_set_t options;


  void SetUp ()
  {
    options
      .add({"n", "no-argument"})
      .add({"o", "optional"}, po::optional_argument("unit"))
      .add({"r", "requires"}, po::requires_argument("unit"))
    ;
  }


  template <typename... Args>
  std::vector<std::pair<std::string, std::string>> parse (Args &&...args)
  {
    const char *argv[] = { "app", std::forward<Args>(args)... };
    po::command_line_t parser(sizeof(argv)/sizeof(argv[0]), argv);

    std::string option, argument;
    std::vector<std::pair<std::string, std::string>> result;
    while (parser(options, &option, &argument))
    {
      result.emplace_back(option, argument);
    }

    return result;
  }


  void check (const std::vector<std::pair<std::string, std::string>> &result,
    std::initializer_list<std::pair<std::string, std::string>> expected)
  {
    ASSERT_EQ(expected.size(), result.size());

    auto index = 0U;
    for (auto &value: expected)
    {
      EXPECT_EQ(value, result[index]) << "Where index=" << index;
      ++index;
    }
  }
};


INSTANTIATE_TEST_CASE_P(program_options, command_line, testing::Values(true),);


TEST_P(command_line, no_options) //{{{1
{
  check(parse(), {});
}


TEST_P(command_line, single_empty_positional) //{{{1
{
  check(parse(""),
  {
    { "", "" },
  });
}


TEST_P(command_line, multiple_empty_positionals) //{{{1
{
  check(parse("", ""),
  {
    { "", "" },
    { "", "" },
  });
}


TEST_P(command_line, single_positional) //{{{1
{
  check(parse("first"),
  {
    { "", "first" },
  });
}


TEST_P(command_line, multiple_positionals) //{{{1
{
  check(parse("first", "second"),
  {
    { "", "first" },
    { "", "second" },
  });
}


TEST_P(command_line, single_dash_only) //{{{1
{
  check(parse("-"), {});
}


TEST_P(command_line, double_dash_only) //{{{1
{
  check(parse("--"), {});
}


TEST_P(command_line, single_dash_middle) //{{{1
{
  check(parse("-n", "-", "-o"),
  {
    { "n", "" },
    { "o", "" },
  });
}


TEST_P(command_line, double_dash_middle) //{{{1
{
  check(parse("-n", "--", "-o"),
  {
    { "n", "" },
    { "", "-o" },
  });
}


TEST_P(command_line, single_dash_end) //{{{1
{
  check(parse("-n", "-"),
  {
    { "n", "" },
  });
}


TEST_P(command_line, double_dash_end) //{{{1
{
  check(parse("-n", "--"),
  {
    { "n", "" },
  });
}


TEST_P(command_line, no_argument_short) //{{{1
{
  check(parse("-n"),
  {
    { "n", "" },
  });
}


TEST_P(command_line, optional_short) //{{{1
{
  check(parse("-o"),
  {
    { "o", "" },
  });
}


TEST_P(command_line, requires_short) //{{{1
{
  EXPECT_THROW(parse("-r"), po::option_requires_argument_error);
}


TEST_P(command_line, no_argument_short_with_combined_argument) //{{{1
{
  EXPECT_THROW(parse("-na"), po::unknown_option_error);
}


TEST_P(command_line, optional_short_with_combined_argument) //{{{1
{
  check(parse("-oa"),
  {
    { "o", "a" },
  });
}


TEST_P(command_line, requires_short_with_combined_argument) //{{{1
{
  check(parse("-ra"),
  {
    { "r", "a" },
  });
}


TEST_P(command_line, no_argument_short_with_argument) //{{{1
{
  check(parse("-n", "a"),
  {
    { "n", "" },
    { "", "a" },
  });
}


TEST_P(command_line, optional_short_with_argument) //{{{1
{
  check(parse("-o", "a"),
  {
    { "o", "a" },
  });
}


TEST_P(command_line, requires_short_with_argument) //{{{1
{
  check(parse("-r", "a"),
  {
    { "r", "a" },
  });
}


TEST_P(command_line, no_argument_long) //{{{1
{
  check(parse("--no-argument"),
  {
    { "no-argument", "" },
  });
}


TEST_P(command_line, optional_long) //{{{1
{
  check(parse("--optional"),
  {
    { "optional", "" },
  });
}


TEST_P(command_line, requires_long) //{{{1
{
  EXPECT_THROW(parse("--requires"), po::option_requires_argument_error);
}


TEST_P(command_line, no_argument_long_assign) //{{{1
{
  EXPECT_THROW(parse("--no-argument="), po::option_rejects_argument_error);
}


TEST_P(command_line, optional_long_assign) //{{{1
{
  check(parse("--optional="),
  {
    { "optional", "" },
  });
}


TEST_P(command_line, requires_long_assign) //{{{1
{
  EXPECT_THROW(parse("--requires="), po::option_requires_argument_error);
}


TEST_P(command_line, no_argument_long_with_argument) //{{{1
{
  check(parse("--no-argument", "a"),
  {
    { "no-argument", "" },
    { "", "a" },
  });
}


TEST_P(command_line, no_argument_long_with_assigned_argument) //{{{1
{
  EXPECT_THROW(parse("--no-argument=a"), po::option_rejects_argument_error);
}


TEST_P(command_line, no_argument_long_with_combined_argument) //{{{1
{
  EXPECT_THROW(parse("--no-argumenta"), po::unknown_option_error);
}


TEST_P(command_line, optional_long_with_argument) //{{{1
{
  check(parse("--optional", "a"),
  {
    { "optional", "a" },
  });
}


TEST_P(command_line, optional_long_with_assigned_argument) //{{{1
{
  check(parse("--optional=a"),
  {
    { "optional", "a" },
  });
}


TEST_P(command_line, optional_long_with_combined_argument) //{{{1
{
  EXPECT_THROW(parse("--optionala"), po::unknown_option_error);
}


TEST_P(command_line, requires_long_with_argument) //{{{1
{
  check(parse("--requires", "a"),
  {
    { "requires", "a" },
  });
}


TEST_P(command_line, requires_long_with_assigned_argument) //{{{1
{
  check(parse("--requires=a"),
  {
    { "requires", "a" },
  });
}


TEST_P(command_line, requires_long_with_combined_argument) //{{{1
{
  EXPECT_THROW(parse("--requiresa"), po::unknown_option_error);
}


TEST_P(command_line, no_argument_short_and_no_argument_short) //{{{1
{
  check(parse("-n", "-n"),
  {
    { "n", "" },
    { "n", "" },
  });
}


TEST_P(command_line, no_argument_short_and_no_argument_short_combined) //{{{1
{
  check(parse("-nn"),
  {
    { "n", "" },
    { "n", "" },
  });
}


TEST_P(command_line, no_argument_short_and_optional_short) //{{{1
{
  check(parse("-n", "-o"),
  {
    { "n", "" },
    { "o", "" },
  });
}


TEST_P(command_line, no_argument_short_and_optional_short_combined) //{{{1
{
  check(parse("-no"),
  {
    { "n", "" },
    { "o", "" },
  });
}


TEST_P(command_line, no_argument_short_and_requires_short) //{{{1
{
  EXPECT_THROW(parse("-n", "-r"), po::option_requires_argument_error);
}


TEST_P(command_line, no_argument_short_and_requires_short_combined) //{{{1
{
  EXPECT_THROW(parse("-nr"), po::option_requires_argument_error);
}


TEST_P(command_line, optional_short_and_no_argument_short) //{{{1
{
  check(parse("-o", "-n"),
  {
    { "o", "" },
    { "n", "" },
  });
}


TEST_P(command_line, optional_short_and_no_argument_short_combined) //{{{1
{
  check(parse("-on"),
  {
    { "o", "n" },
  });
}


TEST_P(command_line, optional_short_and_optional_short) //{{{1
{
  check(parse("-o", "-o"),
  {
    { "o", "" },
    { "o", "" },
  });
}


TEST_P(command_line, optional_short_and_optional_short_combined) //{{{1
{
  check(parse("-oo"),
  {
    { "o", "o" },
  });
}


TEST_P(command_line, optional_short_and_requires_short) //{{{1
{
  EXPECT_THROW(parse("-o", "-r"), po::option_requires_argument_error);
}


TEST_P(command_line, optional_short_and_requires_short_combined) //{{{1
{
  check(parse("-or"),
  {
    { "o", "r" },
  });
}


TEST_P(command_line, requires_short_and_no_argument_short) //{{{1
{
  EXPECT_THROW(parse("-r", "-n"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_short_and_no_argument_short_combined) //{{{1
{
  check(parse("-rn"),
  {
    { "r", "n" },
  });
}


TEST_P(command_line, requires_short_and_optional_short) //{{{1
{
  EXPECT_THROW(parse("-r", "-o"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_short_and_optional_short_combined) //{{{1
{
  check(parse("-ro"),
  {
    { "r", "o" },
  });
}


TEST_P(command_line, requires_short_and_requires_short) //{{{1
{
  EXPECT_THROW(parse("-r", "-r"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_short_and_requires_short_combined) //{{{1
{
  check(parse("-rr"),
  {
    { "r", "r" },
  });
}


TEST_P(command_line, no_argument_long_and_no_argument_long) //{{{1
{
  check(parse("--no-argument", "--no-argument"),
  {
    { "no-argument", "" },
    { "no-argument", "" },
  });
}


TEST_P(command_line, no_argument_long_and_no_argument_long_assigned) //{{{1
{
  EXPECT_THROW(parse("--no-argument=no-argument"), po::option_rejects_argument_error);
}


TEST_P(command_line, no_argument_long_and_no_argument_long_spaced) //{{{1
{
  check(parse("--no-argument", "no-argument"),
  {
    { "no-argument", "" },
    { "", "no-argument" },
  });
}


TEST_P(command_line, no_argument_long_and_optional_long) //{{{1
{
  check(parse("--no-argument", "--optional"),
  {
    { "no-argument", "" },
    { "optional", "" },
  });
}


TEST_P(command_line, no_argument_long_and_optional_long_assigned) //{{{1
{
  EXPECT_THROW(parse("--no-argument=optional"), po::option_rejects_argument_error);
}


TEST_P(command_line, no_argument_long_and_optional_long_spaced) //{{{1
{
  check(parse("--no-argument", "optional"),
  {
    { "no-argument", "" },
    { "", "optional" },
  });
}


TEST_P(command_line, no_argument_long_and_requires_long) //{{{1
{
  EXPECT_THROW(parse("--no-argument", "--requires"), po::option_requires_argument_error);
}


TEST_P(command_line, no_argument_long_and_requires_long_assigned) //{{{1
{
  EXPECT_THROW(parse("--no-argument=requires"), po::option_rejects_argument_error);
}


TEST_P(command_line, no_argument_long_and_requires_long_spaced) //{{{1
{
  check(parse("--no-argument", "requires"),
  {
    { "no-argument", "" },
    { "", "requires" },
  });
}


TEST_P(command_line, optional_long_and_no_argument_long) //{{{1
{
  check(parse("--optional", "--no-argument"),
  {
    { "optional", "" },
    { "no-argument", "" },
  });
}


TEST_P(command_line, optional_long_and_no_argument_long_assigned) //{{{1
{
  check(parse("--optional=no-argument"),
  {
    { "optional", "no-argument" },
  });
}


TEST_P(command_line, optional_long_and_no_argument_long_spaced) //{{{1
{
  check(parse("--optional", "no-argument"),
  {
    { "optional", "no-argument" },
  });
}


TEST_P(command_line, optional_long_and_optional_long) //{{{1
{
  check(parse("--optional", "--optional"),
  {
    { "optional", "" },
    { "optional", "" },
  });
}


TEST_P(command_line, optional_long_and_optional_long_assigned) //{{{1
{
  check(parse("--optional=optional"),
  {
    { "optional", "optional" },
  });
}


TEST_P(command_line, optional_long_and_optional_long_spaced) //{{{1
{
  check(parse("--optional", "optional"),
  {
    { "optional", "optional" },
  });
}


TEST_P(command_line, optional_long_and_requires_long) //{{{1
{
  EXPECT_THROW(parse("--optional", "--requires"), po::option_requires_argument_error);
}


TEST_P(command_line, optional_long_and_requires_long_assigned) //{{{1
{
  check(parse("--optional=requires"),
  {
    { "optional", "requires" },
  });
}


TEST_P(command_line, optional_long_and_requires_long_spaced) //{{{1
{
  check(parse("--optional", "requires"),
  {
    { "optional", "requires" },
  });
}


TEST_P(command_line, requires_long_and_no_argument_long) //{{{1
{
  EXPECT_THROW(parse("--requires", "--no-argument"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_long_and_no_argument_long_assigned) //{{{1
{
  check(parse("--requires=no-argument"),
  {
    { "requires", "no-argument" },
  });
}


TEST_P(command_line, requires_long_and_no_argument_long_spaced) //{{{1
{
  check(parse("--requires", "no-argument"),
  {
    { "requires", "no-argument" },
  });
}


TEST_P(command_line, requires_long_and_optional_long) //{{{1
{
  EXPECT_THROW(parse("--requires", "--optional"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_long_and_optional_long_assigned) //{{{1
{
  check(parse("--requires=optional"),
  {
    { "requires", "optional" },
  });
}


TEST_P(command_line, requires_long_and_optional_long_spaced) //{{{1
{
  check(parse("--requires", "optional"),
  {
    { "requires", "optional" },
  });
}


TEST_P(command_line, requires_long_and_requires_long) //{{{1
{
  EXPECT_THROW(parse("--requires", "--requires"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_long_and_requires_long_assigned) //{{{1
{
  check(parse("--requires=requires"),
  {
    { "requires", "requires" },
  });
}


TEST_P(command_line, requires_long_and_requires_long_spaced) //{{{1
{
  check(parse("--requires", "requires"),
  {
    { "requires", "requires" },
  });
}


TEST_P(command_line, no_argument_short_and_no_argument_long) //{{{1
{
  check(parse("-n", "--no-argument"),
  {
    { "n", "" },
    { "no-argument", "" },
  });
}


TEST_P(command_line, no_argument_short_and_no_argument_long_combined) //{{{1
{
  EXPECT_THROW(parse("-nno-argument"), po::unknown_option_error);
}


TEST_P(command_line, no_argument_short_and_optional_long) //{{{1
{
  check(parse("-n", "--optional"),
  {
    { "n", "" },
    { "optional", "" },
  });
}


TEST_P(command_line, no_argument_short_and_optional_long_combined) //{{{1
{
  check(parse("-noptional"),
  {
    { "n", "" },
    { "o", "ptional" },
  });
}


TEST_P(command_line, no_argument_short_and_requires_long) //{{{1
{
  EXPECT_THROW(parse("-n", "--requires"), po::option_requires_argument_error);
}


TEST_P(command_line, no_argument_short_and_requires_long_combined) //{{{1
{
  check(parse("-nrequires"),
  {
    { "n", "" },
    { "r", "equires" },
  });
}


TEST_P(command_line, optional_short_and_no_argument_long) //{{{1
{
  check(parse("-o", "--no-argument"),
  {
    { "o", "" },
    { "no-argument", "" },
  });
}


TEST_P(command_line, optional_short_and_no_argument_long_combined) //{{{1
{
  check(parse("-ono-argument"),
  {
    { "o", "no-argument" },
  });
}


TEST_P(command_line, optional_short_and_optional_long) //{{{1
{
  check(parse("-o", "--optional"),
  {
    { "o", "" },
    { "optional", "" },
  });
}


TEST_P(command_line, optional_short_and_optional_long_combined) //{{{1
{
  check(parse("-ooptional"),
  {
    { "o", "optional" },
  });
}


TEST_P(command_line, optional_short_and_requires_long) //{{{1
{
  EXPECT_THROW(parse("-o", "--requires"), po::option_requires_argument_error);
}


TEST_P(command_line, optional_short_and_requires_long_combined) //{{{1
{
  check(parse("-orequires"),
  {
    { "o", "requires" },
  });
}


TEST_P(command_line, requires_short_and_no_argument_long) //{{{1
{
  EXPECT_THROW(parse("-r", "--no-argument"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_short_and_no_argument_long_combined) //{{{1
{
  check(parse("-rno-argument"),
  {
    { "r", "no-argument" },
  });
}


TEST_P(command_line, requires_short_and_optional_long) //{{{1
{
  check(parse("-r", "optional"),
  {
    { "r", "optional" },
  });
}


TEST_P(command_line, requires_short_and_optional_long_combined) //{{{1
{
  check(parse("-roptional"),
  {
    { "r", "optional" },
  });
}


TEST_P(command_line, requires_short_and_requires_long) //{{{1
{
  EXPECT_THROW(parse("-r", "--requires"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_short_and_requires_long_combined) //{{{1
{
  check(parse("-rrequires"),
  {
    { "r", "requires" },
  });
}


TEST_P(command_line, no_argument_long_and_no_argument_short) //{{{1
{
  check(parse("--no-argument", "-n"),
  {
    { "no-argument", "" },
    { "n", "" },
  });
}


TEST_P(command_line, no_argument_long_and_no_argument_short_spaced) //{{{1
{
  check(parse("--no-argument", "n"),
  {
    { "no-argument", "" },
    { "", "n" },
  });
}


TEST_P(command_line, no_argument_long_and_no_argument_short_assigned) //{{{1
{
  EXPECT_THROW(parse("--no-argument=n"), po::option_rejects_argument_error);
}


TEST_P(command_line, no_argument_long_and_optional_short) //{{{1
{
  check(parse("--no-argument", "-o"),
  {
    { "no-argument", "" },
    { "o", "" },
  });
}


TEST_P(command_line, no_argument_long_and_optional_short_spaced) //{{{1
{
  check(parse("--no-argument", "o"),
  {
    { "no-argument", "" },
    { "", "o" },
  });
}


TEST_P(command_line, no_argument_long_and_optional_short_assigned) //{{{1
{
  EXPECT_THROW(parse("--no-argument=o"), po::option_rejects_argument_error);
}


TEST_P(command_line, no_argument_long_and_requires_short) //{{{1
{
  EXPECT_THROW(parse("--no-argument", "-r"), po::option_requires_argument_error);
}


TEST_P(command_line, no_argument_long_and_requires_short_spaced) //{{{1
{
  check(parse("--no-argument", "r"),
  {
    { "no-argument", "" },
    { "", "r" },
  });
}


TEST_P(command_line, no_argument_long_and_requires_short_assigned) //{{{1
{
  EXPECT_THROW(parse("--no-argument=r"), po::option_rejects_argument_error);
}


TEST_P(command_line, optional_long_and_no_argument_short) //{{{1
{
  check(parse("--optional", "-n"),
  {
    { "optional", "" },
    { "n", "" },
  });
}


TEST_P(command_line, optional_long_and_no_argument_short_spaced) //{{{1
{
  check(parse("--optional", "n"),
  {
    { "optional", "n" },
  });
}


TEST_P(command_line, optional_long_and_no_argument_short_assigned) //{{{1
{
  check(parse("--optional=n"),
  {
    { "optional", "n" },
  });
}


TEST_P(command_line, optional_long_and_optional_short) //{{{1
{
  check(parse("--optional", "-o"),
  {
    { "optional", "" },
    { "o", "" },
  });
}


TEST_P(command_line, optional_long_and_optional_short_spaced) //{{{1
{
  check(parse("--optional", "o"),
  {
    { "optional", "o" },
  });
}


TEST_P(command_line, optional_long_and_optional_short_assigned) //{{{1
{
  check(parse("--optional=o"),
  {
    { "optional", "o" },
  });
}


TEST_P(command_line, optional_long_and_requires_short) //{{{1
{
  EXPECT_THROW(parse("--optional", "-r"), po::option_requires_argument_error);
}


TEST_P(command_line, optional_long_and_requires_short_spaced) //{{{1
{
  check(parse("--optional", "r"),
  {
    { "optional", "r" },
  });
}


TEST_P(command_line, optional_long_and_requires_short_assigned) //{{{1
{
  check(parse("--optional=r"),
  {
    { "optional", "r" },
  });
}


TEST_P(command_line, requires_long_and_no_argument_short) //{{{1
{
  EXPECT_THROW(parse("--requires", "-n"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_long_and_no_argument_short_spaced) //{{{1
{
  check(parse("--requires", "n"),
  {
    { "requires", "n" },
  });
}


TEST_P(command_line, requires_long_and_no_argument_short_assigned) //{{{1
{
  check(parse("--requires=n"),
  {
    { "requires", "n" },
  });
}


TEST_P(command_line, requires_long_and_optional_short) //{{{1
{
  EXPECT_THROW(parse("--requires", "-o"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_long_and_optional_short_spaced) //{{{1
{
  check(parse("--requires", "o"),
  {
    { "requires", "o" },
  });
}


TEST_P(command_line, requires_long_and_optional_short_assigned) //{{{1
{
  check(parse("--requires=o"),
  {
    { "requires", "o" },
  });
}


TEST_P(command_line, requires_long_and_requires_short) //{{{1
{
  EXPECT_THROW(parse("--requires", "-r"), po::option_requires_argument_error);
}


TEST_P(command_line, requires_long_and_requires_short_spaced) //{{{1
{
  check(parse("--requires", "r"),
  {
    { "requires", "r" },
  });
}


TEST_P(command_line, requires_long_and_requires_short_assigned) //{{{1
{
  check(parse("--requires=r"),
  {
    { "requires", "r" },
  });
}


} // namespace

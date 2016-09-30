#include <sal/program_options/option_set.hpp>
#include <sal/program_options/common.test.hpp>


namespace {

namespace po = sal::program_options;


struct option_set
  : public sal_test::with_value<bool>
{
  po::option_set_t options;
};


INSTANTIATE_TEST_CASE_P(program_options, option_set, testing::Values(true));


TEST_P(option_set, add_no_names)
{
  EXPECT_THROW(
    options.add({}),
    std::logic_error
  );
}


TEST_P(option_set, add_empty_name)
{
  EXPECT_THROW(
    options.add({ "" }),
    std::logic_error
  );
}


TEST_P(option_set, add_invalid_name)
{
  EXPECT_THROW(
    options.add({ "$name" }),
    std::logic_error
  );
}


TEST_P(option_set, add_duplicate_name_same_option)
{
  EXPECT_THROW(
    options.add({ "name", "name" }),
    std::logic_error
  );
}


TEST_P(option_set, add_duplicate_name_different_option)
{
  EXPECT_THROW(
    options
      .add({ "name" })
      .add({ "name" }),
    std::logic_error
  );
}


void fill (po::option_set_t &options)
{
  options
    .add({ "1", "first" },
      po::requires_argument("FIRST"),
      po::help("TSRIF")
    )
    .add({ "2", "second" },
      po::optional_argument("SECOND"),
      po::help("DNOCES")
    )
    .add({ "3", "third" },
      po::help("Third\ntHird\n\nthIrd\nthiRd thirD")
    )
    .add({ "4", "fourth" },
      po::requires_argument("FOURTH"),
      po::default_value("4th"),
      po::help("HTRUOF")
    )
    .add({ "5", "fifth" },
      po::optional_argument("FIFTH"),
      po::default_value("5th"),
      po::help("HTFIF")
    )
  ;
}


void check (const std::string &help)
{
  EXPECT_NE(help.find("1"), help.npos);
  EXPECT_NE(help.find("first"), help.npos);
  EXPECT_NE(help.find("FIRST"), help.npos);
  EXPECT_NE(help.find("TSRIF"), help.npos);

  EXPECT_NE(help.find("2"), help.npos);
  EXPECT_NE(help.find("second"), help.npos);
  EXPECT_NE(help.find("[SECOND]"), help.npos);
  EXPECT_NE(help.find("DNOCES"), help.npos);

  EXPECT_NE(help.find("3"), help.npos);
  EXPECT_NE(help.find("third"), help.npos);
  EXPECT_NE(help.find("Third"), help.npos);
  EXPECT_NE(help.find("tHird"), help.npos);
  EXPECT_NE(help.find("thIrd"), help.npos);
  EXPECT_NE(help.find("thiRd"), help.npos);
  EXPECT_NE(help.find("thirD"), help.npos);

  EXPECT_NE(help.find("4"), help.npos);
  EXPECT_NE(help.find("fourth"), help.npos);
  EXPECT_NE(help.find("FOURTH"), help.npos);
  EXPECT_NE(help.find("HTRUOF"), help.npos);

  EXPECT_NE(help.find("5"), help.npos);
  EXPECT_NE(help.find("fifth"), help.npos);
  EXPECT_NE(help.find("FIFTH"), help.npos);
  EXPECT_NE(help.find("HTFIF"), help.npos);
}


TEST_P(option_set, help)
{
  fill(options);
  std::ostringstream oss;
  oss << options;
  check(oss.str());
}


TEST_P(option_set, help_wide)
{
  fill(options);
  std::ostringstream oss;
  oss << std::setw(100) << options;
  check(oss.str());
}


TEST_P(option_set, help_narrow)
{
  fill(options);
  std::ostringstream oss;
  oss << std::setw(10) << options;
  check(oss.str());
}


TEST_P(option_set, empty_load_from_empty)
{
  sal_test::hardcoded_config_t hardcoded = {};
  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has(case_name));
  EXPECT_TRUE(conf[case_name].empty());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, empty_load_from_hardcoded_empty_option_and_argument)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { "", "" },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has(case_name));
  EXPECT_TRUE(conf[case_name].empty());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, empty_load_from_hardcoded_multiple_empty_option_and_argument)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { "", "" },
    { "", "" },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has(case_name));
  EXPECT_TRUE(conf[case_name].empty());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, empty_load_from_hardcoded_option_and_argument)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { "option", case_name },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_TRUE(conf.has("option"));
  ASSERT_EQ(1U, conf["option"].size());
  EXPECT_EQ(case_name, conf["option"].back());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, empty_load_from_hardcoded_multiple_option_and_argument)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { "option", case_name + "_0" },
    { "option", case_name + "_1" },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_TRUE(conf.has("option"));
  ASSERT_EQ(2U, conf["option"].size());
  EXPECT_EQ(case_name + "_0", conf["option"][0]);
  EXPECT_EQ(case_name + "_1", conf["option"][1]);
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, empty_load_from_hardcoded_option)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { case_name, "" },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_TRUE(conf.has(case_name));
  ASSERT_EQ(1U, conf[case_name].size());
  EXPECT_EQ("", conf[case_name].back());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, empty_load_from_hardcoded_multiple_option)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { case_name, "" },
    { case_name, "" },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_TRUE(conf.has(case_name));
  ASSERT_EQ(2U, conf[case_name].size());
  EXPECT_EQ("", conf[case_name][0]);
  EXPECT_EQ("", conf[case_name][1]);
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, empty_load_from_hardcoded_argument)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { "", case_name },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has(case_name));
  EXPECT_TRUE(conf["option"].empty());
  ASSERT_EQ(1U, conf.positional_arguments().size());
  EXPECT_EQ(case_name, conf.positional_arguments().back());
}


TEST_P(option_set, empty_load_from_hardcoded_multiple_argument)
{
  sal_test::hardcoded_config_t hardcoded =
  {
    { "", case_name + "_0" },
    { "", case_name + "_1" },
  };
  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has(case_name));
  EXPECT_TRUE(conf["option"].empty());
  ASSERT_EQ(2U, conf.positional_arguments().size());
  EXPECT_EQ(case_name + "_0", conf.positional_arguments()[0]);
  EXPECT_EQ(case_name + "_1", conf.positional_arguments()[1]);
}


TEST_P(option_set, load_from_empty)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded = {};
  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has("1"));
  EXPECT_TRUE(conf["1"].empty());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_empty_option_and_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "", "" },
  };

  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has(""));
  EXPECT_TRUE(conf[""].empty());
  EXPECT_FALSE(conf.has("1"));
  EXPECT_TRUE(conf["1"].empty());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_multiple_empty_option_and_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "", "" },
    { "", "" },
  };

  auto conf = options.load_from(hardcoded);
  EXPECT_FALSE(conf.has(""));
  EXPECT_TRUE(conf[""].empty());
  EXPECT_FALSE(conf.has("1"));
  EXPECT_TRUE(conf["1"].empty());
  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_option_and_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "1", case_name },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("1"));
  ASSERT_EQ(1U, conf["1"].size());
  EXPECT_EQ(case_name, conf["1"][0]);

  EXPECT_TRUE(conf.has("first"));
  ASSERT_EQ(1U, conf["first"].size());
  EXPECT_EQ(case_name, conf["first"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_multiple_option_and_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "1", case_name + "_1_0" },
    { "1", case_name + "_1_1" },
    { "2", case_name + "_2" },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("1"));
  ASSERT_EQ(2U, conf["1"].size());
  EXPECT_EQ(case_name + "_1_0", conf["1"][0]);
  EXPECT_EQ(case_name + "_1_1", conf["1"][1]);

  EXPECT_TRUE(conf.has("first"));
  ASSERT_EQ(2U, conf["first"].size());
  EXPECT_EQ(case_name + "_1_0", conf["first"][0]);
  EXPECT_EQ(case_name + "_1_1", conf["first"][1]);

  EXPECT_TRUE(conf.has("2"));
  ASSERT_EQ(1U, conf["2"].size());
  EXPECT_EQ(case_name + "_2", conf["2"][0]);

  EXPECT_TRUE(conf.has("second"));
  ASSERT_EQ(1U, conf["second"].size());
  EXPECT_EQ(case_name + "_2", conf["second"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_required_argument_option_no_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "1", "" },
  };

  EXPECT_THROW(options.load_from(hardcoded), std::runtime_error);
}


TEST_P(option_set, load_from_hardcoded_required_argument_option_with_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "1", case_name },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("1"));
  ASSERT_EQ(1U, conf["1"].size());
  EXPECT_EQ(case_name, conf["1"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_optional_argument_option_no_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "2", "" },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("2"));
  ASSERT_EQ(1U, conf["2"].size());
  EXPECT_EQ("", conf["2"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_optional_argument_option_with_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "2", case_name },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("2"));
  ASSERT_EQ(1U, conf["2"].size());
  EXPECT_EQ(case_name, conf["2"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_no_argument_option_no_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "3", "" },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("3"));
  ASSERT_EQ(1U, conf["3"].size());
  EXPECT_EQ("", conf["3"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_no_argument_option_with_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "3", case_name },
  };

  EXPECT_THROW(options.load_from(hardcoded), std::runtime_error);
}


TEST_P(option_set, load_from_hardcoded_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "", case_name },
  };

  auto conf = options.load_from(hardcoded);

  ASSERT_EQ(1U, conf.positional_arguments().size());
  EXPECT_EQ(case_name, conf.positional_arguments()[0]);
}


TEST_P(option_set, load_from_hardcoded_multiple_argument)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "", case_name + "_0" },
    { "", case_name + "_1" },
  };

  auto conf = options.load_from(hardcoded);

  ASSERT_EQ(2U, conf.positional_arguments().size());
  EXPECT_EQ(case_name + "_0", conf.positional_arguments()[0]);
  EXPECT_EQ(case_name + "_1", conf.positional_arguments()[1]);
}


TEST_P(option_set, load_from_hardcoded_required_argument_option_no_argument_with_default)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "4", "" },
  };

  EXPECT_THROW(options.load_from(hardcoded), std::runtime_error);
}


TEST_P(option_set, load_from_hardcoded_required_argument_option_with_argument_with_default)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "4", case_name },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("4"));
  ASSERT_EQ(1U, conf["4"].size());
  EXPECT_EQ(case_name, conf["4"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_optional_argument_option_no_argument_with_default)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "5", "" },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("5"));
  ASSERT_EQ(1U, conf["5"].size());
  EXPECT_EQ("5th", conf["5"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


TEST_P(option_set, load_from_hardcoded_optional_argument_option_with_argument_with_default)
{
  fill(options);

  sal_test::hardcoded_config_t hardcoded =
  {
    { "5", case_name },
  };

  auto conf = options.load_from(hardcoded);

  EXPECT_TRUE(conf.has("5"));
  ASSERT_EQ(1U, conf["5"].size());
  EXPECT_EQ(case_name, conf["5"][0]);

  EXPECT_TRUE(conf.positional_arguments().empty());
}


} // namespace

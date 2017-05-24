#include <sal/program_options/option_set.hpp>
#include <sal/program_options/common.test.hpp>


namespace {

namespace po = sal::program_options;


struct option_set
  : public sal_test::with_value<bool>
{
  po::option_set_t options;


  const std::string &not_found () const
  {
    static const std::string data = "not_found";
    return data;
  }


  po::argument_map_t make_conf (const std::string &option)
  {
    sal_test::hardcoded_config_t config =
    {
      { "option_" + option, "argument_" + option + "_0" },
      { "option_" + option, "argument_" + option + "_1" },
      { "x_option_" + option, "x_argument_" + option },
      { "option_" + case_name, option },
      { "", "positional_" + option },
    };
    return options.load_from(config);
  }


  po::argument_map_t make_empty_conf ()
  {
    sal_test::hardcoded_config_t config{};
    return options.load_from(config);
  }
};


INSTANTIATE_TEST_CASE_P(program_options, option_set, testing::Values(true),);


TEST_P(option_set, add_no_names)
{
  EXPECT_THROW(options.add({}), po::no_option_name_error);
}


TEST_P(option_set, add_empty_name)
{
  EXPECT_THROW(options.add({ "" }), po::empty_option_name_error);
}


TEST_P(option_set, add_invalid_name)
{
  EXPECT_THROW(options.add({ "$name" }), po::invalid_option_name_error);
}


TEST_P(option_set, add_duplicate_name_same_option)
{
  EXPECT_THROW(options.add({ "name", "name" }), po::duplicate_option_name_error);
}


TEST_P(option_set, add_duplicate_name_different_option)
{
  EXPECT_THROW(
    options.add({ "name" }).add({ "name" }),
    po::duplicate_option_name_error
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
      po::requires_argument("FOURTH", "4th"),
      po::help("HTRUOF")
    )
    .add({ "5", "fifth" },
      po::optional_argument("FIFTH", "5th"),
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


TEST_P(option_set, find_option)
{
  fill(options);

  auto first = options.find("first");
  ASSERT_NE(nullptr, first);
  EXPECT_EQ(first, options.find("1"));
  EXPECT_TRUE(first->requires_argument());
  EXPECT_FALSE(first->optional_argument());
  EXPECT_FALSE(first->no_argument());
  EXPECT_EQ("FIRST", first->unit());
  EXPECT_TRUE(first->default_value().empty());

  auto second = options.find("second");
  ASSERT_NE(nullptr, second);
  EXPECT_EQ(second, options.find("2"));
  EXPECT_FALSE(second->requires_argument());
  EXPECT_TRUE(second->optional_argument());
  EXPECT_FALSE(second->no_argument());
  EXPECT_EQ("SECOND", second->unit());
  EXPECT_TRUE(second->default_value().empty());

  auto third = options.find("third");
  ASSERT_NE(nullptr, third);
  EXPECT_EQ(third, options.find("3"));
  EXPECT_FALSE(third->requires_argument());
  EXPECT_FALSE(third->optional_argument());
  EXPECT_TRUE(third->no_argument());
  EXPECT_TRUE(third->unit().empty());
  EXPECT_TRUE(third->default_value().empty());

  auto fourth = options.find("fourth");
  ASSERT_NE(nullptr, fourth);
  EXPECT_EQ(fourth, options.find("4"));
  EXPECT_TRUE(fourth->requires_argument());
  EXPECT_FALSE(fourth->optional_argument());
  EXPECT_FALSE(fourth->no_argument());
  EXPECT_EQ("FOURTH", fourth->unit());
  EXPECT_EQ("4th", fourth->default_value());

  auto fifth = options.find("fifth");
  ASSERT_NE(nullptr, fifth);
  EXPECT_EQ(fifth, options.find("5"));
  EXPECT_FALSE(fifth->requires_argument());
  EXPECT_TRUE(fifth->optional_argument());
  EXPECT_FALSE(fifth->no_argument());
  EXPECT_EQ("FIFTH", fifth->unit());
  EXPECT_EQ("5th", fifth->default_value());
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

  EXPECT_THROW(options.load_from(hardcoded), po::option_requires_argument_error);
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

  EXPECT_THROW(options.load_from(hardcoded), po::option_rejects_argument_error);
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

  EXPECT_THROW(options.load_from(hardcoded), po::option_requires_argument_error);
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


TEST_P(option_set, has)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");

  EXPECT_TRUE(options.has("option_a", { a, b }));
  EXPECT_TRUE(options.has("option_a", { b, a }));

  EXPECT_TRUE(options.has("option_b", { a, b }));
  EXPECT_TRUE(options.has("option_b", { b, a }));

  EXPECT_FALSE(options.has(not_found(), { a, b }));
  EXPECT_FALSE(options.has(not_found(), { b, a }));
}


TEST_P(option_set, front)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("argument_a_0", *options.front("option_a", { a, b }));
  EXPECT_EQ("argument_a_0", *options.front("option_a", { b, a }));
  EXPECT_EQ("argument_b_0", *options.front("option_b", { a, b }));
  EXPECT_EQ("argument_b_0", *options.front("option_b", { b, a }));
}


TEST_P(option_set, front_not_found)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ(nullptr, options.front(not_found(), { a, b }));
  EXPECT_EQ(nullptr, options.front(not_found(), { b, a }));
}


TEST_P(option_set, front_or_default)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("argument_a_0", options.front_or_default("option_a", { a, b }));
  EXPECT_EQ("argument_a_0", options.front_or_default("option_a", { b, a }));
  EXPECT_EQ("argument_b_0", options.front_or_default("option_b", { a, b }));
  EXPECT_EQ("argument_b_0", options.front_or_default("option_b", { b, a }));
}


TEST_P(option_set, front_or_default_not_found)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("4th", options.front_or_default("4", { a, b }));
  EXPECT_EQ("4th", options.front_or_default("4", { b, a }));
}


TEST_P(option_set, front_or_default_not_found_empty)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("", options.front_or_default(not_found(), { a, b }));
  EXPECT_EQ("", options.front_or_default(not_found(), { b, a }));
}


TEST_P(option_set, back)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("argument_a_1", *options.back("option_a", { a, b }));
  EXPECT_EQ("argument_a_1", *options.back("option_a", { b, a }));
  EXPECT_EQ("argument_b_1", *options.back("option_b", { a, b }));
  EXPECT_EQ("argument_b_1", *options.back("option_b", { b, a }));
}


TEST_P(option_set, back_not_found)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ(nullptr, options.back(not_found(), { a, b }));
  EXPECT_EQ(nullptr, options.back(not_found(), { b, a }));
}


TEST_P(option_set, back_or_default)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("argument_a_1", options.back_or_default("option_a", { a, b }));
  EXPECT_EQ("argument_a_1", options.back_or_default("option_a", { b, a }));
  EXPECT_EQ("argument_b_1", options.back_or_default("option_b", { a, b }));
  EXPECT_EQ("argument_b_1", options.back_or_default("option_b", { b, a }));
}


TEST_P(option_set, back_or_default_not_found)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("4th", options.back_or_default("4", { a, b }));
  EXPECT_EQ("4th", options.back_or_default("4", { b, a }));
}


TEST_P(option_set, back_or_default_not_found_empty)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  EXPECT_EQ("", options.back_or_default(not_found(), { a, b }));
  EXPECT_EQ("", options.back_or_default(not_found(), { b, a }));
}


TEST_P(option_set, merge)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  auto ab = options.merge("option_" + case_name, { a, b });
  ASSERT_EQ(2U, ab.size());
  EXPECT_EQ("a", ab[0]);
  EXPECT_EQ("b", ab[1]);
}


TEST_P(option_set, merge_reverse)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  auto ba = options.merge("option_" + case_name, { b, a });
  ASSERT_EQ(2U, ba.size());
  EXPECT_EQ("b", ba[0]);
  EXPECT_EQ("a", ba[1]);
}


TEST_P(option_set, merge_not_found)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  auto ab = options.merge(not_found(), { a, b });
  EXPECT_TRUE(ab.empty());
}


TEST_P(option_set, merge_empty)
{
  fill(options);
  auto result = options.merge(case_name, {});
  EXPECT_TRUE(result.empty());
}


TEST_P(option_set, positional_arguments)
{
  fill(options);
  auto a = make_conf("a"), b = make_conf("b");
  auto pos_args = options.positional_arguments({ a, b });
  ASSERT_EQ(2U, pos_args.size());
  EXPECT_EQ("positional_a", pos_args[0]);
  EXPECT_EQ("positional_b", pos_args[1]);
}


TEST_P(option_set, positional_arguments_not_found)
{
  fill(options);
  auto a = make_empty_conf(), b = make_empty_conf();
  auto pos_args = options.positional_arguments({ a, b });
  EXPECT_TRUE(pos_args.empty());
}


TEST_P(option_set, positional_arguments_empty)
{
  fill(options);
  auto pos_args = options.positional_arguments({});
  EXPECT_TRUE(pos_args.empty());
}


} // namespace

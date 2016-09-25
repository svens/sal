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


} // namespace

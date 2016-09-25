# docs
list(APPEND sal_docs
  sal/program_options/Options.md
)


# sources
list(APPEND sal_sources
  sal/program_options/__bits/option_set.hpp
  sal/program_options/option_set.hpp
  sal/program_options/option_set.cpp
)


# unittests
list(APPEND sal_unittests
  sal/program_options/common.test.hpp
  sal/program_options/option_set.test.cpp
)

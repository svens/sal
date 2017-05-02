# docs
list(APPEND sal_docs
)


# sources
list(APPEND sal_sources
  sal/crypto/__bits/algorithm.hpp
  sal/crypto/__bits/algorithm.cpp
  sal/crypto/hash.hpp
)


# unittests
list(APPEND sal_unittests
  sal/crypto/hash.test.cpp
)

# docs
list(APPEND sal_docs
)


# sources
list(APPEND sal_sources
  sal/crypto/__bits/hash_algorithm.hpp
  sal/crypto/__bits/hash_algorithm.cpp
  sal/crypto/hash.hpp
  sal/crypto/hmac.hpp
  sal/crypto/random.hpp
  sal/crypto/random.cpp
)


# unittests
list(APPEND sal_unittests
  sal/crypto/hash.test.cpp
  sal/crypto/hmac.test.cpp
  sal/crypto/random.test.cpp
)

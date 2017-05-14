# docs
list(APPEND sal_docs
)


# sources
list(APPEND sal_sources
  sal/crypto/__bits/digest.hpp
  sal/crypto/hash.hpp
  sal/crypto/hash.cpp
  sal/crypto/hmac.hpp
  sal/crypto/hmac.cpp
  sal/crypto/random.hpp
  sal/crypto/random.cpp
)


# unittests
list(APPEND sal_unittests
  sal/crypto/hash.test.cpp
  sal/crypto/hmac.test.cpp
  sal/crypto/random.test.cpp
)

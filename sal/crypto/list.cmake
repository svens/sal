# sources
list(APPEND sal_sources
  sal/crypto/__bits/x509.hpp
  sal/crypto/__bits/x509.cpp
  sal/crypto/certificate.hpp
  sal/crypto/certificate.cpp
  sal/crypto/error.hpp
  sal/crypto/error.cpp
  sal/crypto/__bits/digest.hpp
  sal/crypto/hash.hpp
  sal/crypto/hash.cpp
  sal/crypto/hmac.hpp
  sal/crypto/hmac.cpp
  sal/crypto/key.hpp
  sal/crypto/key.cpp
  sal/crypto/oid.hpp
  sal/crypto/oid.cpp
  sal/crypto/random.hpp
  sal/crypto/random.cpp
)


# unittests
list(APPEND sal_unittests_sources
  sal/crypto/common.test.hpp
  sal/crypto/common.test.cpp
  sal/crypto/certificate.test.cpp
  sal/crypto/hash.test.cpp
  sal/crypto/hmac.test.cpp
  sal/crypto/key.test.cpp
  sal/crypto/oid.test.cpp
  sal/crypto/random.test.cpp
)

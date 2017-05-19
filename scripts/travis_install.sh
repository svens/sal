case "${TRAVIS_OS_NAME}-${CC}" in
  linux-clang)
    CMAKE_C_COMPILER=clang-3.8
    CMAKE_CXX_COMPILER=clang++-3.8
    ;;

  linux-gcc)
    CMAKE_C_COMPILER=gcc-6
    CMAKE_CXX_COMPILER=g++-6
    GCOV=gcov-6
    if test "${BUILD_TYPE}" = "Coverage"; then
      curl -sO http://ftp.debian.org/debian/pool/main/l/lcov/lcov_1.13.orig.tar.gz
      gunzip -c lcov_1.13.orig.tar.gz | tar xvf -
      make -C lcov-1.13 install PREFIX=${HOME} BIN_DIR=${HOME}/bin
      gem install coveralls-lcov
    fi
    ;;

  osx-clang)
    CMAKE_C_COMPILER=clang
    CMAKE_CXX_COMPILER=clang++
    brew update
    brew install cmake || true
    ;;

  osx-gcc)
    CMAKE_C_COMPILER=gcc-7
    CMAKE_CXX_COMPILER=g++-7
    brew update
    brew install cmake gcc || true
    ;;
esac

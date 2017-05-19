case "${TRAVIS_OS_NAME}-${CC}" in
  linux-clang)
    CC=clang-3.8
    CXX=clang++-3.8
    ;;

  linux-gcc)
    CC=gcc-6
    CXX=g++-6
    GCOV=gcov-6
    if test "${BUILD_TYPE}" = "Coverage"; then
      curl -sO http://ftp.debian.org/debian/pool/main/l/lcov/lcov_1.13.orig.tar.gz
      gunzip -c lcov_1.13.orig.tar.gz | tar xvf -
      make -C lcov-1.13 install PREFIX=${HOME} BIN_DIR=${HOME}/bin
      gem install coveralls-lcov
    fi
    ;;

  osx-clang)
    CC=clang
    CXX=clang++
    brew update
    brew install cmake || true
    ;;

  osx-gcc)
    CC=gcc-7
    CXX=g++-7
    brew update
    brew install cmake gcc || true
    ;;
esac

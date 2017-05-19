case "${TRAVIS_OS_NAME}-${CC}" in
  linux-clang)
    export CC=clang-3.8
    export CXX=clang++-3.8
    ;;

  linux-gcc)
    export CC=gcc-6
    export CXX=g++-6
    export GCOV=gcov-6
    if test "${BUILD_TYPE}" = "Coverage"; then
      curl -sO http://ftp.debian.org/debian/pool/main/l/lcov/lcov_1.13.orig.tar.gz
      gunzip -c lcov_1.13.orig.tar.gz | tar xvf -
      make -C lcov-1.13 install PREFIX=${HOME} BIN_DIR=${HOME}/bin
      gem install coveralls-lcov
    fi
    ;;

  osx-clang)
    export CC=clang
    export CXX=clang++
    brew update
    brew install cmake || true
    ;;

  osx-gcc)
    export CC=gcc-7
    export CXX=g++-7
    brew update
    brew install cmake gcc || true
    ;;
esac

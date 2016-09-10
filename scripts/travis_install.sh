case "${TRAVIS_OS_NAME}-${CC}" in
  linux-clang)
    export CC=clang-3.8 CXX=clang++-3.8
    ;;

  linux-gcc)
    export CC=gcc-6 CXX=g++-6 GCOV=gcov-6
    if test "${TEST_TARGET}" = "gen-cov"; then
      curl -sO http://ftp.debian.org/debian/pool/main/l/lcov/lcov_1.12.orig.tar.gz
      gunzip -c lcov_1.12.orig.tar.gz | tar xvf -
      make -C lcov-1.12 install PREFIX=${HOME} BIN_DIR=${HOME}/bin
    fi
    ;;

  osx-clang)
    export CC=clang CXX=clang++
    brew update
    brew install cmake || true
    ;;

  osx-gcc)
    export CC=gcc-6 CXX=g++-6
    brew update
    brew install cmake gcc || true
    ;;
esac

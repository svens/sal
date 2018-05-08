case "${TRAVIS_OS_NAME}-${CC}" in
  linux-clang)
    export CC=clang
    export CXX=clang++
    ;;

  linux-gcc)
    export CC=gcc-7
    export CXX=g++-7
    export GCOV=gcov-7
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
    ;;

  osx-gcc)
    export CC=gcc-7
    export CXX=g++-7

    # oclint conflicts with gcc (include/c++ symlink)
    brew cask uninstall oclint || true

    brew update
    brew install gcc || true
    ;;
esac

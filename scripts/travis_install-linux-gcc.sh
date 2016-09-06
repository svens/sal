export CC=gcc-6
export CXX=g++-6

if test "${TEST_TARGET}" = "gen-cov"; then
  export GCOV=gcov-6

  # install lcov-1.12
  curl -sO http://ftp.debian.org/debian/pool/main/l/lcov/lcov_1.12.orig.tar.gz
  gunzip -c lcov_1.12.orig.tar.gz | tar xvf -
  make -C lcov-1.12 install PREFIX=${HOME} BIN_DIR=${HOME}/bin
fi

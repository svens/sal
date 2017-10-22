if test "${BUILD_TYPE}" = "Coverity"; then
  cmake . -DUNITTESTS=no -DDOCS=no -DBENCH=no
  bash <(curl -s https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh)
elif test "${BUILD_TYPE}" = "Coverage"; then
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DDOCS=no -DBENCH=no
  cmake --build .
  travis_wait cmake --build . --target gen-cov -- ARGS=--output-on-failure
else
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DDOCS=no -DBENCH=no
  cmake --build .
  cmake --build . --target test -- ARGS=--output-on-failure
fi

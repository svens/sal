if test "${BUILD_TYPE}" = "Coverity"; then
  cmake . -DSAL_UNITTESTS=no -DSAL_DOCS=no -DSAL_BENCH=no
  bash <(curl -s https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh)
elif test "${BUILD_TYPE}" = "Coverage"; then
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DSAL_DOCS=no -DSAL_BENCH=no
  cmake --build .
  cmake --build . --target gen-cov -- ARGS=--output-on-failure
else
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DSAL_DOCS=no -DSAL_BENCH=no
  cmake --build .
  cmake --build . --target test -- ARGS=--output-on-failure
fi

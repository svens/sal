if test "${BUILD_TYPE}" = "Coverity"; then
  cmake . -Dsal_unittests=no -Dsal_docs=no -Dsal_bench=no
  bash <(curl -s https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh)
elif test "${BUILD_TYPE}" = "Coverage"; then
  if test "${TRAVIS_OS_NAME}" = "osx"; then
    # TODO remove once Travis-CI has required OSX SDK as default
    cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 -Dsal_docs=no -Dsal_bench=no
  else
    cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -Dsal_docs=no -Dsal_bench=no
  fi
  cmake --build .
  travis_wait cmake --build . --target sal-cov -- ARGS=--output-on-failure
else
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -Dsal_docs=no -Dsal_bench=no
  cmake --build .
  cmake --build . --target test -- ARGS=--output-on-failure
fi

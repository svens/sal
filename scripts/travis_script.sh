if test "${BUILD_TYPE}" = "Coverity"; then
  cmake . \
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
    -DSAL_UNITTESTS=no \
    -DSAL_BENCH=no
  bash <(curl -s https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh)
elif test "${BUILD_TYPE}" = "Coverage"; then
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
    -DSAL_UNITTESTS=yes \
    -DSAL_BENCH=no
  cmake --build .
  cmake --build . --target gen-cov -- ARGS=--output-on-failure
else
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  cmake --build .
  cmake --build . --target test -- ARGS=--output-on-failure
fi

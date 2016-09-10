if test "${BUILD_TYPE}" = "Coverity"; then
  cmake . \
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
    -DSAL_UNITTESTS=no \
    -DSAL_BENCH=no
  bash <(curl -s https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh)
else
  cmake . \
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
  cmake --build .
  cmake --build . --target ${TEST_TARGET} -- ARGS=--output-on-failure
fi

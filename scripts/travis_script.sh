if test "${BUILD_TYPE}" = "Coverity"; then
  cmake . -Dsal_unittests=no -Dsal_docs=no -Dsal_bench=no
  bash <(curl -s https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh)
elif test "${BUILD_TYPE}" = "Coverage"; then
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -Dsal_docs=no -Dsal_bench=no
  cmake --build .
  travis_wait cmake --build . --target sal-cov -- ARGS=--output-on-failure
else
  cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -Dsal_docs=no -Dsal_bench=no
  cmake --build .
  #cmake --build . --target test -- ARGS=--output-on-failure
  ./unittests --gtest_filter="*:-net_ip_resolver/?.resolve_v6_host_localhost"
fi

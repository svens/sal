if test "${BUILD_TYPE}" = "Coverage"; then
  bash <(curl -s https://codecov.io/bash) -f sal.info
fi

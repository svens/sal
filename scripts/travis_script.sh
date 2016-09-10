cmake . -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
cmake --build .
cmake --build . --target ${TEST_TARGET} -- ARGS=--output-on-failure

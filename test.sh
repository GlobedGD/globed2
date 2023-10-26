cd tests/build
cmake ..
cmake --build .
./globed_tests "$@"
cd ../../

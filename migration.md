move_to_old.sh
move_to_phasar_folder.sh
config -> inclure from utils but utils links to config => moved IO.h to config
=> resolved with fabian copied small code part
utils uses includes from PhasarLLVM but PhasarLLVM links to utils
=> split TypeTrait / TypeTraitsLLVM
utils uses includes from DB but DB links to utils
- mkdir -p phasar/db/include/phasar/Utils && git mv phasar/utils/include/phasar/Utils/LLVMShorthands.h phasar/db/include/phasar/Utils
  - git mv phasar/utils/src/LLVMShorthands.cpp phasar/db/src/
  - mkdir -p phasar/db/test/src/ && git mv phasar/utils/test/src/LLVMIRToSrcTest.cpp phasar/db/test/src/
  - git mv phasar/utils/test/src/LLVMShorthandsTest.cpp phasar/db/test/src/
util tests
- mkdir -p phasar/test-utils/include/phasar/Utils/ && git mv /home/ubuntu/git/phasar-itst/unittests/TestUtils/TestConfig.h phasar/test-utils/include/phasar/Utils/
- mkdir -p phasar/utils/test/resource && git mv test/text_test_code/test.txt /phasar/utils/test/resource

- test/clang phasar-clang-test
- test/json phasar-llvm-test -> taint
- test/llvm phasar-llvm-test
- test/text_test phasar-utils
- test/build_systems_tests !? Fabian kennt es nicht


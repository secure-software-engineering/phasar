move_to_old.sh
move_to_phasar_folder.sh
config -> inclure from utils but utils links to config => moved IO.h to config
=> resolved with fabian copied small code part
utils uses includes from PhasarLLVM but PhasarLLVM links to utils
=> split TypeTrait / TypeTraitsLLVM
utils uses includes from DB but DB links to utils -> dependency moved later to phasarLLVM
- mkdir -p phasar/llvm/include/phasar/Utils/ && phasar/utils/include/phasar/Utils/LLVMShorthands.h phasar/llvm/include/phasar/Utils/
  - mkdir -p phasar/llvm/src/Utils/ && git mv phasar/utils/src/LLVMShorthands.cpp phasar/llvm/src/Utils/
  - mkdir -p phasar/db/test/src/ && git mv phasar/utils/test/src/LLVMIRToSrcTest.cpp phasar/llvm/test/src/Utils/
  - mkdir -p phasar/llvm/test/src/ && git mv phasar/utils/test/src/LLVMShorthandsTest.cpp phasar/llvm/test/src/
util tests
- mkdir -p phasar/test-utils/include/phasar/Utils/ && git mv /home/ubuntu/git/phasar-itst/unittests/TestUtils/TestConfig.h phasar/test-utils/include/
- mkdir -p phasar/utils/test/resource && git mv test/text_test_code/test.txt /phasar/utils/test/resource

DB uses phasarLLVM include but phasarLLVM links to DB
- mkdir -p phasar/llvm/include/phasar/DB/ && git mv phasar/db/include/phasar/DB/ProjectIRDB.h phasar/llvm/include/phasar/DB/
- mkdir -p phasar/llvm/src/phasar/DB/ && git mv phasar/db/src/ProjectIRDB.cpp phasar/llvm/src/phasar/DB/

- test/clang phasar-clang-test
- test/json phasar-llvm-test -> taint
- test/llvm phasar-llvm-test
- test/text_test phasar-utils
- test/build_systems_tests !? Fabian kennt es nicht


move_to_old.sh
move_to_phasar_folder.sh
config -> inclure from utils but utils links to config => moved IO.h to config
//- mkdir -p phasar/config/include/phasar/Utils/ && git mv phasar/utils/include/phasar/Utils/IO.h phasar/config/include/phasar/Utils/IO.h
//  - git mv phasar/utils/src/IO.cpp phasar/config/src/
utils uses includes from PhasarLLVM but PhasarLLVM links to utils
// - git mv phasar/utils/include/phasar/Utils/TypeTraits.h phasar/llvm/include/phasar/PhasarLLVM/Utils/TypeTraits.h
//  - git mv phasar/utils/include/phasar/Utils/Utilities.h phasar/llvm/include/phasar/PhasarLLVM/Utils/
//  - git mv phasar/utils/src/Utilities.cpp phasar/llvm/src/
utils uses includes from DB but DB links to utils
- mkdir -p phasar/db/include/phasar/Utils && git mv phasar/utils/include/phasar/Utils/LLVMShorthands.h phasar/db/include/phasar/Utils
  - git mv phasar/utils/src/LLVMShorthands.cpp phasar/db/src/
  - mkdir -p phasar/db/test/src/ && git mv phasar/utils/test/src/LLVMIRToSrcTest.cpp phasar/db/test/src/
  - git mv phasar/utils/test/src/LLVMShorthandsTest.cpp phasar/db/test/src/
util tests
- mkdir -p phasar/test-utils/include/phasar/Utils/ && git mv /home/ubuntu/git/phasar-itst/unittests/TestUtils/TestConfig.h phasar/test-utils/include/phasar/Utils/

- test/clang phasar-clang-test
- test/json phasar-llvm-test -> taint
- test/llvm phasar-llvm-test
- test/text_test phasar-utils
- test/build_systems_tests !? Fabian kennt es nicht


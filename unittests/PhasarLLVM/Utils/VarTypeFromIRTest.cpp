#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Casting.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <optional>

class GetDITypeFromValueTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("/types/");

  std::optional<psr::LLVMProjectIRDB> IRDBBuf;

  // Helper function to load LLVM IR from file
  [[nodiscard]] llvm::Module *loadIRFromFile(const llvm::Twine &Filename) {
    IRDBBuf.emplace(PathToLLFiles + Filename);
    return IRDBBuf->getModule();
  }

  // Helper to find instruction by name/ID
  [[nodiscard]] const llvm::Instruction *
  findInstructionByPSRId(llvm::Module *M, llvm::StringRef Id) {
    return llvm::dyn_cast_if_present<llvm::Instruction>(
        psr::fromMetaDataId(IRDBBuf.value(), Id));
  }

  // Helper to find alloca by debug variable name
  llvm::AllocaInst *findAllocaByName(llvm::Module *M, llvm::StringRef VarName) {
    for (auto &F : *M) {
      for (auto &I : llvm::instructions(F)) {
        if (auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
          if (psr::getVarNameFromIR(Alloca) == VarName) {
            return Alloca;
          }
        }
      }
    }
    return nullptr;
  }

  // Helper to find function argument by index
  [[nodiscard]] llvm::Argument *
  findFunctionArg(llvm::Module *M, llvm::StringRef FuncName, unsigned ArgIdx) {
    if (auto *F = M->getFunction(FuncName)) {
      if (ArgIdx < F->arg_size()) {
        return F->getArg(ArgIdx);
      }
    }
    return nullptr;
  }

  // Generic instruction finder template
  template <typename InstrType>
  [[nodiscard]] const InstrType *
  findFirstInstructionOfType(llvm::Module *M, llvm::StringRef FuncName = "") {
    const auto ProcessFunction =
        [](const llvm::Function &F) -> const InstrType * {
      for (const auto &I : llvm::instructions(F)) {
        if (const auto *Instr = llvm::dyn_cast<InstrType>(&I)) {
          return Instr;
        }
      }

      return nullptr;
    };

    if (!FuncName.empty()) {
      if (const auto *F = M->getFunction(FuncName)) {
        return ProcessFunction(*F);
      }
      return nullptr;
    }

    // Search all functions
    for (const auto &F : *M) {
      if (const auto *Result = ProcessFunction(F)) {
        return Result;
      }
    }
    return nullptr;
  }

  // Find instruction with specific predicate
  template <typename InstrType, typename PredicateType>
  const InstrType *findInstructionWithPredicate(llvm::Module *M,
                                                PredicateType Predicate,
                                                llvm::StringRef FuncName = "") {
    auto ProcessFunction = [&](const llvm::Function &F) -> const InstrType * {
      for (const auto &I : llvm::instructions(F)) {
        if (const auto *Instr = llvm::dyn_cast<InstrType>(&I)) {
          if (Predicate(Instr)) {
            return Instr;
          }
        }
      }

      return nullptr;
    };

    if (!FuncName.empty()) {
      if (const auto *F = M->getFunction(FuncName)) {
        return ProcessFunction(*F);
      }
      return nullptr;
    }

    for (const auto &F : *M) {
      if (const auto *Result = ProcessFunction(F)) {
        return Result;
      }
    }
    return nullptr;
  }

  // Specialized finders using the template
  [[nodiscard]] const llvm::CallInst *
  findVirtualCall(llvm::Module *M, llvm::StringRef FuncName = "") {
    return findInstructionWithPredicate<llvm::CallInst>(
        M, [](const llvm::CallInst *CI) { return CI->isIndirectCall(); },
        FuncName);
  }

  [[nodiscard]] const llvm::GetElementPtrInst *
  findComplexGEP(llvm::Module *M, unsigned MinIndices = 2,
                 llvm::StringRef FuncName = "") {
    return findInstructionWithPredicate<llvm::GetElementPtrInst>(
        M,
        [MinIndices](const llvm::GetElementPtrInst *GEP) {
          return GEP->getNumIndices() > MinIndices;
        },
        FuncName);
  }

  [[nodiscard]] const llvm::PHINode *
  findPointerPhi(llvm::Module *M, llvm::StringRef FuncName = "") {
    return findInstructionWithPredicate<llvm::PHINode>(
        M, [](const llvm::PHINode *PN) { return PN->getType()->isPointerTy(); },
        FuncName);
  }

  // Type validation functions returning AssertionResult
  testing::AssertionResult
  validatePointerType(const llvm::Value *V,
                      llvm::StringRef ExpectedPointeeTypeName = "",
                      bool RequirePointer = true) {
    auto *DIType = psr::getVarTypeFromIR(V);
    if (!DIType) {
      return testing::AssertionFailure() << "getVarTypeFromIR returned nullptr";
    }

    auto *PtrType = llvm::dyn_cast<llvm::DIDerivedType>(DIType);
    if (RequirePointer) {
      if (!PtrType) {
        return testing::AssertionFailure()
               << "Expected DIDerivedType (pointer), got "
               << psr::llvmTypeToString(DIType);
      }

      if (PtrType->getTag() != llvm::dwarf::DW_TAG_pointer_type) {
        return testing::AssertionFailure() << "Expected pointer type tag, got "
                                           << psr::llvmTypeToString(DIType);
      }
    }

    auto *BaseType = PtrType ? PtrType->getBaseType() : DIType;
    if (!BaseType) {
      return testing::AssertionFailure() << "Pointer has no base type";
    }

    if (!ExpectedPointeeTypeName.empty()) {
      llvm::StringRef ActualName;
      if (auto *CompositeType =
              llvm::dyn_cast<llvm::DICompositeType>(BaseType)) {
        ActualName = CompositeType->getName();
      } else if (auto *BasicType =
                     llvm::dyn_cast<llvm::DIBasicType>(BaseType)) {
        ActualName = BasicType->getName();
      }

      if (ActualName != ExpectedPointeeTypeName) {
        return testing::AssertionFailure()
               << "Expected pointee type '" << ExpectedPointeeTypeName.str()
               << "' but got '" << ActualName.str() << "'";
      }
    }

    return testing::AssertionSuccess();
  }

  testing::AssertionResult validateBasicType(const llvm::Value *V,
                                             llvm::StringRef ExpectedTypeName,
                                             unsigned ExpectedSize = 0) {
    auto *DIType = psr::getVarTypeFromIR(V);
    if (!DIType) {
      return testing::AssertionFailure() << "getVarTypeFromIR returned nullptr";
    }

    auto *BasicType = llvm::dyn_cast<llvm::DIBasicType>(DIType);
    if (!BasicType) {
      return testing::AssertionFailure()
             << "Expected DIBasicType, got " << DIType->getMetadataID();
    }

    if (BasicType->getName() != ExpectedTypeName) {
      return testing::AssertionFailure()
             << "Expected type '" << ExpectedTypeName.str() << "' but got '"
             << BasicType->getName().str() << "'";
    }

    if (ExpectedSize != 0 && BasicType->getSizeInBits() != ExpectedSize) {
      return testing::AssertionFailure()
             << "Expected size " << ExpectedSize << " but got "
             << BasicType->getSizeInBits();
    }

    return testing::AssertionSuccess();
  }

  testing::AssertionResult
  validateTypeIsOneOf(const llvm::Value *V,
                      llvm::ArrayRef<std::string> ExpectedTypeNames) {
    auto *DIType = psr::getVarTypeFromIR(V);
    if (!DIType) {
      return testing::AssertionFailure() << "getVarTypeFromIR returned nullptr";
    }

    std::string_view ActualTypeName;

    if (auto *PtrType = llvm::dyn_cast<llvm::DIDerivedType>(DIType)) {
      if (PtrType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
        auto *BaseType = PtrType->getBaseType();
        if (auto *CompositeType =
                llvm::dyn_cast<llvm::DICompositeType>(BaseType)) {
          ActualTypeName = CompositeType->getName();
        } else if (auto *BasicType =
                       llvm::dyn_cast<llvm::DIBasicType>(BaseType)) {
          ActualTypeName = BasicType->getName();
        }
      }
    } else if (auto *CompositeType =
                   llvm::dyn_cast<llvm::DICompositeType>(DIType)) {
      ActualTypeName = CompositeType->getName();
    } else if (auto *BasicType = llvm::dyn_cast<llvm::DIBasicType>(DIType)) {
      ActualTypeName = BasicType->getName();
    }

    bool Found = llvm::is_contained(ExpectedTypeNames, ActualTypeName);
    if (!Found) {
      return testing::AssertionFailure()
             << "Type '" << ActualTypeName << "' not found in expected types: ["
             << llvm::join(ExpectedTypeNames, ", ") << "]";
    }

    return testing::AssertionSuccess();
  }

  // Additional validation for specific DWARF attributes
  testing::AssertionResult
  validateBasicTypeWithEncoding(const llvm::Value *V,
                                llvm::StringRef ExpectedTypeName,
                                unsigned ExpectedEncoding) {
    auto BasicResult = validateBasicType(V, ExpectedTypeName);
    if (!BasicResult) {
      return BasicResult;
    }

    auto *DIType = psr::getVarTypeFromIR(V);
    auto *BasicType = llvm::dyn_cast_if_present<llvm::DIBasicType>(DIType);

    if (!BasicType) {
      return testing::AssertionFailure()
             << "Expected DIBasicType, got " << psr::llvmTypeToString(DIType);
    }

    if (BasicType->getEncoding() != ExpectedEncoding) {
      return testing::AssertionFailure()
             << "Expected encoding " << ExpectedEncoding << " but got "
             << BasicType->getEncoding();
    }

    return testing::AssertionSuccess();
  }

  // Helper to get types when validation passes (for additional checks)
  [[nodiscard]] llvm::DIType *getValidatedType(const llvm::Value *V) {
    return psr::getVarTypeFromIR(V);
  }

  [[nodiscard]] llvm::DIBasicType *getValidatedBasicType(const llvm::Value *V) {
    return llvm::dyn_cast_if_present<llvm::DIBasicType>(
        psr::getVarTypeFromIR(V));
  }

  [[nodiscard]] llvm::DIDerivedType *
  getValidatedPointerType(const llvm::Value *V) {
    auto *DIType = psr::getVarTypeFromIR(V);
    auto *PtrType = llvm::dyn_cast_if_present<llvm::DIDerivedType>(DIType);
    if (PtrType && PtrType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
      return PtrType;
    }
    return nullptr;
  }

  // Module loading with assertion
  [[nodiscard]] llvm::Module *
  loadAndValidateModule(const std::string &Filename) {
    auto *M = loadIRFromFile(Filename);
    EXPECT_NE(M, nullptr) << "Failed to load module: " << Filename;
    return M;
  }
};
TEST_F(GetDITypeFromValueTest, AllocaWithDebugInfo) {
  auto *M = loadAndValidateModule("test_alloca_simple_c_dbg.ll");
  if (!M) {
    return;
  }

  llvm::AllocaInst *Alloca = findAllocaByName(M, "x");
  ASSERT_NE(Alloca, nullptr);

  ASSERT_TRUE(validateBasicType(Alloca, "int", 32));

  // Additional specific checks if needed
  auto *BasicType = getValidatedBasicType(Alloca);
  EXPECT_EQ(BasicType->getEncoding(), llvm::dwarf::DW_ATE_signed);
}

TEST_F(GetDITypeFromValueTest, LoadFromAlloca) {
  auto *M = loadAndValidateModule("test_alloca_simple_c_dbg.ll");
  if (!M) {
    return;
  }

  const auto *LoadInst = findInstructionByPSRId(M, "2");
  ASSERT_NE(LoadInst, nullptr);
  ASSERT_TRUE(llvm::isa<llvm::LoadInst>(LoadInst));

  EXPECT_TRUE(validateBasicType(LoadInst, "int"));
}

TEST_F(GetDITypeFromValueTest, ArrayGEP) {
  auto *M = loadAndValidateModule("test_array_gep_c_dbg.ll");
  if (!M) {
    return;
  }

  const auto *GEPInst = findInstructionByPSRId(M, "6");
  ASSERT_NE(GEPInst, nullptr);
  ASSERT_TRUE(llvm::isa<llvm::GetElementPtrInst>(GEPInst));

  EXPECT_TRUE(validateBasicType(GEPInst, "int"));
}

TEST_F(GetDITypeFromValueTest, VirtualFunctionCall) {
  auto *M = loadAndValidateModule("test_virtual_call_cpp_dbg.ll");
  if (!M) {
    return;
  }

  const llvm::CallInst *VCall = findVirtualCall(M, "_Z17test_virtual_callP1B");
  ASSERT_NE(VCall, nullptr);

  const llvm::Value *arg = VCall->getArgOperand(0);
  ASSERT_NE(arg, nullptr);

  EXPECT_TRUE(validatePointerType(arg, "B"));
}

TEST_F(GetDITypeFromValueTest, MultipleInheritanceVirtualCall) {
  auto *M = loadAndValidateModule("test_multiple_inheritance_cpp_dbg.ll");
  if (!M) {
    return;
  }

  const llvm::CallInst *VCall =
      findVirtualCall(M, "_Z25test_multiple_inheritanceP7Derived");
  ASSERT_NE(VCall, nullptr);

  const llvm::Value *thisPtr = VCall->getArgOperand(0);
  ASSERT_NE(thisPtr, nullptr);

  EXPECT_TRUE(validateTypeIsOneOf(thisPtr, {"Base1", "Base2", "Derived"}));
}

TEST_F(GetDITypeFromValueTest, SignedIntegerType) {
  auto *M = loadAndValidateModule("test_alloca_simple_c_dbg.ll");
  if (!M) {
    return;
  }

  llvm::AllocaInst *Alloca = findAllocaByName(M, "x");
  ASSERT_NE(Alloca, nullptr);

  // Using the specialized validation function
  EXPECT_TRUE(
      validateBasicTypeWithEncoding(Alloca, "int", llvm::dwarf::DW_ATE_signed));
}

// Example of using the getter functions for complex validation
TEST_F(GetDITypeFromValueTest, ComplexTypeInspection) {
  auto *M = loadAndValidateModule("test_multiple_inheritance_cpp_dbg.ll");
  if (!M) {
    return;
  }

  const llvm::CallInst *VCall = findVirtualCall(M);
  ASSERT_NE(VCall, nullptr);

  const llvm::Value *ThisPtr = VCall->getArgOperand(0);
  ASSERT_NE(ThisPtr, nullptr);

  // First validate it's a pointer
  ASSERT_TRUE(validatePointerType(ThisPtr));

  // Then get the pointer type for detailed inspection
  auto *PtrType = getValidatedPointerType(ThisPtr);
  ASSERT_NE(PtrType, nullptr);

  // Perform additional checks on the pointer type
  auto *BaseType =
      llvm::dyn_cast<llvm::DICompositeType>(PtrType->getBaseType());
  ASSERT_NE(BaseType, nullptr);

  // Check for inheritance relationships, member count, etc.
  EXPECT_GT(BaseType->getElements().size(), 0U) << "Class should have members";
}

// Tests for opaque pointer arithmetic challenges
TEST_F(GetDITypeFromValueTest, OpaquePointerArithmetic_BasicGEP) {
  auto *M = loadAndValidateModule("test_opaque_pointer_arithmetic_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find GEP instruction for array access: &points[3]
  const auto *ArrayGEP = findInstructionWithPredicate<llvm::GetElementPtrInst>(
      M,
      [](const llvm::GetElementPtrInst *GEP) {
        return GEP->getNumIndices() >= 1; // Basic array access
      },
      "_Z23test_pointer_arithmeticv");
  ASSERT_NE(ArrayGEP, nullptr);

  EXPECT_TRUE(validatePointerType(ArrayGEP, "Point", false));
}

TEST_F(GetDITypeFromValueTest, OpaquePointerArithmetic_PointerIncrement) {
  auto *M = loadAndValidateModule("test_opaque_pointer_arithmetic_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find GEP for pointer arithmetic: p1 + 2
  const auto *PtrArithGEP =
      findInstructionWithPredicate<llvm::GetElementPtrInst>(
          M, [](const llvm::GetElementPtrInst *GEP) {
            // Look for GEP that adds to an existing pointer
            return GEP->getPointerOperand()->getType()->isPointerTy();
          });
  ASSERT_NE(PtrArithGEP, nullptr);

  EXPECT_TRUE(validatePointerType(PtrArithGEP, "Point", false));
}

TEST_F(GetDITypeFromValueTest, OpaquePointerArithmetic_StructMemberAccess) {
  auto *M = loadAndValidateModule("test_opaque_pointer_arithmetic_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find GEP for struct member access: &lines[1].start
  const auto *MemberGEP = findInstructionWithPredicate<llvm::GetElementPtrInst>(
      M, [](const llvm::GetElementPtrInst *GEP) {
        return GEP->getNumIndices() >= 2; // Array + struct member access
      });
  ASSERT_NE(MemberGEP, nullptr);

  EXPECT_TRUE(validatePointerType(MemberGEP, "Point", false));
}

TEST_F(GetDITypeFromValueTest, OpaquePointerArithmetic_FieldAccess) {
  auto *M = loadAndValidateModule("test_opaque_pointer_arithmetic_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find GEP for field access: &p1->x
  const auto *FieldGEP = findInstructionWithPredicate<llvm::GetElementPtrInst>(
      M, [](const llvm::GetElementPtrInst *GEP) {
        // Look for member access that results in int*
        return GEP->getSourceElementType()->isStructTy() &&
               GEP->getSourceElementType()->getStructName().contains("Point") &&
               GEP->getNumIndices() >= 2;
      });

  if (FieldGEP) {
    // Should point to int (the field type)
    EXPECT_TRUE(validatePointerType(FieldGEP, "int", false));
  } else {
    FAIL() << "Expect to find GEP for field access";
  }
}

TEST_F(GetDITypeFromValueTest, OpaqueFunctionReturns_SimpleReturn) {
  auto *M = loadAndValidateModule("test_opaque_function_returns_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find call to create_node function
  const auto *CreateNodeCall = findInstructionWithPredicate<llvm::CallInst>(
      M, [](const llvm::CallInst *CI) {
        auto *Callee = CI->getCalledFunction();
        return Callee && Callee->getName().contains("create_node");
      });
  ASSERT_NE(CreateNodeCall, nullptr);

  EXPECT_TRUE(validatePointerType(CreateNodeCall, "Node"));
}

TEST_F(GetDITypeFromValueTest, OpaqueFunctionReturns_ChainedCalls) {
  auto *M = loadAndValidateModule("test_opaque_function_returns_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find call to find_last function
  const auto *FindLastCall = findInstructionWithPredicate<llvm::CallInst>(
      M, [](const llvm::CallInst *CI) {
        auto *Callee = CI->getCalledFunction();
        return Callee && Callee->getName().contains("find_last");
      });
  ASSERT_NE(FindLastCall, nullptr);

  EXPECT_TRUE(validatePointerType(FindLastCall, "Node"));
}

TEST_F(GetDITypeFromValueTest, OpaqueFunctionReturns_ComplexSignature) {
  auto *M = loadAndValidateModule("test_opaque_function_returns_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find call to get_data_ptr function
  const auto *GetDataCall = findInstructionWithPredicate<llvm::CallInst>(
      M, [](const llvm::CallInst *CI) {
        auto *Callee = CI->getCalledFunction();
        return Callee && Callee->getName().contains("get_data_ptr");
      });
  ASSERT_NE(GetDataCall, nullptr);

  EXPECT_TRUE(validatePointerType(GetDataCall, "int"));
}

TEST_F(GetDITypeFromValueTest, OpaqueFunctionReturns_PointerToPointer) {
  auto *M = loadAndValidateModule("test_opaque_function_returns_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find call to get_generic_ptr function
  const auto *GetGenericCall = findInstructionWithPredicate<llvm::CallInst>(
      M, [](const llvm::CallInst *CI) {
        auto *Callee = CI->getCalledFunction();
        return Callee && Callee->getName().contains("get_generic_ptr");
      });
  ASSERT_TRUE(GetGenericCall);

  // This should be void** (pointer to pointer to void)
  auto *DIType = psr::getVarTypeFromIR(GetGenericCall);
  ASSERT_NE(DIType, nullptr);

  auto *OuterPtr = llvm::dyn_cast<llvm::DIDerivedType>(DIType);
  ASSERT_NE(OuterPtr, nullptr);
  EXPECT_EQ(OuterPtr->getTag(), llvm::dwarf::DW_TAG_pointer_type);

  // The base type should also be a pointer
  auto *InnerPtr = llvm::dyn_cast<llvm::DIDerivedType>(OuterPtr->getBaseType());
  if (InnerPtr) {
    EXPECT_EQ(InnerPtr->getTag(), llvm::dwarf::DW_TAG_pointer_type);
  } else {
    FAIL() << "Is not pointer type: " << psr::llvmTypeToString(InnerPtr);
  }
}

TEST_F(GetDITypeFromValueTest, OpaquePhiNodes_ConditionalPointers) {
  auto *M = loadAndValidateModule("test_opaque_phi_nodes_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find phi node merging different object pointers
  const auto *ConditionalPhi = findInstructionWithPredicate<llvm::PHINode>(
      M,
      [](const llvm::PHINode *PN) {
        return PN->getType()->isPointerTy() && PN->getNumIncomingValues() >= 2;
      },
      "_Z18test_phi_scenariosbi");
  ASSERT_NE(ConditionalPhi, nullptr);

  // This is challenging - phi merging TypeA* and TypeB*
  // The function should handle this gracefully
  auto *DIType = psr::getVarTypeFromIR(ConditionalPhi);
  ASSERT_NE(DIType, nullptr) << "Phi node should have some type information";

  // Could be void* or one of the input types
  EXPECT_TRUE(validateTypeIsOneOf(ConditionalPhi, {"TypeA", "TypeB", "void"}));
}

TEST_F(GetDITypeFromValueTest, OpaquePhiNodes_ArrayAccess) {
  auto *M = loadAndValidateModule("test_opaque_phi_nodes_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find phi node from array access in conditional
  const auto *ArrayPhi = findInstructionWithPredicate<llvm::PHINode>(
      M,
      [](const llvm::PHINode *PN) {
        // Look for phi that merges array element pointers
        return PN->getType()->isPointerTy();
      },
      "_Z18test_phi_scenariosbi");

  if (ArrayPhi) {
    EXPECT_TRUE(validatePointerType(ArrayPhi, "TypeA", false));
  } else {
    FAIL() << "Could not find array-phi";
  }
}

TEST_F(GetDITypeFromValueTest, OpaquePhiNodes_LoopInduction) {
  auto *M = loadAndValidateModule("test_opaque_phi_nodes_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find phi node from loop (merging different GEPs)
  const llvm::PHINode *LoopPhi = nullptr;

  // Look in loop blocks for phi nodes
  for (auto &F : *M) {
    for (auto &BB : F) {
      // Simple heuristic: blocks with phi nodes and back edges are likely loop
      // headers
      if (!BB.empty() && llvm::isa<llvm::PHINode>(BB.front())) {
        for (auto &I : BB) {
          if (auto *PN = llvm::dyn_cast<llvm::PHINode>(&I)) {
            if (PN->getType()->isPointerTy()) {
              LoopPhi = PN;
              break;
            }
          }
        }
      }
      if (LoopPhi) {
        break;
      }
    }
    if (LoopPhi) {
      break;
    }
  }

  if (LoopPhi) {
    EXPECT_TRUE(validatePointerType(LoopPhi, "TypeA", false));
  } else {
    FAIL() << "Did not find loop-phi";
  }
}

TEST_F(GetDITypeFromValueTest, OpaqueComplexExpressions_MultiLevelGEP) {
  auto *M = loadAndValidateModule("test_opaque_complex_expressions_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find complex GEP: &containers[1].matrices[2]->data[3][4]
  const auto *ComplexGEP =
      findInstructionWithPredicate<llvm::GetElementPtrInst>(
          M,
          [](const llvm::GetElementPtrInst *GEP) {
            return GEP->getNumIndices() >= 2 &&
                   GEP->getSourceElementType()->isArrayTy() &&
                   llvm::isa<llvm::GetElementPtrInst>(
                       GEP->getPointerOperand()); // Multi-level access
          },
          "_Z24test_complex_expressionsv");
  ASSERT_NE(ComplexGEP, nullptr);

  EXPECT_TRUE(validatePointerType(ComplexGEP, "Matrix", false));
}

TEST_F(GetDITypeFromValueTest, OpaqueComplexExpressions_IndirectAccess) {
  auto *M = loadAndValidateModule("test_opaque_complex_expressions_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find load from pointer to pointer: **matrix_ptr
  const auto *IndirectLoad = findInstructionWithPredicate<llvm::LoadInst>(
      M, [](const llvm::LoadInst *LI) {
        // Look for loads that load from a loaded pointer
        return llvm::isa<llvm::LoadInst>(LI->getPointerOperand());
      });

  if (IndirectLoad) {
    EXPECT_TRUE(validatePointerType(IndirectLoad, "Matrix"));
  } else {
    FAIL() << "Did not find load";
  }
}

TEST_F(GetDITypeFromValueTest, OpaqueComplexExpressions_DynamicIndexing) {
  auto *M = loadAndValidateModule("test_opaque_complex_expressions_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Find GEP with dynamic indices
  const auto *DynamicGEP =
      findInstructionWithPredicate<llvm::GetElementPtrInst>(
          M, [](const llvm::GetElementPtrInst *GEP) {
            // Look for GEPs where indices are not constants
            return llvm::any_of(GEP->indices(), [](const llvm::Use &Idx) {
              return !llvm::isa<llvm::ConstantInt>(Idx.get());
            });
          });

  if (DynamicGEP) {
    // Even with dynamic indexing, type should be preserved
    EXPECT_TRUE(validatePointerType(DynamicGEP, "Container", false));
  } else {
    FAIL() << "Did not find dynamic GEP";
  }
}

// Additional validation functions for complex scenarios
TEST_F(GetDITypeFromValueTest, OpaquePointers_EdgeCaseValidation) {
  auto *M = loadAndValidateModule("test_opaque_complex_expressions_cpp_dbg.ll");
  if (!M) {
    return;
  }

  // Test helper to validate our validation functions work correctly
  const auto *SomeGEP = findFirstInstructionOfType<llvm::GetElementPtrInst>(M);
  ASSERT_TRUE(SomeGEP);

  // Test that validation fails appropriately for wrong types
  EXPECT_FALSE(validatePointerType(SomeGEP, "NonExistentType"));
  EXPECT_FALSE(validateBasicType(
      SomeGEP, "float")); // GEP should be pointer, not basic type
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>

using namespace psr;

static llvm::DbgVariableIntrinsic *getDbgVarIntrinsic(const llvm::Value *V) {

  if (auto *VAM =
          llvm::ValueAsMetadata::getIfExists(const_cast<llvm::Value *>(V))) {
    if (auto *MDV = llvm::MetadataAsValue::getIfExists(V->getContext(), VAM)) {
      for (auto *U : MDV->users()) {
        if (auto *DBGIntr = llvm::dyn_cast<llvm::DbgVariableIntrinsic>(U)) {
          return DBGIntr;
        }
      }
    }
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    /* If mem2reg is not activated, formal parameters will be stored in
     * registers at the beginning of function call. Debug info will be linked to
     * those alloca's instead of the arguments itself. */
    for (const auto *User : Arg->users()) {
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
        if (Store->getValueOperand() == Arg &&
            llvm::isa<llvm::AllocaInst>(Store->getPointerOperand())) {
          return getDbgVarIntrinsic(Store->getPointerOperand());
        }
      }
    }
  }
  return nullptr;
}

llvm::DILocalVariable *psr::getDILocalVariable(const llvm::Value *V) {
  if (auto *DbgIntr = getDbgVarIntrinsic(V)) {
    if (auto *DDI = llvm::dyn_cast<llvm::DbgDeclareInst>(DbgIntr)) {
      return DDI->getVariable();
    }
    if (auto *DVI = llvm::dyn_cast<llvm::DbgValueInst>(DbgIntr)) {
      return DVI->getVariable();
    }
  }
  return nullptr;
}

static llvm::DIGlobalVariable *getDIGlobalVariable(const llvm::Value *V) {
  if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    if (auto *MN = GV->getMetadata(llvm::LLVMContext::MD_dbg)) {
      if (auto *DIGVExp =
              llvm::dyn_cast<llvm::DIGlobalVariableExpression>(MN)) {
        return DIGVExp->getVariable();
      }
    }
  }
  return nullptr;
}

static llvm::DISubprogram *getDISubprogram(const llvm::Value *V) {
  if (const auto *F = llvm::dyn_cast<llvm::Function>(V)) {
    return F->getSubprogram();
  }
  return nullptr;
}

llvm::DILocation *psr::getDILocation(const llvm::Value *V) {
  // Arguments and Instruction such as AllocaInst
  if (auto *DbgIntr = getDbgVarIntrinsic(V)) {
    if (auto *MN = DbgIntr->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return llvm::dyn_cast<llvm::DILocation>(MN);
    }
  } else if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (auto *MN = I->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return llvm::dyn_cast<llvm::DILocation>(MN);
    }
  }
  return nullptr;
}

std::string psr::getVarNameFromIR(const llvm::Value *V) {
  if (auto *LocVar = getDILocalVariable(V)) {
    return LocVar->getName().str();
  }
  if (auto *GlobVar = getDIGlobalVariable(V)) {
    return GlobVar->getName().str();
  }
  return "";
}

static llvm::DIType *getVarTypeFromIRImpl(const llvm::Value *V) {
  if (auto *LocVar = getDILocalVariable(V)) {
    return LocVar->getType();
  }
  if (auto *GlobVar = getDIGlobalVariable(V)) {
    return GlobVar->getType();
  }
  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(V)) {
    if (const auto *Callee = llvm::dyn_cast<llvm::Function>(
            Call->getCalledOperand()->stripPointerCastsAndAliases())) {
      if (auto *DICallee = Callee->getSubprogram()) {
        auto Types = DICallee->getType()->getTypeArray();
        if (Types.size()) {
          return Types[0];
        }
      }
    }
  }
  return nullptr;
}

// static const llvm::GEPOperator *getStructGep(const llvm::Value *V) {
//   if (const auto *Gep = llvm::dyn_cast<llvm::GEPOperator>(V)) {
//     if (Gep->getNumIndices() != 2) {
//       return nullptr;
//     }
//     const auto *FirstIdx =
//         llvm::dyn_cast<llvm::ConstantInt>(Gep->indices().begin()->get());
//     if (!FirstIdx || FirstIdx->getZExtValue() != 0) {
//       return nullptr;
//     }

//     const auto *SecondIdx = llvm::dyn_cast<llvm::ConstantInt>(
//         std::next(Gep->indices().begin())->get());
//     if (!SecondIdx) {
//       return nullptr;
//     }
//     return Gep;
//   }

//   return nullptr;
// }

// static std::pair<const llvm::Value *, size_t>
// getOffsetAndBase(const llvm::Value *V) {
//   const auto *Base = V->stripPointerCastsAndAliases();
//   uint64_t Offset = 0;
//   if (const auto *Gep = getStructGep(Base)) {
//     // Look for gep ptr, 0, N; where N is a constant

//     const auto *SecondIdx =
//         llvm::cast<llvm::ConstantInt>(std::next(Gep->indices().begin())->get());

//     Offset = SecondIdx->getZExtValue();
//     Base = Gep->getPointerOperand();
//   }
//   return {Base, Offset};
// }

static llvm::DIType *stripTAGMember(llvm::DIType *Ty) {
  if (auto *DerivedTy = llvm::dyn_cast_if_present<llvm::DIDerivedType>(Ty)) {
    if (DerivedTy->getTag() == llvm::dwarf::DW_TAG_member) {
      return DerivedTy->getBaseType();
    }
  }
  return Ty;
}

static llvm::DIType *stripTAGTypedef(llvm::DIType *Ty) {
  while (auto *DerivedTy = llvm::dyn_cast_if_present<llvm::DIDerivedType>(Ty)) {
    if (DerivedTy->getTag() == llvm::dwarf::DW_TAG_typedef ||
        DerivedTy->getTag() == llvm::dwarf::DW_TAG_const_type) {
      Ty = DerivedTy->getBaseType();
      continue;
    }
    break;
  }
  return Ty;
}

// static llvm::DIType *getStructElementType(llvm::DIType *BaseTy, size_t
// Offset) {
//   const auto *DerivedTy =
//       llvm::dyn_cast_if_present<llvm::DIDerivedType>(BaseTy);
//   auto *StructTy = DerivedTy ? DerivedTy->getBaseType() : BaseTy;

//   if (Offset == 0 && DerivedTy) {
//     return StructTy;
//   }

//   if (const auto *CompositeTy =
//           llvm::dyn_cast_if_present<llvm::DICompositeType>(StructTy)) {

//     if (CompositeTy->getTag() == llvm::dwarf::DW_TAG_array_type) {
//       if (auto *ElemTy = llvm::dyn_cast_if_present<llvm::DIType>(
//               CompositeTy->getBaseType())) {
//         return ElemTy;
//       }
//     }

//     auto Elems = CompositeTy->getElements();
//     if (!Elems || Offset >= Elems.size()) {
//       // llvm::errs() << "Requested offset " << Offset
//       //              << " exceeds number of elements (" << Elems.size()
//       //              << ") of type " << *CompositeTy << '\n';
//       return nullptr;
//     }

//     // TODO: DW_TAG_member

//     if (auto *ElemTy =
//     llvm::dyn_cast_if_present<llvm::DIType>(Elems[Offset])) {
//       return stripTAGMember(ElemTy);
//     }
//   }
//   // llvm::errs() << "No struct element type for " << *BaseTy << " at index "
//   //              << Offset << '\n';
//   return nullptr;
// }

// static llvm::DIType *getVarTypeFromIRRec(const llvm::Value *V, size_t Depth)
// {
//   static constexpr size_t DepthLimit = 10;

//   V = V->stripPointerCastsAndAliases();

//   if (auto *VarTy = getVarTypeFromIRImpl(V)) {
//     return VarTy;
//   }

//   const auto InternalGetOffsetAndBase =
//       [](const llvm::Value *V) -> std::pair<const llvm::Value *, size_t> {
//     if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
//       return getOffsetAndBase(Load->getPointerOperand());
//     }
//     if (const auto *Gep = llvm::dyn_cast<llvm::GEPOperator>(V)) {
//       return getOffsetAndBase(Gep->getPointerOperand());
//     }
//     return {};
//   };

//   auto [Base, Offset] = InternalGetOffsetAndBase(V);
//   if (!Base) {
//     // llvm::errs() << "No Base for val " << llvmIRToString(V) << '\n';
//     return nullptr;
//   }

//   // TODO: Get rid of the recursion
//   if (Depth >= DepthLimit) {
//     // llvm::errs() << "Reached depth-limit for val " << llvmIRToString(V) <<
//     // '\n';
//     return nullptr;
//   }
//   auto *BaseTy = getVarTypeFromIRRec(Base, Depth + 1);
//   if (!BaseTy) {
//     // llvm::errs() << "No BaseTy for val " << llvmIRToString(Base) << '\n';
//     return nullptr;
//   }
//   return getStructElementType(BaseTy, Offset);
// }

static llvm::DIType *
getDITypeFromValue(const llvm::Value *V,
                   llvm::SmallDenseSet<const llvm::Value *> &Visited) {
  // llvm::errs() << "[getDITypeFromValue]: V: " << llvmIRToString(V) << '\n';
  if (!V || Visited.count(V)) {
    // llvm::errs() << "> already visited or null\n";
    return nullptr;
  }

  Visited.insert(V);

  auto *Ret = [&]() -> llvm::DIType * {
    // First, check for direct debug intrinsic references
    if (auto *VarTy = getVarTypeFromIRImpl(V)) {
      // llvm::errs() << "  > return varty\n";
      return VarTy;
    }
    // else {
    //   llvm::errs() << "  > no varty\n";
    // }

    if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {

      // Handle LoadInst - trace back to what we're loading from
      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Inst)) {
        const llvm::Value *LoadedFrom = Load->getPointerOperand();

        // Get the type of what we're loading from
        if (auto *LoadedFromType = getDITypeFromValue(LoadedFrom, Visited)) {

          // If we're loading from a pointer type, dereference it
          if (auto *DerivedType =
                  llvm::dyn_cast<llvm::DIDerivedType>(LoadedFromType)) {
            if (DerivedType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
              if (!llvm::isa<llvm::GetElementPtrInst, llvm::AllocaInst,
                             llvm::GlobalVariable>(Load->getPointerOperand())) {
                // XXX: In case of a GEP, we did the dereferencing already (see
                // below!)
                // For Allocas and Globals, we return the dbg-intrinsic type,
                // which also does not contain any additional pointer

                return DerivedType->getBaseType();
              }
            }
          }

          // Otherwise return the type as-is
          return LoadedFromType;
        }

        // llvm::errs() << "> fallthrough at load\n";
        return nullptr;
      }

      // Handle GetElementPtrInst - calculate the resulting type
      if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Inst)) {
        // Get the base pointer's type
        if (auto *BaseType =
                getDITypeFromValue(GEP->getPointerOperand(), Visited)) {
          // llvm::errs() << "  > found GEP base type\n";
          llvm::DIType *CurrentType = BaseType;

          // If base type is a pointer, dereference it first, otherwise, we
          // cannot access the struct/array members
          if (auto *DerivedType =
                  llvm::dyn_cast<llvm::DIDerivedType>(CurrentType)) {
            if (DerivedType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
              CurrentType = DerivedType->getBaseType();
            }
          }

          // Regular struct/array GEP handling
          // First index does not change the type
          for (const auto &Idx : llvm::drop_begin(GEP->indices())) {
            // llvm::errs() << "   > Handle index " << llvmIRToString(Idx) <<
            // '\n';
            if (auto *CompositeType =
                    llvm::dyn_cast_if_present<llvm::DICompositeType>(
                        CurrentType)) {
              // llvm::errs() << "   > Is composite\n";
              if (CompositeType->getTag() ==
                      llvm::dwarf::DW_TAG_structure_type ||
                  CompositeType->getTag() == llvm::dwarf::DW_TAG_class_type) {
                // llvm::errs() << "   > Is struct/class\n";
                if (const auto *ConstInt =
                        llvm::dyn_cast<llvm::ConstantInt>(Idx.get())) {
                  uint64_t Index = ConstInt->getZExtValue();
                  auto Elements = CompositeType->getElements();
                  if (Index < Elements.size()) {
                    CurrentType = stripTAGTypedef(stripTAGMember(
                        llvm::dyn_cast<llvm::DIType>(Elements[Index])));
                    // llvm::errs() << "   > Adjust type to elemty "
                    //              << llvmTypeToString(CurrentType) << '\n';
                  }
                }
              } else if (CompositeType->getTag() ==
                         llvm::dwarf::DW_TAG_array_type) {

                // llvm::errs() << "   > Is array\n";
                CurrentType = stripTAGTypedef(
                    stripTAGMember(CompositeType->getBaseType()));
                // llvm::errs() << "   > Adjust type to elemty "
                //              << llvmTypeToString(CurrentType) << '\n';
              }
            }
          }

          // TODO: Get pointer back! (then need to sync with load handling
          // above!)
          return CurrentType;
        }
      }
      // Handle CallInst - get return type from function debug info
      else if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst)) {
        if (const auto *CalledFunc = llvm::dyn_cast<llvm::Function>(
                Call->getCalledOperand()->stripPointerCastsAndAliases())) {
          if (auto *Subprogram = CalledFunc->getSubprogram()) {
            if (auto *FnType = llvm::dyn_cast<llvm::DISubroutineType>(
                    Subprogram->getType())) {
              auto TypeArray = FnType->getTypeArray();
              if (TypeArray.begin() != TypeArray.end()) {
                // Return type is at index 0
                return llvm::dyn_cast_or_null<llvm::DIType>(TypeArray[0]);
              }
            }
          }
        } else {
          const llvm::Value *FnPtr = Call->getCalledOperand();

          // Try to get debug type information for the function pointer
          if (auto *FnPtrType = getDITypeFromValue(FnPtr, Visited)) {
            llvm::DIType *CurrentType = FnPtrType;

            // If it's a pointer type, dereference to get the function type
            if (auto *DerivedType =
                    llvm::dyn_cast<llvm::DIDerivedType>(CurrentType)) {
              if (DerivedType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
                CurrentType = DerivedType->getBaseType();
              }
            }

            // Now we should have a function type
            if (auto *SubroutineType =
                    llvm::dyn_cast<llvm::DISubroutineType>(CurrentType)) {
              auto TypeArray = SubroutineType->getTypeArray();
              if (TypeArray.size() > 0) {
                return llvm::dyn_cast_or_null<llvm::DIType>(TypeArray[0]);
              }
            }
          }
        }
      }

      // Handle PHI nodes - check all incoming values
      else if (const auto *Phi = llvm::dyn_cast<llvm::PHINode>(Inst)) {
        for (unsigned i = 0; i < Phi->getNumIncomingValues(); ++i) {
          if (auto *Type =
                  getDITypeFromValue(Phi->getIncomingValue(i), Visited)) {
            return Type; // Return the first type we find
          }
        }
      }

      // Handle Select instructions
      else if (const auto *Select = llvm::dyn_cast<llvm::SelectInst>(Inst)) {
        // Check true value first, then false value
        if (auto *Type = getDITypeFromValue(Select->getTrueValue(), Visited)) {
          return Type;
        }
        return getDITypeFromValue(Select->getFalseValue(), Visited);
      }

      // Handle Cast instructions - trace through to the source
      else if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Inst)) {
        // TODO: This can be tricky. We are interested in the type, so a cast
        // could give us all we need; only in case of pointer-casts we need to
        // recurse
        return getDITypeFromValue(Cast->getOperand(0), Visited);
      }
    }

    // Handle Function Arguments
    else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
      const llvm::Function *F = Arg->getParent();
      if (auto *Subprogram = F->getSubprogram()) {
        auto ArgNo = Arg->getArgNo();
        if (auto *FnType =
                llvm::dyn_cast<llvm::DISubroutineType>(Subprogram->getType())) {
          auto TypeArray = FnType->getTypeArray();
          // Note: 0 is return type!
          if (ArgNo + 1 < TypeArray.size()) {
            return llvm::dyn_cast_or_null<llvm::DIType>(TypeArray[ArgNo + 1]);
          }
        }
      }
    }

    // Handle Global Variables
    else if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
      if (const auto *GlobalVar = getDIGlobalVariable(GV)) {
        return GlobalVar->getType();
      }
      // llvm::SmallVector<llvm::DIGlobalVariableExpression *, 1> GVEs;
      // GV->getDebugInfo(GVEs);
      // if (!GVEs.empty()) {
      //   if (auto *GlobalVar = GVEs[0]->getVariable()) {
      //     return GlobalVar->getType();
      //   }
      // }
    }

    // llvm::errs() << "> fallthrough\n";
    return nullptr;
  }();

  return stripTAGTypedef(Ret);
}

llvm::DIType *psr::getVarTypeFromIR(const llvm::Value *V) {
  if (!V) {
    return nullptr;
  }
  // llvm::errs() << '\n';
  // return getVarTypeFromIRRec(V, 0);
  llvm::SmallDenseSet<const llvm::Value *> Visited;
  return getDITypeFromValue(V, Visited);
}

std::string psr::getFunctionNameFromIR(const llvm::Value *V) {
  // We can return unmangled function names w/o checking debug info
  if (const auto *F = llvm::dyn_cast<llvm::Function>(V)) {
    return F->getName().str();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent()->getName().str();
  }
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getName().str();
  }
  return "";
}

std::string psr::getFilePathFromIR(const llvm::Value *V) {
  if (const auto *DIF = getDIFileFromIR(V)) {
    return getFilePathFromIR(DIF);
  }
  /* As a fallback solution, we will return 'source_filename' info from
   * module. However, it is not guaranteed to contain the absoult path, and it
   * will return 'llvm-link' for linked modules. */
  if (const auto *F = llvm::dyn_cast<llvm::Function>(V)) {
    return F->getParent()->getSourceFileName();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent()->getParent()->getSourceFileName();
  }
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getParent()->getSourceFileName();
  }

  return {};
}

std::string psr::getFilePathFromIR(const llvm::DIFile *DIF) {
  auto FileName = DIF->getFilename();
  auto DirName = DIF->getDirectory();

  if (FileName.empty()) {
    return {};
  }

  // try to concatenate file path and dir to get absolute path
  if (!DirName.empty() &&
      !llvm::sys::path::has_root_directory(DIF->getFilename())) {
    llvm::SmallString<256> Buf;
    llvm::sys::path::append(Buf, DirName, FileName);

    return Buf.str().str();
  }

  return FileName.str();
}

const llvm::DIFile *psr::getDIFileFromIR(const llvm::Value *V) {
  if (const auto *GO = llvm::dyn_cast<llvm::GlobalObject>(V)) {
    if (auto *MN = GO->getMetadata(llvm::LLVMContext::MD_dbg)) {
      if (auto *Subpr = llvm::dyn_cast<llvm::DISubprogram>(MN)) {
        return Subpr->getFile();
      }
      if (auto *GVExpr = llvm::dyn_cast<llvm::DIGlobalVariableExpression>(MN)) {
        return GVExpr->getVariable()->getFile();
      }
    }
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    if (auto *LocVar = getDILocalVariable(Arg)) {
      return LocVar->getFile();
    }
  } else if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (I->isUsedByMetadata()) {
      if (auto *LocVar = getDILocalVariable(I)) {
        return LocVar->getFile();
      }
    } else if (I->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return I->getDebugLoc()->getFile();
    }
    if (const auto *DIFun = I->getFunction()->getSubprogram()) {
      return DIFun->getFile();
    }
  }
  return nullptr;
}

std::string psr::getDirectoryFromIR(const llvm::Value *V) {
  // Argument and Instruction
  if (auto *DILoc = getDILocation(V)) {
    return DILoc->getDirectory().str();
  }
  if (auto *DISubpr = getDISubprogram(V)) { // Function
    return DISubpr->getDirectory().str();
  }
  if (auto *DIGV = getDIGlobalVariable(V)) { // Globals
    return DIGV->getDirectory().str();
  }
  return "";
}

unsigned int psr::getLineFromIR(const llvm::Value *V) {
  // Argument and Instruction
  if (auto *DILoc = getDILocation(V)) {
    return DILoc->getLine();
  }
  if (auto *DISubpr = getDISubprogram(V)) { // Function
    return DISubpr->getLine();
  }
  if (auto *DIGV = getDIGlobalVariable(V)) { // Globals
    return DIGV->getLine();
  }
  return 0;
}

unsigned int psr::getColumnFromIR(const llvm::Value *V) {
  // Globals and Function have no column info
  if (auto *DILoc = getDILocation(V)) {
    return DILoc->getColumn();
  }
  return 0;
}

std::pair<unsigned, unsigned> psr::getLineAndColFromIR(const llvm::Value *V) {
  // Argument and Instruction
  if (auto *DILoc = getDILocation(V)) {
    return {DILoc->getLine(), DILoc->getColumn()};
  }
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (const auto *DIFun = I->getFunction()->getSubprogram()) {
      return {DIFun->getLine(), 0};
    }
  }
  if (auto *DISubpr = getDISubprogram(V)) { // Function
    return {DISubpr->getLine(), 0};
  }
  if (auto *DIGV = getDIGlobalVariable(V)) { // Globals
    return {DIGV->getLine(), 0};
  }
  return {0, 0};
}

std::string psr::getSrcCodeFromIR(const llvm::Value *V, bool Trim) {
  if (auto Loc = getDebugLocation(V)) {
    return getSrcCodeFromIR(*Loc, Trim);
  }
  return {};
}

std::string psr::getSrcCodeFromIR(DebugLocation Loc, bool Trim) {
  if (Loc.Line == 0) {
    return {};
  }
  auto Path = getFilePathFromIR(Loc.File);

  if (llvm::sys::fs::exists(Path) && !llvm::sys::fs::is_directory(Path)) {
    std::ifstream Ifs(Path, std::ios::binary);
    if (Ifs.is_open()) {
      Ifs.seekg(std::ios::beg);
      std::string SrcLine;
      for (unsigned int I = 0; I < Loc.Line - 1; ++I) {
        Ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      std::getline(Ifs, SrcLine);
      return Trim ? llvm::StringRef(SrcLine).trim().str() : SrcLine;
    }
  }
  return {};
}

std::string psr::getModuleIDFromIR(const llvm::Value *V) {
  if (const auto *GO = llvm::dyn_cast<llvm::GlobalObject>(V)) {
    return GO->getParent()->getModuleIdentifier();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent()->getParent()->getModuleIdentifier();
  }
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getParent()->getModuleIdentifier();
  }
  return "";
}

bool SourceCodeInfo::empty() const noexcept { return SourceCodeLine.empty(); }

bool SourceCodeInfo::operator==(const SourceCodeInfo &Other) const noexcept {
  // don't compare the SourceCodeFunctionName. It is directly derivable from
  // line, column and filename
  return Line == Other.Line && Column == Other.Column &&
         SourceCodeLine == Other.SourceCodeLine &&
         SourceCodeFilename == Other.SourceCodeFilename;
}

bool SourceCodeInfo::equivalentWith(const SourceCodeInfo &Other) const {
  // Here, we need to compare the SourceCodeFunctionName, because we don't
  // compare the complete SourceCodeFilename
  if (Line != Other.Line || Column != Other.Column ||
      SourceCodeLine != Other.SourceCodeLine ||
      SourceCodeFunctionName != Other.SourceCodeFunctionName) {
    return false;
  }

  auto Pos = SourceCodeFilename.find_last_of(
      std::filesystem::path::preferred_separator);
  if (Pos == std::string::npos) {
    Pos = 0;
  }

  return llvm::StringRef(Other.SourceCodeFilename)
      .endswith(llvm::StringRef(SourceCodeFilename)
                    .slice(Pos + 1, llvm::StringRef::npos));
}

void psr::from_json(const nlohmann::json &J, SourceCodeInfo &Info) {
  J.at("sourceCodeLine").get_to(Info.SourceCodeLine);
  J.at("sourceCodeFileName").get_to(Info.SourceCodeFilename);
  if (auto Fn = J.find("sourceCode"); Fn != J.end()) {
    Fn->get_to(Info.SourceCodeFunctionName);
  }
  J.at("line").get_to(Info.Line);
  J.at("column").get_to(Info.Column);
}
void psr::to_json(nlohmann::json &J, const SourceCodeInfo &Info) {
  J = nlohmann::json{
      {"sourceCodeLine", Info.SourceCodeLine},
      {"sourceCodeFileName", Info.SourceCodeFilename},
      {"sourceCodeFunctionName", Info.SourceCodeFunctionName},
      {"line", Info.Line},
      {"column", Info.Column},
  };
}

SourceCodeInfo psr::getSrcCodeInfoFromIR(const llvm::Value *V) {
  return SourceCodeInfo{
      getSrcCodeFromIR(V),
      getFilePathFromIR(V),
      llvm::demangle(getFunctionNameFromIR(V)),
      getLineFromIR(V),
      getColumnFromIR(V),
  };
}

std::optional<DebugLocation> psr::getDebugLocation(const llvm::Value *V) {
  // Argument and Instruction
  if (auto *DILoc = getDILocation(V)) {
    return DebugLocation{DILoc->getLine(), DILoc->getColumn(),
                         DILoc->getFile()};
  }
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (const auto *DIFun = I->getFunction()->getSubprogram()) {
      return DebugLocation{DIFun->getLine(), 0, DIFun->getFile()};
    }
  }
  if (auto *DISubpr = getDISubprogram(V)) { // Function
    return DebugLocation{DISubpr->getLine(), 0, DISubpr->getFile()};
  }
  if (auto *DIGV = getDIGlobalVariable(V)) { // Globals
    return DebugLocation{DIGV->getLine(), 0, DIGV->getFile()};
  }

  return std::nullopt;
}

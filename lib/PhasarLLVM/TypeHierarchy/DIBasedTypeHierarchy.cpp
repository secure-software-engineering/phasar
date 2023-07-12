/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/TypeHierarchy/VFTable.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

namespace psr {

DIBasedTypeHierarchy::DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  const auto *const Module = IRDB.getModule();
  llvm::DebugInfoFinder Finder;
  Finder.processModule(*Module);

  // find and save all base types first, so they are present in TypeToVertex
  size_t Counter = 0;
  for (const llvm::DIType *DIType : Finder.types()) {
    if (const auto *CompositeType =
            llvm::dyn_cast<llvm::DICompositeType>(DIType)) {
      NameToType.try_emplace(CompositeType->getName(), CompositeType);
      TypeToVertex.try_emplace(CompositeType, Counter++);
      VertexTypes.push_back(CompositeType);
      continue;
    }
  }

  // Initialize the transitive closure matrix with all as false
  llvm::BitVector InitVector(VertexTypes.size(), false);

  for (size_t I = 0; I < VertexTypes.size(); I++) {
    TransitiveClosure.push_back(InitVector);
  }

  std::set<std::string> TypeScopeNames;
  std::set<std::string> RecordedNames;
  // find and save all derived types
  for (const llvm::DIType *DIType : Finder.types()) {
    if (const auto *DerivedType = llvm::dyn_cast<llvm::DIDerivedType>(DIType)) {
      if (DerivedType->getTag() == llvm::dwarf::DW_TAG_inheritance) {
        assert(NameToType.count(DerivedType->getScope()->getName()));
        assert(
            TypeToVertex.find(NameToType[DerivedType->getScope()->getName()]) !=
            TypeToVertex.end());
        const size_t ActualDerivedType =
            TypeToVertex[NameToType[DerivedType->getScope()->getName()]];
        assert(TypeToVertex.find(DerivedType->getBaseType()) !=
               TypeToVertex.end());
        size_t BaseTypeVertex = TypeToVertex[DerivedType->getBaseType()];

        assert(TransitiveClosure.size() >= BaseTypeVertex);
        assert(TransitiveClosure.size() >= ActualDerivedType);
        TransitiveClosure[BaseTypeVertex][ActualDerivedType] = true;

        // if the scope isn't a nullpointer, place it's name into the set
        if (DerivedType->getScope()) {
          TypeScopeNames.insert(DerivedType->getScope()->getName().str());
        }

        continue;
      }
    }
  }

  // Add transitive edges
  bool Change = true;
  size_t TCSize = TransitiveClosure.size();
  size_t RowIndex = 0;
  size_t PreviousRow = 0;

  // (max): I know the code below is very ugly, but I wanted to avoid recursion.
  // If the algorithm goes over the entire matrix and couldn't update any rows
  // anymore, it stops. As soon as one position gets updated, it goes over the
  // matrix again
  while (Change) {
    Change = false;
    // go over all rows of the matrix
    for (size_t CurrentRow = 0; CurrentRow < TCSize; CurrentRow++) {
      // compare current row with all other rows and check if an edge can be
      // added
      for (size_t CompareRow = 0; CompareRow < TCSize; CompareRow++) {
        // row doesn't need to compare itself with itself
        if (CurrentRow == CompareRow) {
          continue;
        }

        // if the current row is not a parent type of the current compare row,
        // no edges should be added
        if (!TransitiveClosure[CurrentRow][CompareRow]) {
          continue;
        }

        // Compare both rows and add edges if needed
        for (size_t Column = 0; Column < TCSize; Column++) {
          if (TransitiveClosure[CompareRow][Column] &&
              !TransitiveClosure[CurrentRow][Column] && CurrentRow != Column) {
            TransitiveClosure[CurrentRow][Column] = true;
            Change = true;
          }
        }
      }
    }
  }

  // add edges onto vertices themselves
  for (size_t I = 0; I < TransitiveClosure.size(); I++) {
    TransitiveClosure[I][I] = true;
  }

  // add virtual functions table
  size_t VTableSize = 0;
  for (const auto &Subprogram : Finder.subprograms()) {
    if (Subprogram->getVirtualIndex() > VTableSize) {
      VTableSize = Subprogram->getVirtualIndex();
    }
  }
  // if the biggest virtual index is 2 for example, the vector needs to have a
  // size of 3 (Indices: 0, 1, 2)
  VTableSize++;

  std::vector<const llvm::Function *> Init = {};
  std::vector<std::vector<const llvm::Function *>> IndexToFunctions;
  for (size_t I = 0; I < VTableSize; I++) {
    IndexToFunctions.push_back(Init);
  }

  // get VTables
  for (auto *Subprogram : Finder.subprograms()) {
    if (RecordedNames.find(Subprogram->getScope()->getName().str()) !=
        RecordedNames.end()) {
      continue;
    }

    if (!Subprogram->getVirtuality()) {
      continue;
    }

    const auto *const FunctionToAdd =
        IRDB.getFunction(Subprogram->getLinkageName());

    if (!FunctionToAdd) {
      continue;
    }

    const size_t VirtualIndex = Subprogram->getVirtualIndex();
    std::vector<const llvm::Function *> IndexFunctions;

    size_t TypeIndex = 0;
    for (const auto &Curr : TypeScopeNames) {
      // check if Subprogram Scope isn't a nullpointer
      if (const auto &SubCurr = Subprogram->getScope()) {
        if (Curr == SubCurr->getName().str()) {
          RecordedNames.insert(SubCurr->getName().str());
          break;
        }
      }
      TypeIndex++;
    }

    if (TypeIndex >= IndexToFunctions.size()) {
      continue;
    }

    if (IndexToFunctions[TypeIndex].size() <= VirtualIndex) {
      IndexToFunctions[TypeIndex].resize(VirtualIndex + 1);
    }
    IndexToFunctions[TypeIndex][VirtualIndex] = FunctionToAdd;
  }

  for (auto &ToAdd : IndexToFunctions) {
    //
    VTables.emplace_back(std::move(ToAdd));
  }
}

[[nodiscard]] bool DIBasedTypeHierarchy::isSubType(ClassType Type,
                                                   ClassType SubType) {
  const auto IndexOfTypeFind = TypeToVertex.find(Type);
  const auto IndexOfSubTypeFind = TypeToVertex.find(SubType);

  assert(IndexOfTypeFind != TypeToVertex.end());
  assert(IndexOfSubTypeFind != TypeToVertex.end());

  size_t IndexOfType = IndexOfTypeFind->getSecond();
  size_t IndexOfSubType = IndexOfSubTypeFind->getSecond();

  return TransitiveClosure[IndexOfType][IndexOfSubType];
}

[[nodiscard]] auto DIBasedTypeHierarchy::getSubTypes(ClassType Type)
    -> std::set<ClassType> {
  // find index of super type
  const auto IndexOfTypeFind = TypeToVertex.find(Type);

  assert(IndexOfTypeFind != TypeToVertex.end());

  size_t IndexOfType = IndexOfTypeFind->getSecond();

  // if the super type hasn't been found, return an empty set
  if (IndexOfType >= TypeToVertex.size()) {
    return {};
  }

  // return all sub types
  std::set<ClassType> SubTypes = {};

  for (auto Index : TransitiveClosure[IndexOfType].set_bits()) {
    SubTypes.insert(VertexTypes[Index]);
  }

  return SubTypes;
}

[[nodiscard]] bool DIBasedTypeHierarchy::isSuperType(ClassType Type,
                                                     ClassType SuperType) {
  return isSubType(SuperType, Type); // NOLINT
}

[[nodiscard]] auto DIBasedTypeHierarchy::getSuperTypes(ClassType Type)
    -> std::set<ClassType> {
  // TODO: implement (low priority)
  llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] bool DIBasedTypeHierarchy::hasVFTable(ClassType Type) const {
  const auto TypeIndex = TypeToVertex.find(Type);
  assert(TypeIndex->second < VTables.size());
  return !VTables[TypeIndex->second].empty();
}

[[nodiscard]] auto DIBasedTypeHierarchy::getVFTable(ClassType Type) const
    -> const VFTable<f_t> * {
  const auto TypeIndex = TypeToVertex.find(Type);
  assert(TypeIndex != TypeToVertex.end());
  return &VTables[TypeIndex->getSecond()];
}

void DIBasedTypeHierarchy::print(llvm::raw_ostream &OS) const {
  size_t VertexIndex = 0;
  size_t EdgeIndex = 0;

  OS << "Type Hierarchy\n";
  for (const auto &CurrentVertex : TransitiveClosure) {
    OS << VertexTypes[VertexIndex]->getName() << "\n";
    for (size_t I = 0; I < TransitiveClosure.size(); I++) {
      if (EdgeIndex != VertexIndex && CurrentVertex[I]) {
        OS << "--> " << VertexTypes[EdgeIndex]->getName() << "\n";
      }
      EdgeIndex++;
    }
    VertexIndex++;
    EdgeIndex = 0;
  }

  OS << "VFTables:\n";
  for (const auto &VTable : VTables) {
    if (VTable.empty()) {
      continue;
    }

    // get all function names for the llvm::interleaveComma function
    llvm::SmallVector<std::string, 6> Names;
    for (const auto &Function : VTable.getAllFunctions()) {
      if (Function) {
        Names.push_back(Function->getName().str());
      }
    }

    // prints out all function names, seperated by comma, without a trailing
    // comma
    llvm::interleaveComma(Names, OS);

    OS << "\n";
  };
  OS << "\n";
}

[[nodiscard]] nlohmann::json DIBasedTypeHierarchy::getAsJson() const {
  /// TODO: implement
  llvm::report_fatal_error("Not implemented");
}

void DIBasedTypeHierarchy::printAsDot(llvm::raw_ostream &OS) const {
  OS << "digraph TypeHierarchy{\n";

  if (TransitiveClosure.size() != VertexTypes.size()) {
    llvm::report_fatal_error("TransitiveClosure and VertexType size not equal");
    return;
  }

  // add all edges
  size_t CurrentRowIndex = 0;
  for (const auto &Row : TransitiveClosure) {
    for (const auto &Cell : Row.set_bits()) {
      if (Row[Cell]) {
        OS << "  " << VertexTypes[CurrentRowIndex]->getName() << " -> "
           << VertexTypes[Cell]->getName() << "\n";
      }
    }
    CurrentRowIndex++;
  }

  OS << "}\n";
}

} // namespace psr

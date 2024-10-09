#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IndexWrapper_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IndexWrapper_H

#include "phasar/Utils/Printer.h"
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"


#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Value.h>
#include <llvm-14/llvm/IR/Instructions.h>

namespace psr {
/**
 * @brief A wrapper around llvm::Value objects to include an indexing into the
 * object
 *
 */
template <typename T> struct IndexWrapper {

  T const *Value;
  std::vector<unsigned int> Indices;

public:
  IndexWrapper() : Value(nullptr), Indices({}){};
  IndexWrapper(const IndexWrapper & Other) : Value(Other.Value), Indices(Other.Indices){};
  IndexWrapper(IndexWrapper && Other)  noexcept : Value(Other.Value), Indices(std::move(Other.Indices)){};
  IndexWrapper &operator=(const IndexWrapper & Other) {
    if (this == Other) {
      return *this;
    }
    this->Value = Other.Value;
    this->Indices = Other.Indices;
    return *this;
  };
  IndexWrapper &operator=(IndexWrapper && Other)  noexcept {
    if (this == &Other) {
        return *this;  // Handle self-assignment
    }
    // Move members from Other to this
    Value = Other.Value;          // Move the pointer
    Indices = std::move(Other.Indices);  // Move the vector

    Other.Value = nullptr;  // Nullify the moved-from object's Value
    return *this;
  };
  IndexWrapper(T const *Value, std::vector<unsigned int> *Indices)
      : Value(Value), Indices(*Indices){};
  IndexWrapper(T const *Value, std::vector<unsigned int> Indices)
      : Value(Value), Indices(std::move(Indices)){};
  IndexWrapper(T const *Value) : Value(Value), Indices({}){};
  ~IndexWrapper() {};
  T const *getValue() const { return Value; };

  [[nodiscard]] std::vector<unsigned int> getIndices() const {
    return Indices;
  };

  std::set<IndexWrapper<T>>
  excluded(const llvm::ArrayRef<unsigned int> &ExcludedIndex) const {
    if (this->indexContains(ExcludedIndex)) {
      std::set<IndexWrapper<T>> Result = {};
      std::vector<unsigned int> PrefixVector = {};
      llvm::Type *Ty = this->Value->getType();
      for (unsigned int I : this->Indices) {
        if (auto *StructType = llvm::dyn_cast<llvm::StructType>(Ty)) {
          for (unsigned int J = 0; J < StructType->getNumElements(); ++I) {
            if (I != J) {
              PrefixVector.push_back(I);
              auto Index = std::vector<unsigned int>(PrefixVector);
              PrefixVector.pop_back();
              Result.insert(IndexWrapper<T>(this->Value, Index));
            }
          }
          Ty = StructType->getTypeAtIndex(I);
        }
        if (auto *ArrayType = llvm::dyn_cast<llvm::ArrayType>(Ty)) {
          for (unsigned int J = 0; J < ArrayType->getNumElements(); ++I) {
            if (I != J) {
              PrefixVector.push_back(I);
              auto Index = std::vector<unsigned int>(PrefixVector);
              PrefixVector.pop_back();
              Result.insert(IndexWrapper<T>(this->Value, Index));
            }
          }
          Ty = ArrayType->getElementType();
        }
      }
      return Result;
    }
    return {*this};
  };

  bool contains(const IndexWrapper<T> &Other) const {
    return this->Value == Other.Value && this->indexContains(Other);
  };

  [[nodiscard]] bool
  contains(const llvm::ExtractValueInst &ExtractValue) const {
    return this->Value == ExtractValue.getAggregateOperand() &&
           this->indexContains(ExtractValue.getIndices());
  };

  [[nodiscard]] bool
  contains(const llvm::GetElementPtrInst &GetElementPtr) const {
    if (this->Value == GetElementPtr.getPointerOperand() &&
        GetElementPtr.hasIndices() &&
        this->Indices.size() <= GetElementPtr.getNumIndices()) {
      for (size_t I = 0; I < this->Indices.size(); ++I) {
        llvm::Value *OtherI = GetElementPtr.getOperand(I + 1);
        if (auto *ConstantIndex = llvm::dyn_cast<llvm::ConstantInt>(OtherI)) {
          if ((this->Indices)[I] != ConstantIndex->getZExtValue()) {
            return false;
          }
        } else {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  bool indexContains(const IndexWrapper<T> &Other) const {
    return indexContains(Other.Indices);
  };

  [[nodiscard]] bool
  indexContains(const llvm::ArrayRef<unsigned int> &OtherIndices) const {
    if (this->Indices.size() <= OtherIndices.size()) {
      for (size_t I = 0; I < this->Indices.size(); ++I) {
        if ((this->Indices)[I] != OtherIndices[I]) {
          return false;
        }
      }
      return true;
    }
    return false;
  };

  bool operator<(const IndexWrapper<T> &Other) const {
    if (this->Value < Other.Value) {
      return true;
    }
    for (size_t I = 0; I < this->Indices.size(); ++I) {
      if ((this->Indices)[I] < (Other.Indices)[I]) {
        return true;
      }
      if ((this->Indices)[I] > (Other.Indices)[I]) {
        return false;
      }
    }
    return false;
  };

  bool operator==(const IndexWrapper<T> &Other) const {
    return (this->Value == Other.Value) && (this->Indices == Other.Indices);
  };
  std::string str() const {
    return "NOT YET IMPLEMENTED";
  }

};


} // namespace psr

// Specialize std::hash for IndexWrapper
namespace std {
    template <typename T>
    struct hash<psr::IndexWrapper<T>> {
        std::size_t operator()(const psr::IndexWrapper<T> &IW) const {
            std::size_t HashValue = std::hash<T const *>()(IW.Value);

            for (const auto &Index : IW.Indices) {
                HashValue ^= std::hash<unsigned int>()(Index) + 0x9e3779b9 + (HashValue << 6) + (HashValue >> 2);
            }

            return HashValue;
        }
    };
} // namespace std

#endif
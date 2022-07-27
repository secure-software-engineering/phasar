/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_NLOHMANNLOGGING_H
#define PHASAR_UTILS_NLOHMANNLOGGING_H

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

namespace llvm {
class raw_ostream;
}

namespace nlohmann::detail {
class llvm_output_stream_adapter : public output_adapter_protocol<char> {
public:
  explicit llvm_output_stream_adapter(llvm::raw_ostream &OS) noexcept
      : OS(OS) {}

  void write_character(char c) override { OS.write(c); }

  void write_characters(const char *Str, std::size_t Length) override {
    OS.write(Str, static_cast<std::streamsize>(Length));
  }

private:
  llvm::raw_ostream &OS;
};

template <typename StringType = std::basic_string<char>>
class llvm_output_adapter {
public:
  template <typename AllocatorType = std::allocator<char>>

  llvm_output_adapter(llvm::raw_ostream &OS)
      : OA(std::make_shared<llvm_output_stream_adapter>(OS)) {}

  operator output_adapter_t<char>() { return OA; }

private:
  output_adapter_t<char> OA = nullptr;
};
} // namespace nlohmann::detail

namespace psr {
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const nlohmann::json &J);
};

#endif /* PHASAR_UTILS_NLOHMANNLOGGING_H */

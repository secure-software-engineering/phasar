/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_SOL_H_
#define PHASAR_UTILS_SOL_H_

#include <stdexcept>
#include <string>

#include <dlfcn.h>

namespace psr {

class SOL {
private:
  char *error;
  void *so_handle;
  const std::string path;

public:
  SOL(const std::string &path);
  ~SOL();
  SOL(SOL &&) noexcept;
  SOL &operator=(SOL &&) noexcept;
  SOL(const SOL &) = delete;
  SOL &operator=(const SOL &) = delete;
  template <typename Signature> auto loadSymbol(const std::string &name) {
    auto sym = reinterpret_cast<Signature>(dlsym(so_handle, name.c_str()));
    if (!sym)
      throw std::invalid_argument("symbol not found");
    return sym;
  }
};

} // namespace psr

#endif

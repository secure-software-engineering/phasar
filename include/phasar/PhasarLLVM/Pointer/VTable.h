/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * VTable.h
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_POINTER_VTABLE_H_
#define PHASAR_PHASARLLVM_POINTER_VTABLE_H_

#include <iosfwd>
#include <string>
#include <vector>

#include <json.hpp>

namespace llvm {
class Module;
class Type;
} // namespace llvm

namespace psr {

using json = nlohmann::json;

/**
 * 	@brief Represents a virtual method table.
 *
 * 	Note that the position of a function identifier in the
 * 	virtual method table matters.
 */
class VTable {
private:
  std::vector<std::string> vtbl;

public:
  VTable() = default;
  virtual ~VTable() = default;

  /**
   * 	@brief Returns a function identifier by it's index in the VTable.
   * 	@param i Index of the entry.
   * 	@return Function identifier.
   */
  std::string getFunctionByIdx(unsigned i) const;

  /**
   * 	@brief Returns position index of the given function identifier
   * 	       in the VTable.
   * 	@param fname Function identifier.
   * 	@return Index of the functions entry.
   */
  int getEntryByFunctionName(std::string fname) const;

  /**
   * 	@brief Adds the given entry to the VTable.
   * 	@param entry Function identifier.
   *
   * 	A new entry will be added at the end of the VTable.
   */
  void addEntry(std::string entry);

  /**
   * 	@brief Checks if the VTable has no entries.
   * 	@return True, if VTable is empty, false otherwise.
   */
  bool empty() const;

  size_t size() const;

  std::vector<std::string>::iterator begin();
  std::vector<std::string>::const_iterator begin() const;
  std::vector<std::string>::iterator end();
  std::vector<std::string>::const_iterator end() const;
  /**
   * 	@brief Returns the VTable as a std::vector.
   * 	@return std::vector holding all information of the VTable.
   */
  std::vector<std::string> getVTable() const;

  /**
   * 	@brief VTable's print operator.
   * 	@param os A character output stream.
   * 	@param t A VTable to be printed.
   */
  friend std::ostream &operator<<(std::ostream &os, const VTable &t);

  json getAsJson();
};

} // namespace psr

#endif

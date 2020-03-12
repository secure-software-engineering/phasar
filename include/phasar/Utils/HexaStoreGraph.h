/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * HexaStoreGraph.hh
 *
 *  Created on: 06.02.2017
 *      Author: pdschbrt
 */

#ifndef PHASAR_UTILS_HEXASTOREGRAPH_H_
#define PHASAR_UTILS_HEXASTOREGRAPH_H_

#include <iostream> // cerr, to suppress once it is not used anymore
#include <ostream>
#include <set>

//#include "phasar/DB/DBConn.h"
#include "phasar/Utils/Table.h"

namespace psr {

template <typename S, typename E, typename D> class HexaStoreGraph {
private:
  Table<S, E, D> sed;
  Table<S, D, E> sde;
  Table<E, S, D> esd;
  Table<E, D, S> eds;
  Table<D, S, E> dse;
  Table<D, E, S> des;

public:
  HexaStoreGraph() = default;

  ~HexaStoreGraph() = default;

  void insertEdge(S src, E edge, D dest) {
    sed.insert(src, edge, dest);
    sde.insert(src, dest, edge);
    esd.insert(edge, src, dest);
    eds.insert(edge, dest, src);
    dse.insert(dest, src, edge);
    des.insert(dest, edge, src);
  }

  void removeEdge(S src, E edge, D dest) {
    sed.remove(src, edge);
    sde.remove(src, dest);
    esd.remove(edge, src);
    eds.remove(edge, dest);
    dse.remove(dest, src);
    des.remove(dest, edge);
  }

  bool containsEdge(S src, E edge, D dest) { return sed.contains(src, edge); }

  D getDbySE(S src, E edge) { return sed.get(src, edge); }

  E getEbySD(S src, D dest) { return sde.get(src, dest); }

  D getDbyES(E edge, S src) { return esd.get(edge, src); }

  S getSbyED(E edge, D dest) { return eds.get(edge, dest); }

  E getEbyDS(D dest, S src) { return dse.get(dest, src); }

  S getSyDE(D dest, E edge) { return des.get(dest, edge); }

  void clear() {
    sed.clear();
    sde.clear();
    esd.clear();
    eds.clear();
    dse.clear();
    des.clear();
  }

  bool empty() { return sed.empty(); }

  std::set<typename Table<S, E, D>::Cell> tripleSet() { return sed.cellSet(); }

  std::multiset<S> sourceSet() { return sed.rowKeySet(); }

  std::multiset<E> edgeSet() { return sed.columnKeySet(); }

  std::multiset<D> destinationSet() { return sed.values(); }

  void loadHexaStoreGraphFromDB(const std::string &tablename) {
    cerr << "Not implemented yet!" << std::endl;
  }

  void storeHexaStoreGraphToDB(const std::string &tablename) {
    cerr << "Not implemented yet!" << std::endl;
  }

  friend bool operator==(const HexaStoreGraph<S, E, D> &lhs,
                         const HexaStoreGraph<S, E, D> &rhs) {
    return lhs.sed == rhs.sed;
  }

  friend bool operator<(const HexaStoreGraph<S, E, D> &lhs,
                        const HexaStoreGraph<S, E, D> &rhs) {
    return lhs.sed < rhs.sed;
  }

  friend std::ostream &operator<<(std::ostream &os, const HexaStoreGraph &hsg) {
    return os << hsg.sed;
  }
};

} // namespace psr

#endif

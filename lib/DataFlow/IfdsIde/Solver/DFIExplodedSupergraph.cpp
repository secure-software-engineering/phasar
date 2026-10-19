/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/DFIExplodedSupergraph.h"

#include "phasar/Utils/IotaIterator.h"

#include "llvm/ADT/STLExtras.h"

#include <algorithm>

using psr::detail::DFIExplodedSupergraph;

DFIExplodedSupergraph::DFIExplodedSupergraph(
    psr::SCCHolder<psr::FunctionId> CGSCCs, bool FollowReturnsPastSeeds)
    : CGSCCs(std::move(CGSCCs)),
      FollowReturnsPastSeeds(FollowReturnsPastSeeds) {
  FunState.resize(this->CGSCCs.SCCOfNode.size());
  PendingSCCs.reserve(this->CGSCCs.size());
}

auto DFIExplodedSupergraph::nextAction() -> std::optional<Action> {
  while (auto SCC = PendingSCCs.findFirst()) {
    const auto &FunsInSCC = CGSCCs.NodesInSCC[*SCC];
    for (auto Fun : FunsInSCC) {
      if (!FunState[Fun].Worklist.empty()) {
        return Action{Fun, ActionKind::Expand};
      }
    }
    for (auto Fun : FunsInSCC) {
      if (FunState[Fun].Dirty) {
        return Action{Fun, ActionKind::Summarize};
      }
    }
    PendingSCCs.erase(*SCC);
  }
  return std::nullopt;
}

psr::dfi::VertexId DFIExplodedSupergraph::getOrCreateVertex(dfi::InstId Inst,
                                                            dfi::FactId Fact,
                                                            FunctionId Fun,
                                                            bool IsExitVtx) {
  auto [It, Inserted] =
      VertexOf.try_emplace({Inst, Fact}, dfi::VertexId(VtxInst.size()));
  auto Vtx = It->second;
  if (!Inserted) {
    return Vtx;
  }

  auto &FS = FunState[Fun];
  VtxInst.push_back(Inst);
  VtxFact.push_back(Fact);
  VtxFun.push_back(Fun);
  VtxLocalId.push_back(dfi::LocalVertexId(FS.Vertices.size()));
  VtxEdgeBegin.push_back(UINT32_MAX);
  VtxEdgeEnd.push_back(0);
  FS.Vertices.push_back(Vtx);
  if (IsExitVtx) {
    IsExit.insert(Vtx);
  }

  FS.Worklist.push_back(Vtx);
  markDirty(Fun);
  return Vtx;
}

bool DFIExplodedSupergraph::addSeed(dfi::VertexId Vtx) {
  if (!addRoot(Vtx)) {
    return false;
  }
  SeedVertices.push_back(Vtx);
  FunState[VtxFun[Vtx]].Seeds.push_back(Vtx);
  return true;
}

psr::dfi::EntryId DFIExplodedSupergraph::getOrCreateEntry(dfi::VertexId Vtx) {
  auto [It, Inserted] =
      EntryOf.try_emplace(Vtx, dfi::EntryId(EntryVertex.size()));
  auto Entry = It->second;
  if (Inserted) {
    EntryVertex.push_back(Vtx);
    EntryCallers.emplace_back();
    SummaryExits.emplace_back();
    FunState[VtxFun[Vtx]].Entries.push_back(Entry);
    addRoot(Vtx);
  }
  return Entry;
}

llvm::ArrayRef<psr::dfi::VertexId>
DFIExplodedSupergraph::takeWorklist(FunctionId Fun) {
  Batch.clear();
  std::swap(Batch, FunState[Fun].Worklist);
  llvm::sort(Batch, [this](dfi::VertexId Lhs, dfi::VertexId Rhs) {
    return VtxInst[Lhs] < VtxInst[Rhs];
  });
  return Batch;
}

bool DFIExplodedSupergraph::addSummaryEdge(dfi::VertexId CallVtx,
                                           dfi::VertexId RetVtx) {
  if (!SummaryEdgeSet.insert({CallVtx, RetVtx}).second) {
    return false;
  }
  auto Caller = VtxFun[CallVtx];
  FunState[Caller].SummaryEdges.emplace_back(CallVtx, RetVtx);
  markDirty(Caller);
  return true;
}

llvm::ArrayRef<psr::dfi::VertexId>
DFIExplodedSupergraph::addCallEdge(dfi::VertexId CallVtx, dfi::EntryId Entry) {
  EntryCallers[Entry].push_back(CallVtx);
  CallEdges.emplace_back(CallVtx, EntryVertex[Entry]);
  return SummaryExits[Entry];
}

void DFIExplodedSupergraph::addUnbalancedReturn(dfi::VertexId Exit,
                                                dfi::VertexId RetVtx) {
  if (!UnbalancedReturnEdges.insert({Exit, RetVtx}).second) {
    return;
  }

  auto Fun = VtxFun[RetVtx];
  if (addRoot(RetVtx)) {
    FunState[Fun].UnbalancedRoots.push_back(RetVtx);
  } else if (!llvm::is_contained(FunState[Fun].UnbalancedRoots, RetVtx)) {
    // An entry or seed; still, it may now reach exits via the zero entry
    FunState[Fun].UnbalancedRoots.push_back(RetVtx);
    markDirty(Fun);
  }
}

psr::CsrGraph<psr::dfi::VertexId>
DFIExplodedSupergraph::buildSupergraph() const {
  assert(VtxInst.size() == VtxFun.size() &&
         "Must not build the supergraph after takeVertexInsts()");
  llvm::SmallVector<std::pair<dfi::VertexId, dfi::VertexId>, 0> GraphEdges;
  GraphEdges.reserve(Edges.size() + SummaryEdgeSet.size() + CallEdges.size());
  for (auto From : iota<dfi::VertexId>(numVertices())) {
    for (auto To : edgesOf(From)) {
      GraphEdges.emplace_back(From, To);
    }
  }
  for (const auto &FS : FunState) {
    GraphEdges.append(FS.SummaryEdges);
  }
  GraphEdges.append(CallEdges);
  GraphEdges.append(UnbalancedReturnEdges.begin(), UnbalancedReturnEdges.end());
  return CsrGraph<dfi::VertexId>::fromEdges(numVertices(), GraphEdges);
}

void DFIExplodedSupergraph::markDirty(FunctionId Fun) {
  FunState[Fun].Dirty = true;
  PendingSCCs.insert(CGSCCs.SCCOfNode[Fun]);
}

bool DFIExplodedSupergraph::addRoot(dfi::VertexId Vtx) {
  if (!IsRoot.tryInsert(Vtx)) {
    return false;
  }
  auto Fun = VtxFun[Vtx];
  FunState[Fun].Roots.push_back(Vtx);
  markDirty(Fun);
  return true;
}

bool DFIExplodedSupergraph::hasCallers(dfi::VertexId Vtx) const {
  auto It = EntryOf.find(Vtx);
  return It != EntryOf.end() && !EntryCallers[It->second].empty();
}

bool DFIExplodedSupergraph::isZeroEntryCalled(FunctionId Fun) const {
  return llvm::any_of(FunState[Fun].Entries, [this](dfi::EntryId Entry) {
    return VtxFact[EntryVertex[Entry]] == dfi::FactId::Zero &&
           !EntryCallers[Entry].empty();
  });
}

void DFIExplodedSupergraph::indexFunction(FunctionId Fun) {
  auto &FS = FunState[Fun];
  FS.Dirty = false;

  LocalEdges.clear();
  for (auto Vtx : FS.Vertices) {
    auto From = VtxLocalId[Vtx];
    for (auto To : edgesOf(Vtx)) {
      LocalEdges.emplace_back(From, VtxLocalId[To]);
    }
  }
  for (auto [From, To] : FS.SummaryEdges) {
    LocalEdges.emplace_back(VtxLocalId[From], VtxLocalId[To]);
  }
  LocalGraph.assignEdges(FS.Vertices.size(), LocalEdges);

  LocalRoots.clear();
  for (auto Root : FS.Roots) {
    LocalRoots.push_back(VtxLocalId[Root]);
  }

  // Use out-variable to enable memory-reuse
  LocalBuilder.build(LocalGraph, LocalRoots, LocalIndex,
                     [this, &FS](dfi::LocalVertexId Local) {
                       return IsExit.contains(FS.Vertices[Local]);
                     });
}

void DFIExplodedSupergraph::collectNewExits(
    FunctionId Fun, dfi::VertexId Src, llvm::ArrayRef<dfi::VertexId> Known) {
  const auto &FS = FunState[Fun];
  auto LocalSrc = VtxLocalId[Src];
  if (size_t(LocalSrc) >= LocalIndex.numVertices()) {
    // Created after indexFunction(); covered by the next summarize()
    return;
  }

  LocalIndex.forEachReachableTarget(LocalSrc, [&](dfi::LocalVertexId Local) {
    auto Exit = FS.Vertices[Local];
    if (!std::binary_search(Known.begin(), Known.end(), Exit)) {
      NewExits.push_back(Exit);
    }
  });
}

void DFIExplodedSupergraph::mergeNewExits(
    llvm::SmallVectorImpl<dfi::VertexId> &Known) {
  if (NewExits.empty()) {
    return;
  }
  llvm::sort(NewExits);
  NewExits.erase(std::unique(NewExits.begin(), NewExits.end()), NewExits.end());
  Known.append(NewExits.begin(), NewExits.end());
  llvm::sort(Known);
}

llvm::ArrayRef<psr::dfi::VertexId>
DFIExplodedSupergraph::updateSummaryExits(FunctionId Fun, dfi::EntryId Entry) {
  auto &Exits = SummaryExits[Entry];
  auto EntryVtx = EntryVertex[Entry];

  NewExits.clear();
  collectNewExits(Fun, EntryVtx, Exits);
  if (VtxFact[EntryVtx] == dfi::FactId::Zero) {
    for (auto Root : FunState[Fun].UnbalancedRoots) {
      collectNewExits(Fun, Root, Exits);
    }
  }

  mergeNewExits(Exits);
  return NewExits;
}

llvm::ArrayRef<psr::dfi::VertexId>
DFIExplodedSupergraph::updateUnbalancedExits(FunctionId Fun) {
  auto &FS = FunState[Fun];

  NewExits.clear();
  for (auto Seed : FS.Seeds) {
    if (!hasCallers(Seed)) {
      collectNewExits(Fun, Seed, FS.UnbalancedExits);
    }
  }
  // As in the IDESolver, facts from unbalanced returns behave as if they were
  // reached from the zero entry, so they only return past this function if
  // the zero entry has no callers.
  if (!isZeroEntryCalled(Fun)) {
    for (auto Root : FS.UnbalancedRoots) {
      collectNewExits(Fun, Root, FS.UnbalancedExits);
    }
  }

  mergeNewExits(FS.UnbalancedExits);
  return NewExits;
}

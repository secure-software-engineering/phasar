/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONSINGLETONFACTORY_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONSINGLETONFACTORY_H_

#include "llvm/Support/Compiler.h" // For LLVM_DUMP_METHOD
#include "llvm/Support/raw_ostream.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

namespace psr {

namespace internal {
struct TestEdgeFunction;
} // namespace internal

// This class can be used as a singleton factory for EdgeFunctions that only
// have constant data.
//
// EdgeFunction that have constant data only need to be allocated once and can
// be turned into a singleton, which save large amounts of memory. The
// automatic singleton creation can be enabled for a EdgeFunction class by
// inheriting from EdgeFunctionSingletonFactory and only allocating
// EdgeFunction throught the provided createEdgeFunction method.
//
// Old unused EdgeFunction that were deallocated need to/should be clean from
// the factory by either calling cleanExpiredEdgeFunctions or initializing the
// automatic cleaner thread with initEdgeFunctionCleaner.
template <typename EdgeFunctionType, typename CtorArgT>
class EdgeFunctionSingletonFactory {
public:
  EdgeFunctionSingletonFactory() = default;
  EdgeFunctionSingletonFactory(const EdgeFunctionSingletonFactory &) = default;
  EdgeFunctionSingletonFactory &
  operator=(const EdgeFunctionSingletonFactory &) = default;
  EdgeFunctionSingletonFactory(EdgeFunctionSingletonFactory &&) noexcept =
      default;
  EdgeFunctionSingletonFactory &
  operator=(EdgeFunctionSingletonFactory &&) noexcept = default;

  virtual ~EdgeFunctionSingletonFactory() {
    std::lock_guard DataLock(getCacheData().DataMutex);
  }

  // Creates a new EdgeFunction of type EdgeFunctionType, reusing the previous
  // allocation if an EdgeFunction with the same values was already created.
  static inline std::shared_ptr<EdgeFunctionType>
  createEdgeFunction(CtorArgT K) {
    std::lock_guard DataLock(getCacheData().DataMutex);

    auto &Storage = getCacheData().Storage;

    auto &Slot = Storage[K];
    auto EdgeFun = Slot.lock();
    if (!EdgeFun) {
      EdgeFun = std::make_shared<EdgeFunctionType>(std::move(K));
      Slot = EdgeFun;
    }

    return EdgeFun;
  }

  // Initialize a cleaner thread to automatically remove unused/expired
  // EdgeFunctions. The thread is only started the first time this function is
  // called.
  static inline void initEdgeFunctionCleaner() { getCleaner(); }

  static inline void stopEdgeFunctionCleaner() { getCleaner().stop(); }

  // Clean all unused/expired EdgeFunctions from the internal storage.
  static inline void cleanExpiredEdgeFunctions() {
    cleanExpiredMapEntries(getCacheData());
  }

  LLVM_DUMP_METHOD
  static void dump(bool PrintElements = false) {
    std::lock_guard DataLock(getCacheData().DataMutex);

    llvm::outs() << "Elements in cache: " << getCacheData().Storage.size();

    if (PrintElements) {
      llvm::outs() << "\n";
      for (auto &KVPair : getCacheData().Storage) {
        llvm::outs() << "(" << KVPair.first << ") -> "
                     << KVPair.second.expired() << '\n';
      }
    }
    llvm::outs() << '\n';
  }

private:
  struct EFStorageData {
    std::map<CtorArgT, std::weak_ptr<EdgeFunctionType>> Storage{};
    std::mutex DataMutex;
  };

  static inline EFStorageData &getCacheData() {
    static EFStorageData StoredData{};
    return StoredData;
  }

  static void cleanExpiredMapEntries(EFStorageData &CData) {
    std::lock_guard DataLock(CData.DataMutex);

    auto &Storage = CData.Storage;
    for (auto Iter = Storage.begin(); Iter != Storage.end();) {
      if (Iter->second.expired()) {
        Iter = Storage.erase(Iter);
      } else {
        ++Iter;
      }
    }
  }

  class EdgeFactoryStorageCleaner {
  public:
    EdgeFactoryStorageCleaner()
        : CleanerThread(
              &EdgeFactoryStorageCleaner::runCleaner, this,
              std::ref(EdgeFunctionSingletonFactory::getCacheData())) {
#ifdef __linux__
      pthread_setname_np(CleanerThread.native_handle(),
                         "EdgeFactoryStorageCleanerThread");
#endif
    }
    EdgeFactoryStorageCleaner(const EdgeFactoryStorageCleaner &) = delete;
    EdgeFactoryStorageCleaner &
    operator=(const EdgeFactoryStorageCleaner &) = delete;
    EdgeFactoryStorageCleaner(EdgeFactoryStorageCleaner &&) = delete;
    EdgeFactoryStorageCleaner &operator=(EdgeFactoryStorageCleaner &&) = delete;
    ~EdgeFactoryStorageCleaner() { stop(); }

    // Stops the cleaning thread running in the background.
    void stop() {
      KeepRunning = false;
      if (CleanerThread.joinable()) {
        CleanerThread.join();
      }
    }

  private:
    void runCleaner(EFStorageData &CData) {
      while (KeepRunning) {
        std::this_thread::sleep_for(CleaningPauseTime);
        cleanExpiredMapEntries(CData);
      }
    }

    static constexpr auto CleaningPauseTime = std::chrono::seconds(2);

    std::atomic_bool KeepRunning{true};
    std::thread CleanerThread;
  };

  static inline EdgeFactoryStorageCleaner &getCleaner() {
    static EdgeFactoryStorageCleaner EdgeFunctionCleaner{};
    return EdgeFunctionCleaner;
  }

  friend internal::TestEdgeFunction;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONSINGLETONFACTORY_H_

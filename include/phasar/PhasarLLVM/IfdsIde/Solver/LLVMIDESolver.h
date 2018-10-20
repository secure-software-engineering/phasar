/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMIDESolver.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_LLVMIDESOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_LLVMIDESOLVER_H_

#include <algorithm>
// #include <iostream>
#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/IDESolver.h>

using json = nlohmann::json;

namespace psr {

template <typename D, typename V, typename I>
class LLVMIDESolver : public IDESolver<const llvm::Instruction *, D,
                                       const llvm::Function *, V, I> {
private:
  IDETabulationProblem<const llvm::Instruction *, D, const llvm::Function *, V,
                       I> &Problem;
  const bool DUMP_RESULTS;
  const bool PRINT_REPORT;

public:
  LLVMIDESolver(IDETabulationProblem<const llvm::Instruction *, D,
                                     const llvm::Function *, V, I> &problem,
                bool dumpResults = false, bool printReport = true)
      : IDESolver<const llvm::Instruction *, D, const llvm::Function *, V, I>(
            problem),
        Problem(problem), DUMP_RESULTS(dumpResults), PRINT_REPORT(printReport) {
  }

  virtual ~LLVMIDESolver() = default;

  void solve() override {
    IDESolver<const llvm::Instruction *, D, const llvm::Function *, V,
              I>::solve();
    bl::core::get()->flush();
    if (DUMP_RESULTS) {
      dumpResults();
    }
    if (PRINT_REPORT) {
      printReport();
    }
  }

  void printReport() {
    SolverResults<const llvm::Instruction *, D, V> SR(this->valtab,
                                                      Problem.zeroValue());
    Problem.printIDEReport(std::cout, SR);
  }

  void dumpResults() {
    PAMM_GET_INSTANCE;
    START_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
    std::cout << "### DUMP LLVMIDESolver results\n";
    // for the following line have a look at:
    // http://stackoverflow.com/questions/1120833/derived-template-class-access-to-base-class-member-data
    // https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      std::cout << "EMPTY" << std::endl;
    } else {
      std::vector<typename Table<const llvm::Instruction *, D, V>::Cell> cells;
      for (auto cell : results) {
        cells.push_back(cell);
      }
      std::sort(cells.begin(), cells.end(),
                [](typename Table<const llvm::Instruction *, D, V>::Cell a,
                   typename Table<const llvm::Instruction *, D, V>::Cell b) {
                  return a.r < b.r;
                });
      const llvm::Instruction *prev = nullptr;
      const llvm::Instruction *curr;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
        if (prev != curr) {
          prev = curr;
          std::cout << "--- IDE START RESULT RECORD ---\n";
          std::cout << "N: " << Problem.NtoString(cells[i].r)
                    << " in function: ";
          if (const llvm::Instruction *inst =
                  llvm::dyn_cast<llvm::Instruction>(cells[i].r)) {
            std::cout << inst->getFunction()->getName().str() << "\n";
          }
        }
        std::cout << "D:\t" << Problem.DtoString(cells[i].c) << " "
                  << "\tV:  " << Problem.VtoString(cells[i].v) << "\n";
      }
    }
    std::cout << '\n';
    STOP_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
  }

  json getJsonRepresentationForInstructionNode(const llvm::Instruction *node) {
    json jsonNode = {
        {"number", node_number},
        {"method_name", node->getFunction()->getName().str().c_str()},
        {"instruction", llvmIRToString(node).c_str()},
        {"type", 0}};
    node_number = node_number + 1;
    return jsonNode;
  }
  json getJsonRepresentationForCallsite(const llvm::Instruction *from,
                                        const llvm::Instruction *to) {
    json jsonNode = {
        {"number", node_number},
        {"method_name", from->getFunction()->getName().str().c_str()},
        {"to", to->getFunction()->getName().str().c_str()},
        {"type", 1}};
    node_number = node_number + 1;
    return jsonNode;
  }
  json getJsonRepresentationForReturnsite(const llvm::Instruction *from,
                                          const llvm::Instruction *to) {
    json jsonNode = {
        {"number", node_number},
        {"from", from->getFunction()->getName().str().c_str()},
        {"method_name", to->getFunction()->getName().str().c_str()},
        {"type", 2}};

    node_number = node_number + 1;
    return jsonNode;
  }

  int node_number = 0;
  /**
   * gets id for node from map or adds it if it doesn't exist
   **/
  json
  getJsonOfNode(const llvm::Instruction *node,
                std::map<const llvm::Instruction *, int> *instruction_id_map) {
    std::map<const llvm::Instruction *, int>::iterator it =
        instruction_id_map->find(node);
    json jsonNode;

    if (it == instruction_id_map->end()) {
      std::cout << "adding new element to std::map " << std::endl;

      jsonNode = getJsonRepresentationForInstructionNode(node);

      sendToWebserver(jsonNode.dump().c_str());
      instruction_id_map->insert(
          std::pair<const llvm::Instruction *, int>(node, node_number));
    } else {
      std::cout << "found element in std::map(inter): " << it->second
                << std::endl;
    }
    return jsonNode;
  }

  void iterateExplodedSupergraph(
      const llvm::Instruction *currentNode,
      const llvm::Function *callerFunction,
      std::map<const llvm::Instruction *, int> *instruction_id_map) {
    // In the next line we obtain the corresponding row map which maps (given a
    // source node) the target node to the data flow fact map<D, set<D>. In the
    // data flow fact map D is a fact F which holds at the source node whereas
    // set<D> contains the facts that are produced by F and hold at statement
    // TargetNode. Usually every node has one successor node, that is why the
    // row map obtained by row usually only contains just a single entry. BUT:
    // in case of branch statements and other advanced instructions, one
    // statement sometimes has multiple successor statments. In these cases the
    // row map contains entries for every single successor statement. After
    // having obtained the pairs <SourceNode, TargetNode> the data flow map can
    // be obtained easily. size_t from =
    // getJsonRepresentationForInstructionNode(document, currentNode);
    json fromNode = getJsonOfNode(currentNode, instruction_id_map);

    auto TargetNodeMap = this->computedIntraPathEdges.row(currentNode);
    std::cout << "node pointer current: " << currentNode << std::endl;

    std::cout << "TARGET NODE(S)\n";
    for (auto entry : TargetNodeMap) {
      auto TargetNode = entry.first;
      // use std::map to store key value and match node to json id
      json toNode = getJsonOfNode(TargetNode, instruction_id_map);
      std::cout << "node pointer target : " << TargetNode << std::endl;

      // getJsonRepresentationForInstructionEdge(from, to, document);
      std::cout << "NODE (in function "
                << TargetNode->getFunction()->getName().str() << ")\n";
      TargetNode->print(llvm::outs());

      auto FlowFactMap = entry.second;
      // for (auto FlowFactEntry : FlowFactMap)
      // {
      //     auto FlowFactAtStart = FlowFactEntry.first;
      //     auto ProducedFlowFactsAtTarget = FlowFactEntry.second;
      //     std::cout << "FLOW FACT AT SourceNode:\n";
      //     FlowFactAtStart->dump(); // this would be the place for something
      //     like 'DtoString()'
      //     size_t fromData = getJsonRepresentationForFlowFactNode(document,
      //     from, &FlowFactAtStart);
      //     std::cout << "IS PRODUCING FACTS AT TARGET NODE:\n";
      //     for (auto ProdFlowFact : ProducedFlowFactsAtTarget)
      //     {
      //         size_t toData = getJsonRepresentationForFlowFactNode(document,
      //         to, &ProdFlowFact);
      //         ProdFlowFact->dump(); // this would be the place for something
      //         like 'DtoString()'
      //         getJsonRepresentationForDataFlowEdge(fromData, toData,
      //         document);
      //     }
      // }

      if (this->computedInterPathEdges.containsRow(TargetNode)) {
        std::cout << "FOUND Inter path edge !!" << std::endl;
        auto interEdgeTargetMap = this->computedInterPathEdges.row(TargetNode);

        for (auto interEntry : interEdgeTargetMap) {
          // this doesn't seem to work right.. wait for
          // instruction.dump().toString()
          // for easier debugging of the graph
          if (interEntry.first->getFunction()->getName().str().compare(
                  callerFunction->getName().str()) != 0) {
            std::cout << "callsite: " << std::endl;
            TargetNode->print(llvm::outs());
            interEntry.first->print(llvm::outs());
            json callSiteNode =
                getJsonRepresentationForCallsite(TargetNode, interEntry.first);

            sendToWebserver(callSiteNode.dump().c_str());

            fromNode = getJsonOfNode(TargetNode, instruction_id_map);
            toNode = getJsonOfNode(interEntry.first, instruction_id_map);

            // getJsonRepresentationForInstructionEdge(from, to);

            std::cout << "NODE (in function (inter)"
                      << interEntry.first->getFunction()->getName().str()
                      << ")\n";
            interEntry.first->print(llvm::outs());
            // add function start node here
            iterateExplodedSupergraph(interEntry.first,
                                      TargetNode->getFunction(),
                                      instruction_id_map);
          } else {
            std::cout << "FOUND Return Side" << std::endl;
            json returnSiteNode = getJsonRepresentationForReturnsite(
                TargetNode, interEntry.first);
            // add function end node here
            sendToWebserver(returnSiteNode.dump().c_str());
            fromNode = getJsonOfNode(TargetNode, instruction_id_map);
            toNode = getJsonOfNode(interEntry.first, instruction_id_map);
          }
        }
      }
    }

    for (auto entry : TargetNodeMap) {
      iterateExplodedSupergraph(entry.first, callerFunction,
                                instruction_id_map);
    }
  }

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

  CURL *curl;
  std::string getIdFromWebserver() {
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL,
                       "http://localhost:3000/api/framework/getId");
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);

      std::cout << readBuffer << std::endl;
      auto response = json::parse(readBuffer);
      std::cout << response["my_id"] << std::endl;
      return response["my_id"];
    }
    return 0;
  }

  void sendToWebserver(const char *jsonString) {
    if (curl) {
      printf("Json String: %s \n", jsonString);
      // setting correct headers so that the server will interpret
      // the post body as json
      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Accept: application/json");
      headers = curl_slist_append(headers, "Content-Type: application/json");
      headers = curl_slist_append(headers, "charsets: utf-8");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      /* pass in a pointer to the data - libcurl will not copy */
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString);
      /* Perform the request, res will get the return code */
      CURLcode res = curl_easy_perform(curl);
      /* Check for errors */
      if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
      }
    }
  }

  void sendWebserverFinish(const char *url) {
    curl = curl_easy_init();
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, url);

      CURLcode res = curl_easy_perform(curl);
      /* Check for errors */
      if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
      }

      curl_easy_cleanup(curl);
    }
  }

  void exportJSONDataModel(std::string graph_id) {
    curl_global_init(CURL_GLOBAL_ALL);
    std::string id = graph_id;
    std::cout << "my id: " << graph_id << std::endl;
    /* get a curl handle */
    curl = curl_easy_init();
    std::string url = "http://localhost:3000/api/framework/addGraph/" + id;
    std::cout << url << std::endl;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    for (auto Seed : this->initialSeeds) {
      std::map<const llvm::Instruction *, int> instruction_id_map;

      auto SourceNode = Seed.first;

      std::cout << "START NODE (in function "
                << SourceNode->getFunction()->getName().str() << ")\n";
      SourceNode->print(llvm::outs());
      std::cout << " source node name " << SourceNode->getName().str()
                << std::endl;
      std::cout << " source node opcode name" << SourceNode->getOpcodeName()
                << std::endl;

      iterateExplodedSupergraph(SourceNode, SourceNode->getFunction(),
                                &instruction_id_map);
    }
    url = "http://localhost:3000/api/framework/graphFinish/" + id;
    sendWebserverFinish(url.c_str());
  }

  void dumpAllInterPathEdges() {
    std::cout << "COMPUTED INTER PATH EDGES" << std::endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      std::cout << "FROM" << std::endl;
      cell.r->dump();
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "FACTS" << std::endl;
      for (auto &fact : cell.v) {
        std::cout << "fact" << std::endl;
        fact.first->dump();
        std::cout << "produces" << std::endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }

  void dumpAllIntraPathEdges() {
    std::cout << "COMPUTED INTRA PATH EDGES" << std::endl;
    auto intrape = this->computedIntraPathEdges.cellSet();
    for (auto &cell : intrape) {
      std::cout << "FROM" << std::endl;
      cell.r->dump();
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "FACTS" << std::endl;
      for (auto &fact : cell.v) {
        std::cout << "fact" << std::endl;
        fact.first->dump();
        std::cout << "produces" << std::endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }

  void exportPATBCJSON() { std::cout << "LLVMIDESolver::exportPATBCJSON()\n"; }
};
} // namespace psr

#endif

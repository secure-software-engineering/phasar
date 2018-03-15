/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMIFDSSolver.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_H_
#define ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_H_

#include "../../../utils/Table.h"
#include "../../control_flow/ICFG.h"
#include "../IFDSTabulationProblem.h"
#include "IFDSSolver.h"
#include "json.hpp"
#include <algorithm>
#include <curl/curl.h>
#include <map>
#include <string>

using json = nlohmann::json;
using namespace std;

template <typename D, typename I>
class LLVMIFDSSolver : public IFDSSolver<const llvm::Instruction *, D,
                                         const llvm::Function *, I> {
private:
  const bool DUMP_RESULTS;
  IFDSTabulationProblem<const llvm::Instruction *, D, const llvm::Function *, I>
      &Problem;

public:
  virtual ~LLVMIFDSSolver() = default;

  LLVMIFDSSolver(IFDSTabulationProblem<const llvm::Instruction *, D,
                                       const llvm::Function *, I> &problem,
                 bool dumpResults = false)
      : IFDSSolver<const llvm::Instruction *, D, const llvm::Function *, I>(
            problem),
        DUMP_RESULTS(dumpResults), Problem(problem) {
    cout << "LLVMIFDSSolver::LLVMIFDSSolver()" << endl;
    cout << problem.NtoString(getNthInstruction(
                problem.interproceduralCFG().getMethod("main"), 1))
         << endl;
    cout << Problem.NtoString(getNthInstruction(
                Problem.interproceduralCFG().getMethod("main"), 1))
         << endl;
  }

  virtual void solve() override {
    // Solve the analaysis problem
    IFDSSolver<const llvm::Instruction *, D, const llvm::Function *,
               I>::solve();
    bl::core::get()->flush();
    if (DUMP_RESULTS)
      dumpResults();
  }

  void dumpResults() {
    PAMM_FACTORY;
    START_TIMER("DFA Result Dumping");
    cout << "### DUMP LLVMIFDSSolver results\n";
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      cout << "EMPTY\n";
    } else {
      vector<typename Table<const llvm::Instruction *, const llvm::Value *,
                            BinaryDomain>::Cell>
          cells;
      for (auto cell : results) {
        cells.push_back(cell);
      }
      sort(cells.begin(), cells.end(),
           [](typename Table<const llvm::Instruction *, const llvm::Value *,
                             BinaryDomain>::Cell a,
              typename Table<const llvm::Instruction *, const llvm::Value *,
                             BinaryDomain>::Cell b) { return a.r < b.r; });
      const llvm::Instruction *prev = nullptr;
      const llvm::Instruction *curr;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
        if (prev != curr) {
          prev = curr;
          cout << "--- IFDS START RESULT RECORD ---\n";
          cout << "N: " << Problem.NtoString(cells[i].r) << " in function: ";
          if (const llvm::Instruction *inst =
                  llvm::dyn_cast<llvm::Instruction>(cells[i].r)) {
            cout << inst->getFunction()->getName().str() << "\n";
          }
        }
        cout << "D:\t";
        if (cells[i].c == nullptr)
          cout << "  nullptr " << endl;
        else
          cout << Problem.DtoString(cells[i].c) << " "
               << "\tV:  " << cells[i].v << "\n";
      }
    }
    STOP_TIMER("DFA Result Dumping");
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
      cout << "adding new element to map " << endl;

      jsonNode = getJsonRepresentationForInstructionNode(node);

      sendToWebserver(jsonNode.dump().c_str());
      instruction_id_map->insert(
          std::pair<const llvm::Instruction *, int>(node, node_number));
    } else {
      cout << "found element in map(inter): " << it->second << endl;
    }
    return jsonNode;
  }
  void iterateExplodedSupergraph(
      const llvm::Instruction *currentNode,
      const llvm::Function *callerFunction,
      std::map<const llvm::Instruction *, int> *instruction_id_map) {
    // In the next line we obtain the corresponding row map which maps (given a
    // source node)
    // the target node to the data flow fact map<D, set<D>. In the data flow
    // fact map D is
    // a fact F which holds at the source node whereas set<D> contains the facts
    // that are
    // produced by F and hold at statement TargetNode.
    // Usually every node has one successor node, that is why the row map
    // obtained by row usually
    // only contains just a single entry. BUT: in case of branch statements and
    // other advanced
    // instructions, one statement sometimes has multiple successor statments.
    // In these cases
    // the row map contains entries for every single successor statement. After
    // having obtained
    // the pairs <SourceNode, TargetNode> the data flow map can be obtained
    // easily.
    // size_t from = getJsonRepresentationForInstructionNode(document,
    // currentNode);

    json fromNode = getJsonOfNode(currentNode, instruction_id_map);

    auto TargetNodeMap = this->computedIntraPathEdges.row(currentNode);
    cout << "node pointer current: " << currentNode << endl;

    cout << "TARGET NODE(S)\n";
    for (auto entry : TargetNodeMap) {
      auto TargetNode = entry.first;
      // use map to store key value and match node to json id
      json toNode = getJsonOfNode(TargetNode, instruction_id_map);
      cout << "node pointer target : " << TargetNode << endl;

      // getJsonRepresentationForInstructionEdge(from, to, document);
      cout << "NODE (in function " << TargetNode->getFunction()->getName().str()
           << ")\n";
      TargetNode->dump();

      auto FlowFactMap = entry.second;
      // for (auto FlowFactEntry : FlowFactMap)
      // {
      //     auto FlowFactAtStart = FlowFactEntry.first;
      //     auto ProducedFlowFactsAtTarget = FlowFactEntry.second;
      //     cout << "FLOW FACT AT SourceNode:\n";
      //     FlowFactAtStart->dump(); // this would be the place for something
      //     like 'DtoString()'
      //     size_t fromData = getJsonRepresentationForFlowFactNode(document,
      //     from, &FlowFactAtStart);
      //     cout << "IS PRODUCING FACTS AT TARGET NODE:\n";
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
        cout << "FOUND Inter path edge !!" << endl;
        auto interEdgeTargetMap = this->computedInterPathEdges.row(TargetNode);

        for (auto interEntry : interEdgeTargetMap) {
          // this doesn't seem to work right.. wait for
          // instruction.dump().toString()
          // for easier debugging of the graph
          if (interEntry.first->getFunction()->getName().str().compare(
                  callerFunction->getName().str()) != 0) {
            cout << "callsite: " << endl;
            TargetNode->dump();
            interEntry.first->dump();
            json callSiteNode =
                getJsonRepresentationForCallsite(TargetNode, interEntry.first);

            sendToWebserver(callSiteNode.dump().c_str());

            fromNode = getJsonOfNode(TargetNode, instruction_id_map);
            toNode = getJsonOfNode(interEntry.first, instruction_id_map);

            // getJsonRepresentationForInstructionEdge(from, to);

            cout << "NODE (in function (inter)"
                 << interEntry.first->getFunction()->getName().str() << ")\n";
            interEntry.first->dump();
            // add function start node here
            iterateExplodedSupergraph(interEntry.first,
                                      TargetNode->getFunction(),
                                      instruction_id_map);
          } else {
            cout << "FOUND Return Side" << endl;
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
  string getIdFromWebserver() {
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

  void exportJSONDataModel(string graph_id) {
    curl_global_init(CURL_GLOBAL_ALL);
    string id = graph_id;
    cout << "my id: " << graph_id << endl;
    /* get a curl handle */
    curl = curl_easy_init();
    string url = "http://localhost:3000/api/framework/addGraph/" + id;
    cout << url << endl;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    for (auto Seed : this->initialSeeds) {
      std::map<const llvm::Instruction *, int> instruction_id_map;

      auto SourceNode = Seed.first;

      cout << "START NODE (in function "
           << SourceNode->getFunction()->getName().str() << ")\n";
      SourceNode->dump();
      cout << " source node name " << SourceNode->getName().str() << endl;
      cout << " source node opcode name" << SourceNode->getOpcodeName() << endl;

      iterateExplodedSupergraph(SourceNode, SourceNode->getFunction(),
                                &instruction_id_map);
    }
    url = "http://localhost:3000/api/framework/graphFinish/" + id;
    sendWebserverFinish(url.c_str());
  }

  void dumpAllInterPathEdges() {
    cout << "COMPUTED INTER PATH EDGES" << endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      cout << "FROM" << endl;
      cell.r->dump();
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      cout << "TO" << endl;
      cell.c->dump();
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      cout << "FACTS" << endl;
      for (auto &fact : cell.v) {
        cout << "fact" << endl;
        fact.first->dump();
        cout << "produces" << endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }

  void dumpAllIntraPathEdges() {
    cout << "COMPUTED INTRA PATH EDGES" << endl;
    auto intrape = this->computedIntraPathEdges.cellSet();
    for (auto &cell : intrape) {
      cout << "FROM" << endl;
      cell.r->dump();
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      cout << "TO" << endl;
      cell.c->dump();
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      cout << "FACTS" << endl;
      for (auto &fact : cell.v) {
        cout << "fact" << endl;
        fact.first->dump();
        cout << "produces" << endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }

  void exportPATBCJSON() { cout << "LLVMIFDSSolver::exportPATBCJSON()\n"; }
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_HH_ */

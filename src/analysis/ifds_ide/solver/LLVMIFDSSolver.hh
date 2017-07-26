/*
 * LLVMIFDSSolver.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_HH_

#include <map>
#include <algorithm>
#include <curl/curl.h>
#include "../IFDSTabulationProblem.hh"
#include "IFDSSolver.hh"
#include "../../icfg/ICFG.hh"
#include "../../../utils/Table.hh"
#include "json.hpp"
#include <sstream>

using json = nlohmann::json;
using namespace std;

template <class D, class I>
class LLVMIFDSSolver : public IFDSSolver<const llvm::Instruction *, D, const llvm::Function *, I>
{
  private:
	const bool DUMP_RESULTS;
	IFDSTabulationProblem<const llvm::Instruction *, D, const llvm::Function *, I> &Problem;

  public:
	virtual ~LLVMIFDSSolver() = default;

	LLVMIFDSSolver(IFDSTabulationProblem<const llvm::Instruction *, D, const llvm::Function *, I> &problem, bool dumpResults = false)
		: IFDSSolver<const llvm::Instruction *, D, const llvm::Function *, I>(problem),
		  DUMP_RESULTS(dumpResults), Problem(problem) {}

	virtual void solve() override
	{
		// do the solving of the analaysis problem
		IFDSSolver<const llvm::Instruction *, D, const llvm::Function *, I>::solve();
		if (DUMP_RESULTS)
			dumpResults();
	}

	void dumpResults()
	{
		cout << "I am a LLVMIFDSSolver result" << endl;
		cout << "### DUMP RESULTS" << endl;
		// TODO present results in a nicer way than just calling llvm's dump()
		// for the following line have a look at:
		// http://stackoverflow.com/questions/1120833/derived-template-class-access-to-base-class-member-data
		// https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
		auto results = this->valtab.cellSet();
		if (results.empty())
		{
			cout << "EMPTY" << endl;
		}
		else
		{
			vector<typename Table<const llvm::Instruction *, const llvm::Value *, BinaryDomain>::Cell> cells;
			for (auto cell : results)
			{
				cells.push_back(cell);
			}
			sort(cells.begin(), cells.end(),
				 [](typename Table<const llvm::Instruction *, const llvm::Value *, BinaryDomain>::Cell a,
					typename Table<const llvm::Instruction *, const llvm::Value *, BinaryDomain>::Cell b) {
					 return a.r < b.r;
				 });
			const llvm::Instruction *prev = nullptr;
			const llvm::Instruction *curr;
			for (unsigned i = 0; i < cells.size(); ++i)
			{
				curr = cells[i].r;
				if (prev != curr)
				{
					prev = curr;
					cout << "--- IFDS START RESULT RECORD ---" << endl;
					cout << "N" << endl;
					cells[i].r->dump();
					cout << "of function: ";
					if (const llvm::Instruction *inst = llvm::dyn_cast<llvm::Instruction>(cells[i].r))
					{
						cout << inst->getFunction()->getName().str() << endl;
					}
				}
				cout << "D" << endl;
				if (cells[i].c == nullptr)
					cout << "  nullptr" << endl;
				else
					cells[i].c->dump();
				cout << endl;
				cout << "V\n  ";
				cout << cells[i].v << endl;
			}
		}
		//		cout << "### IFDS RESULTS AT LAST STATEMENT OF MAIN" << endl;
		//		this->icfg.getLastInstructionOf("main")->dump();
		//		auto resultAtEnd = this->resultsAt(this->icfg.getLastInstructionOf("main"));
		//		if (resultAtEnd.empty()) {
		//			cout << "EMPTY" << endl;
		//		} else {
		//			for (auto entry : resultAtEnd) {
		//				cout << "\t--- begin entry ---" << endl;
		//				entry.first->dump();
		//				//cout << "from function: " << entry.first->getFunction().getName().str() << endl;
		//				cout << entry.second << endl;
		//				cout << "\t--- end entry ---" << endl;
		//			}
		//		}
		//		cout << "### IFDS END RESULTS AT LAST STATEMENT OF MAIN" << endl;
	}

	json getJsonRepresentationForInstructionNode(size_t id, const llvm::Instruction *node)
	{
		json jsonNode = {
			{"graph_id", getId("instructionNode_", id)},
			{"method", node->getFunction()->getName().str().c_str()},
			{"instruction", llvmIRToString(node).c_str()},
			{"type", 0}};

		return jsonNode;
	}
	json getJsonRepresentationForCallsite(size_t id, const llvm::Instruction *node)
	{
		json jsonNode = {
			{"graph_id", getId("instructionNode_", id)},
			{"method", node->getFunction()->getName().str().c_str()},
			{"type", 1}};

		return jsonNode;
	}
	json getJsonRepresentationForReturnsite(size_t id, const llvm::Instruction *node)
	{
		json jsonNode = {
			{"graph_id", getId("instructionNode_", id)},
			{"method", node->getFunction()->getName().str().c_str()},
			{"type", 2}};

		return jsonNode;
	}
	// size_t getJsonRepresentationForFlowFactNode(Document *document, size_t nodeid, D *node)
	// {
	// 	Document::AllocatorType &alloc = document->GetAllocator();
	// 	Value &a = document->operator[]("firstSeed");
	// 	Value &nodes = a["dataflowFacts"]["nodes"];
	// 	Value jsonNode(kObjectType);

	// 	Value id;
	// 	id.SetString(getId("flowFactNode_", nodes.Size()), alloc);

	// 	jsonNode.AddMember("instruction_id", (int)nodeid, alloc);
	// 	jsonNode.AddMember("id", id, alloc);
	// 	jsonNode.AddMember("type", "dataflowNode", alloc);
	// 	jsonNode.AddMember("instruction", "string representing fact->dump()", alloc);

	// 	Value data(kObjectType);
	// 	data.AddMember("data", jsonNode, alloc);
	// 	nodes.PushBack(data, alloc);
	// 	return nodes.Size() - 1;
	// }

	const char *getId(string idName, size_t id)
	{
		std::ostringstream oss;
		oss << idName << id;
		return oss.str().c_str();
	}

	// size_t getJsonRepresentationForDataFlowEdge(size_t from, size_t to)
	// {
	// 	Document::AllocatorType &alloc = document->GetAllocator();
	// 	Value &a = document->operator[]("firstSeed");
	// 	Value &edges = a["dataflowFacts"]["edges"];
	// 	Value jsonNode(kObjectType);

	// 	Value id;
	// 	id.SetString(getId("flowFactEdge_", edges.Size()), alloc);

	// 	Value s;
	// 	s.SetString(getId("flowFactNode_", from), alloc);

	// 	Value t;
	// 	t.SetString(getId("flowFactNode_", to), alloc);

	// 	jsonNode.AddMember("target", t, alloc);
	// 	jsonNode.AddMember("source", s, alloc);
	// 	jsonNode.AddMember("id", id, alloc);
	// 	jsonNode.AddMember("type", "dataflowEdge", alloc);

	// 	//a.PushBack(jsonNode, alloc);
	// 	Value data(kObjectType);
	// 	data.AddMember("data", jsonNode, alloc);
	// 	edges.PushBack(data, alloc);
	// 	return edges.Size() - 1;
	// }
	/**
	* gets id for node from map or adds it if it doesn't exist
	**/
	json getJsonOfNode(const llvm::Instruction *node, std::map<const llvm::Instruction *, size_t> *instruction_id_map, size_t node_type)
	{
		std::map<const llvm::Instruction *, size_t>::iterator it = instruction_id_map->find(node);
		json jsonNode;
		size_t id;
		if (it == instruction_id_map->end())
		{
			cout << "adding new element to map " << endl;
			id = instruction_id_map->size() + 1;
			switch (node_type)
			{
			case 0:
				jsonNode = getJsonRepresentationForInstructionNode(id, node);
				break;
			case 1:
				jsonNode = getJsonRepresentationForCallsite(id, node);
				break;
			case 2:
				jsonNode = getJsonRepresentationForReturnsite(id, node);
				break;
			}
			sendToWebserver(jsonNode.dump().c_str());
			instruction_id_map->insert(std::pair<const llvm::Instruction *, size_t>(node, id));
		}
		else
		{
			cout << "found element in map(inter): " << it->second << endl;
			id = it->second;
		}
		return jsonNode;
	}
	void iterateExplodedSupergraph(const llvm::Instruction *currentNode, const llvm::Function *callerFunction, std::map<const llvm::Instruction *, size_t> *instruction_id_map)
	{
		// In the next line we obtain the corresponding row map which maps (given a source node)
		// the target node to the data flow fact map<D, set<D>. In the data flow fact map D is
		// a fact F which holds at the source node whereas set<D> contains the facts that are
		// produced by F and hold at statement TargetNode.
		// Usually every node has one successor node, that is why the row map obtained by row usually
		// only contains just a single entry. BUT: in case of branch statements and other advanced
		// instructions, one statement sometimes has multiple successor statments. In these cases
		// the row map contains entries for every single successor statement. After having obtained
		// the pairs <SourceNode, TargetNode> the data flow map can be obtained easily.
		//size_t from = getJsonRepresentationForInstructionNode(document, currentNode);

		json fromNode = getJsonOfNode(currentNode, instruction_id_map, 0);

		auto TargetNodeMap = this->computedIntraPathEdges.row(currentNode);
		cout << "node pointer current: " << currentNode << endl;

		cout << "TARGET NODE(S)\n";
		for (auto entry : TargetNodeMap)
		{

			auto TargetNode = entry.first;
			//use map to store key value and match node to json id
			json toNode = getJsonOfNode(TargetNode, instruction_id_map, 0);
			cout << "node pointer target : " << TargetNode << endl;

			//getJsonRepresentationForInstructionEdge(from, to, document);
			cout << "NODE (in function " << TargetNode->getFunction()->getName().str() << ")\n";
			TargetNode->dump();

			auto FlowFactMap = entry.second;
			// for (auto FlowFactEntry : FlowFactMap)
			// {
			// 	auto FlowFactAtStart = FlowFactEntry.first;
			// 	auto ProducedFlowFactsAtTarget = FlowFactEntry.second;
			// 	cout << "FLOW FACT AT SourceNode:\n";
			// 	FlowFactAtStart->dump(); // this would be the place for something like 'DtoString()'
			// 	size_t fromData = getJsonRepresentationForFlowFactNode(document, from, &FlowFactAtStart);
			// 	cout << "IS PRODUCING FACTS AT TARGET NODE:\n";
			// 	for (auto ProdFlowFact : ProducedFlowFactsAtTarget)
			// 	{
			// 		size_t toData = getJsonRepresentationForFlowFactNode(document, to, &ProdFlowFact);
			// 		ProdFlowFact->dump(); // this would be the place for something like 'DtoString()'
			// 		getJsonRepresentationForDataFlowEdge(fromData, toData, document);
			// 	}
			// }

			if (this->computedInterPathEdges.containsRow(TargetNode))
			{
				//TODO: actually add interprocedural edges.
				json callSite = getJsonOfNode(TargetNode, instruction_id_map, 1);
				cout << "FOUND Inter path edge !!" << endl;
				auto interEdgeTargetMap = this->computedInterPathEdges.row(TargetNode);

				for (auto interEntry : interEdgeTargetMap)
				{

					//TODO: insert callsite

					//this doesn't seem to work right.. wait for instruction.dump().toString()
					//for easier debugging of the graph
					if (interEntry.first->getFunction()->getName().str().compare(callerFunction->getName().str()) != 0)
					{

						fromNode = getJsonOfNode(TargetNode, instruction_id_map, 0);
						toNode = getJsonOfNode(interEntry.first, instruction_id_map, 0);

						//getJsonRepresentationForInstructionEdge(from, to);

						cout << "NODE (in function (inter)" << interEntry.first->getFunction()->getName().str() << ")\n";
						interEntry.first->dump();

						iterateExplodedSupergraph(interEntry.first, TargetNode->getFunction(), instruction_id_map);
					}
					else
					{
						cout << "FOUND Return Side" << endl;
						//TODO: insert returnsite
						fromNode = getJsonOfNode(TargetNode, instruction_id_map, 0);
						toNode = getJsonOfNode(interEntry.first, instruction_id_map, 0);
						json returnSite = getJsonOfNode(interEntry.first, instruction_id_map, 2);
						//getJsonRepresentationForInstructionEdge(from, to);
					}
				}
			}
		}

		for (auto entry : TargetNodeMap)
		{
			iterateExplodedSupergraph(entry.first, callerFunction, instruction_id_map);
		}
	}

	static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		((std::string *)userp)->append((char *)contents, size * nmemb);
		return size * nmemb;
	}
	CURL *curl;
	int getIdFromWebserver()
	{
		CURLcode res;
		std::string readBuffer;

		curl = curl_easy_init();
		if (curl)
		{
			curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/api/framework/getId");
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

	void sendToWebserver(const char *jsonString)
	{
		if (curl)
		{
			printf("Json String: %s \n", jsonString);
			//setting correct headers so that the server will interpret
			//the post body as json
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
			if (res != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
						curl_easy_strerror(res));
			}
		}
	}
	void exportJSONDataModel()
	{

		curl_global_init(CURL_GLOBAL_ALL);
		int id = getIdFromWebserver();

		/* get a curl handle */
		curl = curl_easy_init();

		ostringstream convert;
		convert << "http://localhost:3000/api/framework/addGraph/" << id;
		const char *url = convert.str().c_str();

		curl_easy_setopt(curl, CURLOPT_URL, url);

		for (auto Seed : this->initialSeeds)
		{
			std::map<const llvm::Instruction *, size_t> instruction_id_map;

			auto SourceNode = Seed.first;

			cout << "START NODE (in function " << SourceNode->getFunction()->getName().str() << ")\n";
			SourceNode->dump();
			cout << " source node name " << SourceNode->getName().str() << endl;
			cout << " source node opcode name" << SourceNode->getOpcodeName() << endl;

			iterateExplodedSupergraph(SourceNode, SourceNode->getFunction(), &instruction_id_map);
		}
	}

	void dumpAllInterPathEdges()
	{
		cout << "COMPUTED INTER PATH EDGES" << endl;
		auto interpe = this->computedInterPathEdges.cellSet();
		for (auto &cell : interpe)
		{
			cout << "FROM" << endl;
			cell.r->dump();
			cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
			cout << "TO" << endl;
			cell.c->dump();
			cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
			cout << "FACTS" << endl;
			for (auto &fact : cell.v)
			{
				cout << "fact" << endl;
				fact.first->dump();
				cout << "produces" << endl;
				for (auto &out : fact.second)
				{
					out->dump();
				}
			}
		}
	}

	void dumpAllIntraPathEdges()
	{
		cout << "COMPUTED INTRA PATH EDGES" << endl;
		auto intrape = this->computedIntraPathEdges.cellSet();
		for (auto &cell : intrape)
		{
			cout << "FROM" << endl;
			cell.r->dump();
			cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
			cout << "TO" << endl;
			cell.c->dump();
			cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
			cout << "FACTS" << endl;
			for (auto &fact : cell.v)
			{
				cout << "fact" << endl;
				fact.first->dump();
				cout << "produces" << endl;
				for (auto &out : fact.second)
				{
					out->dump();
				}
			}
		}
	}

	void exportPATBCJSON()
	{
		cout << "LLVMIFDSSolver::exportPATBCJSON()\n";
	}
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_LLVMIFDSSOLVER_HH_ */

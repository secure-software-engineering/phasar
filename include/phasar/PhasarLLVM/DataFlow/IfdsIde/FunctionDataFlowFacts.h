#include <unordered_map>
#include <set>
#include <string>
#include <variant>

struct DataFlowFact {
    std::variant<int, long, float, double, char, std::string> fact;
};

class FunctionDataFlowFacts {
    private :
        std::unordered_map<std::string, std::unordered_map<int, std::set<DataFlowFact>>> fdff;

        bool getDataFlowFactsOf (std::string funcKey, std::unordered_map<int, std::set<DataFlowFact>> *output) {
            auto const it = this->fdff.find (funcKey);
            if (it == this->fdff.end()) {
                return false;
            }

            else {
                *output = it->second;
                return true;
            }
        }

    public :
        FunctionDataFlowFacts ();

        void insert (std::string funcKey, int index, std::set<DataFlowFact> dff) {
            const auto it = this->fdff.find (funcKey);
            if (it == this->fdff.end()) {
                std::unordered_map<int, std::set<DataFlowFact>> new_facts;
                new_facts.insert ({index, dff});
                this->fdff.insert ({funcKey, new_facts});
            }
            
            else {
                it->second.insert ({index, dff});
            }
        }

         bool getDataFlowFacts (std::string funcKey, int index, std::set<DataFlowFact> *out) {
            std::unordered_map<int, std::set<DataFlowFact>> *output = new std::unordered_map<int, std::set<DataFlowFact>> ();
            if (this->getDataFlowFactsOf (funcKey, output)) {
                auto it = (*output).find (index);
                *out = it->second;
                return true;
            }

            else {
                return false;
            }
        }
};

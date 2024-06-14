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

        const std::unordered_map<int, std::set<DataFlowFact>>& getDataFlowFactsOf (std::string funcKey) const {
            auto const it = this->fdff.find (funcKey);
            if (it == this->fdff.end()) {
                std::unordered_map<int, std::set<DataFlowFact>> ret;
                return ret;
            }

            else {
                return it->second;
            }
        }

        //method to get num of parameters/flow facts
};

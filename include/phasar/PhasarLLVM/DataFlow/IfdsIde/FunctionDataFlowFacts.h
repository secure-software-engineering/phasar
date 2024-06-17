#include <unordered_map>
#include <string>
#include <variant>
#include <vector>

struct DataFlowFactIN {
    std::variant<int, long, float, double, char, std::string> fact;
    int index;
};

struct DataFlowFactOUT {
    std::variant<int, long, float, double, char, std::string> fact;

};

class FunctionDataFlowFacts {
    private :
        std::unordered_map<std::string, std::pair<std::vector<DataFlowFactIN>, std::unordered_map<int, std::vector<DataFlowFactOUT>> *>> fdff;

        bool getDataFlowFactsOf (std::string funcKey, std::unordered_map<int, std::vector<DataFlowFactOUT>> *output) const {
            auto const it = this->fdff.find (funcKey);
            if (it == this->fdff.end()) {
                return false;
            }

            else {
                *output = *(it->second.second);
                return true;
            }
        }

    public :
        FunctionDataFlowFacts ();

        void insert (std::string funcKey, DataFlowFactIN dff_in, std::vector<DataFlowFactOUT> dff_out) {
            const auto it = this->fdff.find (funcKey);
            if (it == this->fdff.end()) {
                //new hash map to store the data flow facts of the out set
                std::unordered_map<int, std::vector<DataFlowFactOUT>> *new_out_facts = new std::unordered_map<int, std::vector<DataFlowFactOUT>>();
                new_out_facts->insert ({dff_in.index, dff_out});
                //new set to store data flow facts for the in set
                std::vector<DataFlowFactIN> new_in_facts = {};
                new_in_facts.push_back (dff_in);
                //pair with data flow facts of the in set and the hash map
                std::pair<std::vector<DataFlowFactIN>, std::unordered_map<int, std::vector<DataFlowFactOUT>> *> new_in_out_facts;
                new_in_out_facts.first = new_in_facts;
                new_in_out_facts.second = new_out_facts;
                //store in and out set
                this->fdff.insert ({funcKey, new_in_out_facts});
            }
            
            else {
                it->second.first.push_back (dff_in);
                it->second.second->insert ({dff_in.index, dff_out});
                //it->second.insert ({index, dff_out});
            }
        }

         bool getDataFlowFacts (std::string funcKey, DataFlowFactIN dff_in, std::vector<DataFlowFactOUT> *out) const {
            std::unordered_map<int, std::vector<DataFlowFactOUT>> *output = new std::unordered_map<int, std::vector<DataFlowFactOUT>> ();
            if (this->getDataFlowFactsOf (funcKey, output)) {
                auto it = (*output).find (dff_in.index);
                *out = it->second;
                return true;
            }

            else {
                return false;
            }
        }
};

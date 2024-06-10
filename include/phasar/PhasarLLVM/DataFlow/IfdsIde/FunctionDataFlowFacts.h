#include <unordered_map>
#include <set>
#include <string>
#include <iterator>

using namespace std;

class FunctionDataFlowFacts {
    private :
        unordered_map<string, set<string>> fdff;

    public :
        FunctionDataFlowFacts ();

        void insert (string funcKey, set<string> dff) {
            this->fdff.insert ({funcKey, dff});
        }

        void remove (string funcKey) {
            this->fdff.erase (funcKey);
        }
        set<string> getDataFlowFacts (string funcKey) {
            return this->fdff.find (funcKey)->second;
        }
};

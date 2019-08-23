#include <CrySLTypechecker.h>
#include <ErrorHelper.h>
#include <TypeParser.h>
#include <Types/Type.h>
#include <iostream>

namespace CCPP {
//using namespace std;

struct OrderTypeChecker{
    const list<Type> &DefinedObjects;
 
bool CCPP::CrySLTypechecker::checkEvent(CrySLParser::PrimaryContext *primaryContext){
    bool result =true;
    if(!DefinedObjects.count(primaryContext->eventName-getText()) 
                    std::cerr << Position(primaryContext) << ": event is not defined in EVENTS section"<<std::endl;
                    return false;
    return result;
}

bool CCPP:: CrySLTypeChecker::checkBound(){

}

bool OrderTypeChecker::typecheck (CrySLParser::OrderContext *order){
    bool result =true;
    for(auto ord : order->orderSequence()){
        for (auto simord : ord->simpleOrder()){
            for(auto unordsym : simord->unorderdSymbols()){
                for(auto primcon: unordsym->primary()){
                    result = checkEvent(primcon);
                }
            }
        }
    }
}

    return result;
}
}
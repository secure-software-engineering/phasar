#include <CrySLTypechecker.h>
#include <ErrorHelper.h>
#include <TypeParser.h>
#include <Types/Type.h>
#include <iostream>

namespace CCPP {
// using namespace std;

bool checkEventAndSymbols(
    CrySLParser::PrimaryContext *primaryContext,
    std::unordered_map<std::string, std::shared_ptr<Type>> &DefinedObjects) {
  bool result = true;
  if (!DefinedObjects.count(primaryContext->eventName->getText())) {
    std::cerr << Position(primaryContext)
              << ": event is not defined in EVENTS section" << std::endl;
    result=false;
    return result;
  }

  if(primaryContext-> eventTop->getText() == '+' || 
      primaryContext-> eventTop->getText() == '*' || 
      primaryContext-> eventTop->getText() == '?'){
          std:out <<Position(primaryContext)
              << ": symbol is defined" << std::endl;
        }
  else{   std::cerr << Position(primaryContext)
              << ": symbol is not defined" << std::endl;
              result=false;
        }
    return result;
}

book checkBound(CrySLParser::UnorderedSymbolsContext *unorderedSymbolsContext,
    std::unordered_map<std::string, std::shared_ptr<Type>> &DefinedObjects){
        bool result=true;

        if!(unorderedSymbolsContext->lower->Ident()){
            
        }
         


        return result;
    }

bool checkSymbols(
    CrySLParser::PrimaryContext *primaryContext,
    std::unordered_map<std::string, std::shared_ptr<Type>> &DefinedObjects) {
   bool result= true;
   if(!DefinedObject.count(primaryContext->elementTop))     

}

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::OrderContext *order) {
  bool result = true;
  for (auto simpleOrder : order->orderSequence()->simpleOrder()) {
    for (auto unorderedSymbolsContext : simpleOrder->unorderedSymbols()) {
      for (auto primaryContext : unorderedSymbolsContext->primary()) {
        result &= checkEventAndSymbols(primaryContext, DefinedObjects);
      }
    }
  }

  return result;
}

} // namespace CCPP
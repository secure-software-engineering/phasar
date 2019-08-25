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
  
  if (!DefinedObjects.count(primaryContext->eventName->getText())) {
    std::cerr << Position(primaryContext)
              << ": event is not defined in EVENTS section" << std::endl;
    return false;
  }

  if(primaryContext-> eventTop->getText() == '+' || 
      primaryContext-> eventTop->getText() == '*' || 
      primaryContext-> eventTop->getText() == '?'){
          std:out <<Position(primaryContext)
              << ": symbol is defined" << std::endl;
        }
  else{   std::cerr << Position(primaryContext)
              << ": symbol is not defined" << std::endl;
              return false;
        }
    return true;
}

book checkBound(CrySLParser::UnorderedSymbolsContext *unorderedSymbolsContext,
    std::unordered_map<std::string, std::shared_ptr<Type>> &DefinedObjects){
    for (auto lowerIdent : unorderedSymbolsContext->lower->Ident()){

          if (!DefinedObjects.count(lowerIdent->getText()))
					{
						std::cerr << Position(lowerIdent) << ": is not defined for this section" << std::endl;
						return false;
					}
            
        }

        for (auto upperIdent : unorderedSymbolsContext->upper->Ident()){

          if (!DefinedObjects.count(upperIdent->getText()))
					{
						std::cerr << Position(upperIdent) << ": is not defined for this section" << std::endl;
						return false;
					}
            
        }
       return true;
    }

bool checkSymbols(
    CrySLParser::PrimaryContext *primaryContext,
    std::unordered_map<std::string, std::shared_ptr<Type>> &DefinedObjects) {
   bool result= true;
   if(!DefinedObject.count(primaryContext->elementTop))     

}

bool isEventCalled(
  CrySLParser::PrimaryContext *primaryContext ,
   std::unordered_set<std::string> &DefinedEvents){
     if (!DefinedEvents.count(primaryContext->eventName->getText())) {
      std::cerr << Position(primaryContext)
              << ": event is defined in EVENTS section but not called in ORDER section" << std::endl;
      return false;
     }
   }

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::OrderContext *order) {
  bool result = true;
  for (auto simpleOrder : order->orderSequence()->simpleOrder()) {
    for (auto unorderedSymbolsContext : simpleOrder->unorderedSymbols()) {
      bool result1 &= checkBound(unorderedSymbolsContext, DefinedObjects);
      for (auto primaryContext : unorderedSymbolsContext->primary()) {
        bool result2 &= checkEventAndSymbols(primaryContext, DefinedObjects);
      }
    }
  }
  result = result1 && result2;
  return result;
}

} // namespace CCPP
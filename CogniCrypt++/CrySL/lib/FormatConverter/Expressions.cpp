#include <FormatConverter/Expressions.h>
#include <FormatConverter/ConstraintsTypeChecker.h>
#include <iostream>

namespace CCPP {
//Expression::formatconverter(CrySLTypechecker::typecheck(CrySLParser::ConstraintsContext *constraintctx) {
	
  //CrySLTypechecker(ccr->cons()->literalExpr()->memberAccess()->Ident);
  

	
  Expression::formatconverter( CrySLParser::ConstraintsContext *constraintctx) {

  
    
    for (auto ccr : constraintctx->constr())
      // CrySLParserParamContext(constraintctx)
    //temp = typ[0];
  {
	  
    if (ccr->constr.plus) {
        //if (ccr->constr.size)

        //if (ccr->constr.s)
        
        
      //ccr->cons()->literalExpr()->memberAccess()->Ident();
      AddExpression addobjct;
      addobjct.evaluate(constraintctx);
      addobjct.getType(constraintctx);

	  
     
    }
   

     else if (ccr->comparingRelOperator()) {

    }
  }
    Object::TypeT AddExpression::getType(){ if ()
    
  }
  AddExpression::unique_ptr<Object> evaluate() { 
	  if ()
  }
 
  }
  
 // namespace CCPP
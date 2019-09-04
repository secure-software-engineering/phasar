#include "CrySLTypechecker.h"
#include <Event.h>
#include <EventConverter.h>
#include <iostream>

namespace CCPP {

Object::Object(CrySLParser::ParamContext *param) {
    if(param->wildCard)
        this->name = "wildCard";
    else if(param->thisPtr)
        this->name = "thisPtr";
    else{
        for(auto paramName : param->memberAccess()->Ident())
        {
            this->name = paramName->getText();
        }
    }
}

std::shared_ptr<CCPP::Types::Type> Object::getType() {

}
} // namespace CCPP
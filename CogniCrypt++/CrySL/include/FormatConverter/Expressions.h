#pragma once
//#include <Parser/CrySLTypechecker.h>
#include "Object.h"
#include <memory>

namespace CCPP {

	class Expression
	{
	public:
        virtual ~Expression() = default;
		virtual std::unique_ptr<Object> evaluate() = 0;
		virtual Object::TypeT getType() = 0
	};
	class DerefExpression : public Expression
	{
        virtual std::unique_ptr<Object> evaluate()override;
		virtual Object::TypeT getType()override;
	};

}
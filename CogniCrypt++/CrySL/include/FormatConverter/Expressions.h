#pragma once
#include "CrySLTypechecker.h"
#include "TypeParser.h"
#include "PositionHelper.h"
#include <Object.h>
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
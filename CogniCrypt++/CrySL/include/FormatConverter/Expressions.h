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
		std::vector<Expression> formatconverter(CrySLParser::ConstraintsContext *constraintctx);
	};
	class DerefExpression : public Expression
	{
        virtual std::unique_ptr<Object> evaluate()override;
		virtual Object::TypeT getType()override;
	};
	class AddExpression : public Expression {
  AddExpression : AddExpression();
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class SubExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class MullExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class DivExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class NotExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class ModExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class EqualExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class NotEqualExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class LessExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class lLessorEqualExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class GreaterorequalExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};
class GreaterExpression : public Expression {
  virtual std::uni_ptr<object> evaluate() override;
  virtual Object::TypeT getType() = 0
};

}

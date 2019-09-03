#pragma once
#include "CrySLTypechecker.h"
#include "TypeParser.h"
#include "PositionHelper.h"

namespace CCPP {
	using namespace std;

	class public Expression
	{
	public:
		virtual objectconverter calculate();
		virtual  types getType();



	};
	class  public DerefExpression : Expression
	{

	};

}
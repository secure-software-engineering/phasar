parser grammar CrySL;
domainModel:
	spec objects forbidden? events order constraints? requires? ensures? negates? EOF;

spec: 'SPEC' qualifiedName;
qualifiedName: Ident ('::' Ident)*;

objects: 'OBJECTS' objectDecl*;
objectDecl: typeName Ident array* ';';
typeName: qualifiedName (pointer = '*')*;
array: '[' Int ']';

requires: 'REQUIRES' (reqPred ';')*;
reqPred:
	reqPredLit
	| reqPred and = ',' reqPred
	| reqPred or = '||' reqPred;
reqPredLit: (constraint '=>')? (not = '!')? pred;
pred: name = Ident '[' (paramList = suParList)? ']';
suParList: suPar (',' suPar)*;
suPar: value = consPred | thisptr = 'this' | wildcard = '_';
consPred: //TODO: predefined functions
	literalExpr;
literalExpr: literal | memberAccess | preDefinedPredicate;
literal:
	Int
	| base = Int pow = '^' exp = Int
	| boolean = 'true'
	| boolean = 'false'
	| String;

memberAccess: Ident | deref = '*' Ident | Ident dot = '.' Ident;
preDefinedPredicate:
	name = 'neverTypeOf' '[' obj = memberAccess ',' type = typeName ']'
	| name = 'noCallTo' '[' evt = Ident ']'
	| name = 'callTo' '[' evt = Ident ']'
	| name = 'notHardCoded' '[' obj = memberAccess ']'
	| name = 'length' '[' obj = memberAccess ']';
parser grammar CrySL;
domainModel:
	spec objects forbidden? events order constraints? requires? ensures? negates? EOF;

spec: 'SPEC' qualifiedName;
qualifiedName: Ident ('::' Ident)*;

objects: // allow now zero objects too
	'OBJECTS' objectDecl*;
objectDecl: typeName Ident array* ';';
typeName: qualifiedName (pointer = '*')*;
// C-style arrays
array: '[' Int ']';

requires: 'REQUIRES' (reqPred ';')*;
reqPred:
	// a normal predicate
	(not = '!')? reqPredLit
	// a goal consisting of a conjunction of dependent predicates
	| reqPred and = ',' reqPred
	// a clause consisting of a disjunction of alternative prediactes
	| reqPred or = '||' reqPred
	// a more general form of constrained predicate requirement
	| constraint implication = '=>' reqPred;
reqPredLit: pred;
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

memberAccess:
	Ident // an object
	| deref = '*' Ident // dereferencing a pointer
	| Ident dot = '.' Ident // actual member-access
	| Ident arrow = '->' Ident;
preDefinedPredicate:
	name = 'neverTypeOf' '[' obj = memberAccess ',' type = typeName ']'
	| name = 'noCallTo' '[' evt = Ident ']'
	| name = 'callTo' '[' evt = Ident ']'
	| name = 'notHardCoded' '[' obj = memberAccess ']'
	| name = 'length' '[' obj = memberAccess ']';
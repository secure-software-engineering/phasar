grammar CrySL;

//==========================================================================
// PARSER GRAMMAR
// 
// =========================================================================

domainModel:
	spec objects forbidden? events order constraints? requiresBlock? ensures? negates? EOF;

spec: 'SPEC' qualifiedName;
qualifiedName: Ident ('::' Ident)*;

objects: // allow now zero objects too
	'OBJECTS' objectDecl*;
objectDecl: constModifier = 'const'? typeName Ident array* ';';
primitiveTypeName:
	booleanType = 'bool'
	| unsignedInt = 'unsigned'? (
		charTy = 'char'
		| shortTy = 'short'
		| intTy = 'int'
		| longTy = 'long'
		| longlongTy = 'long' 'long'
	)
	| floatingPoint = 'float'
	| longDouble = 'long'? doubleFloat = 'double'
	| sizeType = 'size_t';
typeName: (qualifiedName | primitiveTypeName) ( pointer = ptr)*;
ptr: '*';
// C-style arrays
array: '[' Int ']';

requiresBlock: 'REQUIRES' (reqPred ';')*;
reqPred:
	// a normal predicate
	(neg = '!')? reqPredLit
	// a goal consisting of a conjunction of dependent predicates
	| reqPred seq = ',' reqPred
	// a clause consisting of a disjunction of alternative prediactes
	| reqPred alt = '||' reqPred
	// a more general form of constrained predicate requirement
	| constr implication = '=>' reqPred
	| <assoc = right> reqPred implication = '=>' reqPred;
reqPredLit: pred;
pred: name = Ident '[' (paramList = suParList)? ']';
suParList: suPar (',' suPar)*;
suPar: value = consPred | thisptr = 'this' | wildcard = '_';
consPred: //TODO: predefined functions
	literalExpr;
literalExpr: literal | memberAccess | preDefinedPredicate;
literal: Int | base = Int pow = '^' exp = Int | Bool | String;

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

ensures: 'ENSURES' (ensPred ';')+;
ensPred: (constr '=>')? pred ('after' state = Ident)?;

constraints: 'CONSTRAINTS' (constr ';')+;
constr:
	| '(' constr ')'
	| cons
	| lnot = '!' constr
	| constr (mul = '*' | div = '/') constr
	| constr mod = '%' constr
	| constr (plus = '+' | minus = '-') constr
	| constr comparingRelOperator constr
	| constr (equal = '==' | unequal = '!=') constr
	| constr land = '&&' constr
	| constr lor = '||' constr
	| <assoc = right>constr implies = '=>' constr;
comparingRelOperator:
	less = '<'
	| less_or_equal = '<='
	| greater_or_equal = '>='
	| greater = '>';
cons: arrayElements 'in' '{' litList '}' | literalExpr;
arrayElements: (el = 'elements' '(' consPred ')') | consPred;
litList: literal (',' (literal | ellipsis = '...'))*;

//Events section starts
events: 'EVENTS' (eventsOccurence | eventAggregate)+;
eventsOccurence:
	eventName = Ident ':' (
		(returnValue = Ident | returnThis = 'this') '='
	)? methodName = Ident '(' parametersList? ')' ';';
parametersList: param (',' param)*;
param: memberAccess | thisPtr = 'this' | wildCard = '_';

eventAggregate: eventName = Ident ':=' agg ';';
agg: Ident ('|' Ident)+;

//Events section ends

//Order section starts

order: 'ORDER' orderSequence;
orderSequence: simpleOrder (',' simpleOrder)*;
simpleOrder: unorderedSymbols ('|' unorderedSymbols)*;
unorderedSymbols:
	primary (
		('~' primary)+ (
			(lower = Int)? bound = '#' (upper = Int)?
		)?
	)?;
primary:
	eventName = Ident elementop = ('+' | '?' | '*')?
	| ('(' orderSequence ')' elementop = ('+' | '?' | '*')?);

//Order section ends

negates: 'NEGATES' negatesOccurence+;
negatesOccurence: ensPred;

//negates section ends

//forbidden section starts

forbidden: 'FORBIDDEN' forbiddenOccurence+;
forbiddenOccurence:
	methodName = fqn ('=>' eventName = Ident)? ';';
fqn: qualifiedName '(' typeNameList? ')';
typeNameList: typeName (',' typeName)*;

//==========================================================================
// LEXER GRAMMAR
// 
// =========================================================================
Int: ([1-9][0-9_]*) | '0';
Double: Int '.' Int;
Char: '\'' '\\'? . '\'';
Bool: 'true' | 'false';
String: '"' (~["\\] | '\\"')*? '"';
Ident: [a-zA-Z_]([a-zA-Z0-9][a-zA-Z0-9_]*)?;

COMMENT: '/*' .*? '*/' -> skip;

LINE_COMMENT: '//' ~[\r\n]* -> skip;

WS: [ \t\r\n] -> skip;
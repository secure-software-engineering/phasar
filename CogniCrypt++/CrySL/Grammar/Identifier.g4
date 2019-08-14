lexer grammer ident;

WS:			[ \t\r\n] -> skip;
INT:		[0-9]+;
FLOAT:		INT'.'INT;
DOUBLE:		INT'.'INT;
CHAR:		['].|WS['];
BOOL:		[true|flase];
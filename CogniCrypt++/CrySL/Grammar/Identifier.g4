lexer grammar ident;

Int:		[1-9][0-9_]*;
Float:		INT '.' INT;
Double:		INT '.' INT;
Char:		'\'' . '\'';
Bool:		'true' | 'false';
Ident:      [a-zA-Z_]([a-zA-Z0-9][a-zA-Z0-9_]*)?
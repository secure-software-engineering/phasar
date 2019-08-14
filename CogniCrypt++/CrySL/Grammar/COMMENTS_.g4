lexer gammer comment;


WS     			: [ \t\r\n]+ -> skip;

COMMENT    		: '/*' .*? '*/' -> skip;

LINE_COMMENT    : '//' ~[\r\n]* -> skip;
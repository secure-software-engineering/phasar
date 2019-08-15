lexer grammar comment;




COMMENT    		: '/*' .*? '*/' -> skip;

LINE_COMMENT    : '//' ~[\r\n]* -> skip;

WS     			: [ \t\r\n] -> skip;
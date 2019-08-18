
//contraint section starts

constraints: 'CONSTRAINTS' constraintOccurance+;
constraintOccurance: constraintName = Ident 'mathematicalExpression' Ident*';' ;
mathematicalExpressions: '>' | '=' | '>=' |'<' | '<=' ;


// ensure section starts

ensure: 'ENSURES' ensureOccurance+;
ensureOccurance: ensureName = Ident '[' parametersList? ']' 'After?' 'eventname = Ident	';' ;
parametersList: param (',' param)*;
param: memberAccess | thisPtr = 'this' | wildCard = '_';



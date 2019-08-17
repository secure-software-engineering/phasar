//Negates section starts

negates: 'NEGATES' negatesOccurence+;
negatesOccurence: negatesName   methodName = Ident '(' parametersList? ')' ';' ;
parametersList: param (',' param)*;
param: memberAccess ';

//negates section ends


//forbidden section starts

forbidden: 'FORBIDDEN' forbiddenOccurence+;
forbiddenOccurence: forbiddenName   methodName = Ident '(' parametersList? ')' ';' ;
parametersList: param (',' param)*;
param: memberAccess ';

//forbidden section ends

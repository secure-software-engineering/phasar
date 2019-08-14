
//Events section starts

events: 'EVENTS' eventsOccurence+;
eventsOccurence: eventName = Ident ':' methodName = Ident '(' parametersList? ')' ';' ;
parametersList: param (',' param)*;
param: memberAccess | thisPtr = 'this' | wildCard = '_';

//Events section ends

//Order section starts

order: 'ORDER' orderSequence;
orderSequence: simpleOrder (',' simpleOrder)*;
simpleOrder: unorderdSymbols ('|' unorderdSymbols)*;
unorderdSymbols: primary (('~' primary)+ ((lower=Int)?'#'(upper=Int)?)?)?;
primary: eventName=Ident elementop=('+' | '?' | '*')? | ('(' orderSequence ')' elementop=('+' | '?' | '*')?);

//Order section ends
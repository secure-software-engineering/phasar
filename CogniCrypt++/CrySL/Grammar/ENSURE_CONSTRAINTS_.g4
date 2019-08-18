//Ensure section starts

ensures : 'ENSURES' ensuresOccurence+;
ensuresOccurence : ensuresName = Ident '['objectLists? ']' ';' ;
objectLists : param (',' param)*;
param : memberAccess;

//Ensure section ends

//Constraints section starts 

constaints : 'CONSTRAINTS' constaintsOccurence+;
constaintsOccurence : constaintsName = Ident 'in' '{' objectsLists?'}' | '=>' 'callTo' | 'noCallTo' '[' objectsLists? ']' ';' ;
objectLists : param (',' param)*;
param : memberAccess;

//Constraints section ends 
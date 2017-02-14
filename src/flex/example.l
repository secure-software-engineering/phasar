/**
 *
 * @author Philipp D. Schubert
 * @bug no bugs known
 */

%option noyywrap nodefault case-insensitive yylineno

%{


string yy_current_file = "";
string yy_head = "";
string yy_chunks = "";
size_t yy_brackets = 0;
MyFile yy_myfile;
MyFunction yy_global_func;

%}

%x IFUNCTION
%x ISTRUCT
%x IUNION
%x IENUM

%%

 /* pattern for struct */
"typedef"?[ \t\n]*"struct"[ \t\n]+[a-z_0-9]+	 	{ 
							yy_head += yytext;
							BEGIN ISTRUCT;


%%

/*
 * no main method is provided, since this lexer should be called trough the FlexExtraction.hh / FlexExtraction.cpp
 */

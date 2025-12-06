%code requires {
  #include <memory>
  #include <string>
// code in this will be inserted into sysy.lex.hpp
}

%{
#include <memory>
#include <iostream>
#include <string>

int yylex();
void yyerror(std::unique_ptr<std::string> &ast, const char *str);

// code in this will be inserted into sysy.lex.cpp
%}

// actually, yyparser don't have any paramters by default, you need to reload it.
%parse-param { std::unique_ptr<std::string> &ast }

%union {
  std::string *str_val;
  int int_val;
}

// terminal letters are written in uppercase.
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <str_val> FuncDef FuncType Block Stmt Number

%%
CompUnit
  : FuncDef {
    ast = std::unique_ptr<std::string>($1);
  };

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto type = std::unique_ptr<std::string>($1);
    auto ident = std::unique_ptr<std::string>($2);
    auto block = std::unique_ptr<std::string>($5);
    $$ = new std::string(*type + " " + *ident + "() " + *block);
  };

FuncType
  : INT {
    $$ = new std::string("int");
  };

Block
  : '{' Stmt '}' {
    auto stmt = std::unique_ptr<std::string>($2);
    $$ = new std::string("{ " + *stmt + " }");
  };

Stmt
  : RETURN Number ';' {
    auto number = std::unique_ptr<std::string>($2);
    $$ = new std::string("return " + *number + ";");
  };

Number
  : INT_CONST {
    $$ = new std::string(std::to_string($1));
  };

%%

void yyerror(std::unique_ptr<std::string> &ast, const char *str) {
  std::cerr << "error: " << str << std::endl;
}
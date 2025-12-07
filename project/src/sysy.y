// src/sysy.y
%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
// code in this will be inserted into sysy.lex.hpp
}

%{
#include <memory>
#include <string>
#include "ast.hpp"
#include <fmt/core.h>

int yylex();
void yyerror(std::unique_ptr<ast::BaseAST> &ast, const char *str);

// code in this will be inserted into sysy.lex.cpp
%}

// actually, yyparser don't have any paramters by default, you need to reload it.
%parse-param { std::unique_ptr<ast::BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  ast::BaseAST *ast_val;
}

// terminal letters are written in uppercase.
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block Stmt Number

%%
CompUnit
  : FuncDef {
    auto comp_unit = std::make_unique<ast::CompUnitAST>();
    comp_unit->func_def = std::unique_ptr<ast::BaseAST>($1);
    ast = std::move(comp_unit);
  };

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new ast::FuncDefAST();
    ast->func_type = std::unique_ptr<ast::BaseAST>($1);
    ast->ident = *std::unique_ptr<std::string>($2);
    ast->block = std::unique_ptr<ast::BaseAST>($5);
    $$ = ast;
  };

FuncType
  : INT {
    auto ast = new ast::FuncTypeAST();
    ast->type = "int";
    $$ = ast;
  };

Block
  : '{' Stmt '}' {
    // auto stmt = std::unique_ptr<std::string>($2);
    // $$ = new std::string("{ " + *stmt + " }");
    auto ast = new ast::BlockAST();
    ast->stmt = std::unique_ptr<ast::BaseAST>($2);
    $$ = ast;
  };

Stmt
  : RETURN Number ';' {
    // auto number = std::unique_ptr<std::string>($2);
    // $$ = new std::string("return " + *number + ";");
    auto ast = new ast::StmtAST();
    ast->number = std::unique_ptr<ast::BaseAST>($2);
    $$ = ast;
  };

Number
  : INT_CONST {
    // $$ = new std::string(std::to_string($1));
    auto ast = new ast::NumberAST();
    ast->num = $1;
    $$ = ast;
  };

%%

void yyerror(std::unique_ptr<ast::BaseAST> &ast, const char *str) {
  fmt::println(stderr, "{}", str);
}
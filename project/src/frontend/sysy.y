// src/sysy.y
%code requires {
  #include <memory>
  #include <string>
  // #include <vector>
  #include "ir/ast.hpp"
// code in this will be inserted into sysy.lex.hpp
}

%{
#include <memory>
#include <string>
#include <vector>
#include "ir/ast.hpp"
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
  std::vector<std::unique_ptr<ast::BaseAST>> *items_val;
  std::vector<std::unique_ptr<ast::DefAST>> *defs_val;
}

/**
Decl          ::= ConstDecl;
ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
BType         ::= "int";
ConstDef      ::= IDENT "=" ConstInitVal;
ConstInitVal  ::= ConstExp;

Block         ::= "{" {BlockItem} "}";
BlockItem     ::= Decl | Stmt;

LVal          ::= IDENT;
PrimaryExp    ::= "(" Exp ")" | LVal | Number;

ConstExp      ::= Exp;
 */

// terminal letters are written in uppercase.
%token VOID INT RETURN OR AND EQ NE LE GE PRIORITY CONST IF ELSE
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <str_val> Btype
%type <ast_val> FuncDef Block Stmt Number Expr LVal
%type <ast_val> ConstDef VarDef BlockItem Decl ConstDecl VarDecl
%type <items_val> BlockItemList
%type <defs_val> VarDefList ConstDefList

%nonassoc LOWER_ELSE
%nonassoc ELSE

%left OR                  // ||
%left AND                 // &&
%left EQ NE               // == , !=
%left '<' '>' LE GE       // < > <= >=
%left '+' '-'             // + -
%left '*' '/' '%'         // * / %
%right '!' PRIORITY          // ! -

%%
CompUnit
  : FuncDef {
    auto comp_unit = std::make_unique<ast::CompUnitAST>();
    comp_unit->func_def = std::unique_ptr<ast::BaseAST>($1);
    ast = std::move(comp_unit);
  };

FuncDef
  : Btype IDENT '(' ')' Block {
    auto ast = new ast::FuncTypeAST(std::move(*$1));
    $$ = new ast::FuncDefAST(ast, std::move(*$2), $5);
    delete $1;
    delete $2;
  };

Block
  : '{' BlockItemList '}' {
    $$ = new ast::BlockAST($2);
  };

BlockItemList
  : {
    $$ = new std::vector<std::unique_ptr<ast::BaseAST>>();
  }
  | BlockItemList BlockItem {
    $$ = $1;
    $$->push_back(std::unique_ptr<ast::BaseAST>($2));
  };

BlockItem
  : Decl { $$ = $1; }
  | Stmt { $$ = $1; };

Decl 
  : ConstDecl { $$ = $1; }
  | VarDecl   { $$ = $1; };

ConstDecl
  : CONST Btype ConstDefList ';' {
    $$ = new ast::DeclAST(true, std::move(*$2), $3);
    delete $2;
  };

ConstDefList
  : ConstDef {
    $$ = new std::vector<std::unique_ptr<ast::DefAST>>();
    $$->push_back(std::unique_ptr<ast::DefAST>(static_cast<ast::DefAST *>($1)));
  }
  | ConstDefList ',' ConstDef {
    $$ = $1;
    $$->push_back(std::unique_ptr<ast::DefAST>(static_cast<ast::DefAST *>($3)));
  };

ConstDef 
  : IDENT '=' Expr {
    $$ = new ast::DefAST(true, std::move(*$1), $3);
    delete $1;
  };

VarDecl
  : Btype VarDefList ';' {
    $$ = new ast::DeclAST(false, std::move(*$1), $2);
    delete $1;
  }

VarDefList
  : VarDef {
    $$ = new std::vector<std::unique_ptr<ast::DefAST>>();
    $$->push_back(std::unique_ptr<ast::DefAST>(static_cast<ast::DefAST *>($1)));
  };
  | VarDefList ',' VarDef {
    $$ = $1;
    $$->push_back(std::unique_ptr<ast::DefAST>(static_cast<ast::DefAST *>($3)));
  };

VarDef 
  : IDENT '=' Expr {
    $$ = new ast::DefAST(false, std::move(*$1), $3);
    delete $1;
  };
  | IDENT {
    $$ = new ast::DefAST(false, std::move(*$1), nullptr);
    delete $1;
  };

Btype
  : INT { $$ = new std::string("int"); }
  | VOID { $$ = new std::string("void"); };

Stmt
  : LVal '=' Expr ';' {
    $$ = new ast::AssignStmtAST($1, $3);
  }
  | Block {
    $$ = $1;
  }
  | Expr ';' {
    $$ = new ast::ExprStmtAST($1);
  }
  | ';' {
    $$ = new ast::ExprStmtAST(nullptr);
  }
  | RETURN Expr ';' {
    $$ = new ast::ReturnStmtAST($2);
  }
  | RETURN ';' {
    $$ = new ast::ReturnStmtAST(nullptr);
  }
  | IF '(' Expr ')' Stmt %prec LOWER_ELSE {
    $$ = new ast::IfStmtAST($3, $5, nullptr);
  }
  | IF '(' Expr ')' Stmt ELSE Stmt {
    $$ = new ast::IfStmtAST($3, $5, $7);
  };

Expr
  : Number { $$ = $1; }
  | '(' Expr ')' { $$ = $2; }
  | LVal { $$ = $1; }
  /* unary experssion */
  | '!' Expr {
    $$ = new ast::UnaryExprAST(ast::UnaryOp::Not, $2);
  }
  | '+' Expr %prec PRIORITY{
    $$ = $2;
  }
  | '-' Expr %prec PRIORITY {
    $$ = new ast::UnaryExprAST(ast::UnaryOp::Neg, $2);
  }
  /* binary experssion */
  | Expr '+' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Add, $1, $3);
  }
  | Expr '-' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Sub, $1, $3);
  }
  | Expr '*' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Mul, $1, $3);
  }
  | Expr '/' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Div, $1, $3);
  }
  | Expr '<' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Lt, $1, $3);
  }
  | Expr '>' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Gt, $1, $3);
  }
  | Expr '%' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Mod, $1, $3);
  }
  | Expr LE Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Le, $1, $3);
  }
  | Expr GE Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Ge, $1, $3);
  }
  | Expr EQ Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Eq, $1, $3);
  }
  | Expr NE Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Ne, $1, $3);
  }
  | Expr AND Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::And, $1, $3);
  }
  | Expr OR Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Or, $1, $3);
  };

Number
  : INT_CONST {
    $$ = new ast::NumberAST($1);
  };

LVal 
  : IDENT {
    $$ = new ast::LValAST(std::move(*$1));
    delete $1;
  };

%%

void yyerror(std::unique_ptr<ast::BaseAST> &ast, const char *str) {
  fmt::println(stderr, "{}", str);
  if(ast) ast->dump(0);
}
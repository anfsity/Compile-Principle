// src/sysy.y
%code requires {
  #include <memory>
  #include <string>
  #include "ir/ast.hpp"
// code in this will be inserted into sysy.lex.hpp
}

%{
#include <memory>
#include <string>
#include <vector>
#include "ir/ast.hpp"
#include <fmt/core.h>

using namespace ast;
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *str);

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
  std::vector<std::unique_ptr<ast::BaseAST>> *children_val;
  std::vector<std::unique_ptr<ast::FuncParamAST>> *funcParams_val;
  std::vector<std::unique_ptr<ast::ExprAST>> *args_val;
}

/*
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
%token VOID INT RETURN OR AND EQ NE LE GE PRIORITY 
%token CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <str_val> Btype
%type <ast_val> FuncDef Block Stmt Number Expr LVal
%type <ast_val> ConstDef VarDef BlockItem Decl ConstDecl VarDecl
%type <ast_val> CompUnitItem CompUnit FuncFParam
%type <funcParams_val> FuncFParams
%type <args_val> FuncRParams
%type <children_val> CompUnitItemList
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
  : CompUnitItemList {
    auto comp_unit = new CompUnitAST(std::move(*$1));
    ast = std::unique_ptr<BaseAST>(static_cast<BaseAST *>(comp_unit));
    delete $1;
  };

CompUnitItemList
  : CompUnitItem {
    // unique_ptr automatic takeover of ownership of $1
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
    $$->push_back(std::unique_ptr<BaseAST>($1));
  }
  | CompUnitItemList CompUnitItem {
    $$ = $1;
    $$->push_back(std::unique_ptr<BaseAST>($2));
  };

CompUnitItem 
  : Decl {
    $$ = $1;
  }
  | FuncDef {
    $$ = $1;
  };

FuncDef
  : Btype IDENT '(' ')' Block {
    auto params = std::vector<std::unique_ptr<FuncParamAST>>();
    $$ = new FuncDefAST(std::move(*$1), std::move(*$2), std::move(params), $5);
    delete $1;
    delete $2;
  }
  | Btype IDENT '(' FuncFParams ')' Block {
    $$ = new FuncDefAST(std::move(*$1), std::move(*$2), std::move(*$4), $6);
    delete $1;
    delete $2;
    delete $4;
  };

FuncFParams 
  : FuncFParam {
    $$ = new std::vector<std::unique_ptr<FuncParamAST>>();
    $$->push_back(std::unique_ptr<FuncParamAST>(static_cast<FuncParamAST *>($1)));
  }
  | FuncFParams ',' FuncFParam {
    $$ = $1;
    $$->push_back(std::unique_ptr<FuncParamAST>(static_cast<FuncParamAST *>($3)));
  };

FuncFParam
  : Btype IDENT {
    $$ = new FuncParamAST(std::move(*$1), std::move(*$2), false);
    delete $1;
    delete $2;
  }
  | CONST Btype IDENT {
    $$ = new FuncParamAST(std::move(*$2), std::move(*$3), true);
    delete $2;
    delete $3;
  }


Block
  : '{' BlockItemList '}' {
    $$ = new BlockAST(std::move(*$2));
    delete $2;
  };

/* block item list can be empty */
BlockItemList
  : {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  | BlockItemList BlockItem {
    $$ = $1;
    $$->push_back(std::unique_ptr<BaseAST>($2));
  };

BlockItem
  : Decl { $$ = $1; }
  | Stmt { $$ = $1; };

Decl 
  : ConstDecl { $$ = $1; }
  | VarDecl   { $$ = $1; };

ConstDecl
  : CONST Btype ConstDefList ';' {
    $$ = new DeclAST(true, std::move(*$2), std::move(*$3));
    delete $2;
    delete $3;
  };

ConstDefList
  : ConstDef {
    $$ = new std::vector<std::unique_ptr<DefAST>>();
    // this is a pointer , so we don't need move optimize.
    $$->push_back(std::unique_ptr<DefAST>(static_cast<DefAST *>($1)));
  }
  | ConstDefList ',' ConstDef {
    $$ = $1;
    $$->push_back(std::unique_ptr<DefAST>(static_cast<DefAST *>($3)));
  };

ConstDef 
  : IDENT '=' Expr {
    $$ = new DefAST(true, std::move(*$1), $3);
    delete $1;
  };

VarDecl
  : Btype VarDefList ';' {
    $$ = new DeclAST(false, std::move(*$1), std::move(*$2));
    delete $1;
    delete $2;
  };

VarDefList
  : VarDef {
    $$ = new std::vector<std::unique_ptr<DefAST>>();
    $$->push_back(std::unique_ptr<DefAST>(static_cast<DefAST *>($1)));
  };
  | VarDefList ',' VarDef {
    $$ = $1;
    $$->push_back(std::unique_ptr<DefAST>(static_cast<DefAST *>($3)));
  };

VarDef 
  : IDENT '=' Expr {
    $$ = new DefAST(false, std::move(*$1), $3);
    delete $1;
  };
  | IDENT {
    $$ = new DefAST(false, std::move(*$1), nullptr);
    delete $1;
  };

Btype
  : INT { $$ = new std::string("int"); }
  | VOID { $$ = new std::string("void"); };

Stmt
  : LVal '=' Expr ';' {
    $$ = new AssignStmtAST($1, $3);
  }
  | Block {
    $$ = $1;
  }
  | Expr ';' {
    $$ = new ExprStmtAST($1);
  }
  | ';' {
    $$ = new ExprStmtAST(nullptr);
  }
  | RETURN Expr ';' {
    $$ = new ReturnStmtAST($2);
  }
  | RETURN ';' {
    $$ = new ReturnStmtAST(nullptr);
  }
  | BREAK ';' {
    $$ = new BreakStmtAST();
  }
  | CONTINUE ';' {
    $$ = new ContinueStmtAST();
  }
  | WHILE '(' Expr ')' Stmt {
    $$ = new WhileStmtAST($3, $5);
  }
  | IF '(' Expr ')' Stmt %prec LOWER_ELSE {
    $$ = new IfStmtAST($3, $5, nullptr);
  }
  | IF '(' Expr ')' Stmt ELSE Stmt {
    $$ = new IfStmtAST($3, $5, $7);
  };

Expr
  : Number { $$ = $1; }
  | '(' Expr ')' { $$ = $2; }
  | LVal { $$ = $1; }
  /* unary experssion */
  /*  function call */
  | IDENT '(' ')' {
    auto args = std::vector<std::unique_ptr<ExprAST>>();
    $$ = new FuncCallAST(std::move(*$1), std::move(args));
    delete $1;
  }
  | IDENT '(' FuncRParams ')' {
    $$ = new FuncCallAST(std::move(*$1), std::move(*$3));
    delete $1;
    delete $3;
  }
  | '!' Expr {
    $$ = new UnaryExprAST(UnaryOp::Not, $2);
  }
  | '+' Expr %prec PRIORITY {
    $$ = $2;
  }
  | '-' Expr %prec PRIORITY {
    $$ = new UnaryExprAST(UnaryOp::Neg, $2);
  }
  /* binary experssion */
  | Expr '+' Expr {
    $$ = new BinaryExprAST(BinaryOp::Add, $1, $3);
  }
  | Expr '-' Expr {
    $$ = new BinaryExprAST(BinaryOp::Sub, $1, $3);
  }
  | Expr '*' Expr {
    $$ = new BinaryExprAST(BinaryOp::Mul, $1, $3);
  }
  | Expr '/' Expr {
    $$ = new BinaryExprAST(BinaryOp::Div, $1, $3);
  }
  | Expr '<' Expr {
    $$ = new BinaryExprAST(BinaryOp::Lt, $1, $3);
  }
  | Expr '>' Expr {
    $$ = new BinaryExprAST(BinaryOp::Gt, $1, $3);
  }
  | Expr '%' Expr {
    $$ = new BinaryExprAST(BinaryOp::Mod, $1, $3);
  }
  | Expr LE Expr {
    $$ = new BinaryExprAST(BinaryOp::Le, $1, $3);
  }
  | Expr GE Expr {
    $$ = new BinaryExprAST(BinaryOp::Ge, $1, $3);
  }
  | Expr EQ Expr {
    $$ = new BinaryExprAST(BinaryOp::Eq, $1, $3);
  }
  | Expr NE Expr {
    $$ = new BinaryExprAST(BinaryOp::Ne, $1, $3);
  }
  | Expr AND Expr {
    $$ = new BinaryExprAST(BinaryOp::And, $1, $3);
  }
  | Expr OR Expr {
    $$ = new BinaryExprAST(BinaryOp::Or, $1, $3);
  };

Number
  : INT_CONST {
    $$ = new NumberAST($1);
  };

LVal 
  : IDENT {
    $$ = new LValAST(std::move(*$1));
    delete $1;
  };

FuncRParams 
  : Expr {
    $$ = new std::vector<std::unique_ptr<ExprAST>>();
    $$->push_back(std::unique_ptr<ExprAST>(static_cast<ExprAST *>($1)));
  }
  | FuncRParams ',' Expr {
    $$ = $1;
    $$->push_back(std::unique_ptr<ExprAST>(static_cast<ExprAST *>($3)));
  };

%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *str) {
  fmt::println(stderr, "{}", str);
  if(ast) ast->dump(0);
}
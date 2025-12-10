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
%token INT RETURN OR AND EQ NE LE GE NMINUS
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block Stmt Number Expr

%left OR                  // ||
%left AND                 // &&
%left EQ NE               // == , !=
%left '<' '>' LE GE       // < > <= >=
%left '+' '-'             // + -
%left '*' '/' '%'         // * / %
%right '!' NMINUS          // ! -

%%
CompUnit
  : FuncDef {
    auto comp_unit = std::make_unique<ast::CompUnitAST>();
    comp_unit->func_def = std::unique_ptr<ast::BaseAST>($1);
    ast = std::move(comp_unit);
  };

FuncDef
  : FuncType IDENT '(' ')' Block {
    $$ = new ast::FuncDefAST(std::unique_ptr<ast::BaseAST>($1),
    *std::unique_ptr<std::string>($2), std::unique_ptr<ast::BaseAST>($5));
  };

FuncType
  : INT {
    $$ = new ast::FuncTypeAST("int");
  };

Block
  : '{' Stmt '}' {
    $$ = new ast::BlockAST(std::unique_ptr<ast::BaseAST>($2));
  };

Stmt
  : RETURN Expr ';' {
    $$ = new ast::StmtAST(std::unique_ptr<ast::BaseAST>($2));
  };

Expr
  : Number { $$ = $1; }
  | '(' Expr ')' { $$ = $2; }
  /* unary experssion */
  | '!' Expr {
    $$ = new ast::UnaryExprAST(ast::UnaryOp::Not, 
                std::unique_ptr<ast::BaseAST>($2));
  }
  | '-' Expr %prec NMINUS {
    $$ = new ast::UnaryExprAST(ast::UnaryOp::Neg,
                std::unique_ptr<ast::BaseAST>($2));
  }
  /* binary experssion */
  | Expr '+' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Add,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr '-' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Sub,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr '*' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Mul,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr '/' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Div,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr '<' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Lt,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr '>' Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Gt,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr LE Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Le,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr GE Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Ge,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr EQ Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Eq,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr NE Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Ne,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr AND Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::And,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  }
  | Expr OR Expr {
    $$ = new ast::BinaryExprAST(ast::BinaryOp::Or,
    std::unique_ptr<ast::BaseAST>($1), std::unique_ptr<ast::BaseAST>($3));
  };

Number
  : INT_CONST {
    $$ = new ast::NumberAST($1);
  };

%%

void yyerror(std::unique_ptr<ast::BaseAST> &ast, const char *str) {
  fmt::println(stderr, "{}", str);
}
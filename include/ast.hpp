#ifndef AST_HPP
#define AST_HPP

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

enum class NodeType {
  PROG,
  BLOCK,
  FUN_DECL,
  VAR_DECL,
  PARAM,
  TYPE,
  ASSIGN,
  WHILE,
  DO_WHILE,
  FOR,
  IF,
  PRINT,
  RETURN,
  BREAK,
  CONTINUE,
  CALL,
  BIN_OP,
  UN_OP,
  ID,
  NUMBER,
  CHAR,
  STRING,
  EMPTY

};
class ASTNode {
private:
  std::string node_type_to_string(NodeType t) {
    switch (t) {
    case NodeType::PROG:
      return "PROG";
    case NodeType::BLOCK:
      return "BLOCK";
    case NodeType::FUN_DECL:
      return "FUN_DECL";
    case NodeType::VAR_DECL:
      return "VAR_DECL";
    case NodeType::PARAM:
      return "PARAM";
    case NodeType::TYPE:
      return "TYPE";
    case NodeType::ASSIGN:
      return "ASSIGN";
    case NodeType::WHILE:
      return "WHILE";
    case NodeType::DO_WHILE:
      return "DO_WHILE";
    case NodeType::FOR:
      return "FOR";
    case NodeType::IF:
      return "IF";
    case NodeType::PRINT:
      return "PRINT";
    case NodeType::RETURN:
      return "RETURN";
    case NodeType::BREAK:
      return "BREAK";
    case NodeType::CONTINUE:
      return "CONTINUE";
    case NodeType::CALL:
      return "CALL";
    case NodeType::BIN_OP:
      return "BIN_OP";
    case NodeType::UN_OP:
      return "UN_OP";
    case NodeType::ID:
      return "ID";
    case NodeType::NUMBER:
      return "NUMBER";
    case NodeType::CHAR:
      return "CHAR";
    case NodeType::STRING:
      return "STRING";
    case NodeType::EMPTY:
      return "EMPTY";
    default:
      return "UNKNOWN";
    }
  }

public:
  NodeType type;
  std::string value;
  std::string inferredType;
  std::string coercedType;

  ASTNode *parent = nullptr;
  ASTNode *leftmostChild = nullptr;
  ASTNode *leftmostSib = this;
  ASTNode *next = nullptr;

  ASTNode(NodeType t, const std::string &val = "") : type(t), value(val) {}
  ~ASTNode() {
    delete leftmostChild;
    delete next;
  }

  ASTNode *make_siblings(ASTNode *y) {
    if (!y) {
      return this;
    }
    ASTNode *xsibs = this;
    while (xsibs->next != nullptr) {
      xsibs = xsibs->next;
    }
    ASTNode *ysibs = y->leftmostSib;
    xsibs->next = ysibs;
    ysibs->leftmostSib = xsibs->leftmostSib;
    ysibs->parent = xsibs->parent;
    while (ysibs->next != nullptr) {
      ysibs = ysibs->next;
      ysibs->leftmostSib = xsibs->leftmostSib;
      ysibs->parent = xsibs->parent;
    }
    return ysibs;
  }

  void adopt(ASTNode *y) {
    if (!y) {
      return;
    }
    ASTNode *x = this;
    if (x->leftmostChild != nullptr) {
      x->leftmostChild->make_siblings(y);
    } else {
      ASTNode *ysibs = y->leftmostSib;
      x->leftmostChild = ysibs;
      while (ysibs != nullptr) {
        ysibs->parent = x;
        ysibs = ysibs->next;
      }
    }
  }

  void print(int nivel = 0) {
    for (int i = 0; i < nivel; ++i) {
      std::cout << "  ";
    }

    std::cout << "-> " << node_type_to_string(type);
    if (!value.empty()) {
      std::cout << " (" << value << ")";
    }
    std::cout << std::endl;

    if (leftmostChild != nullptr) {
      leftmostChild->print(nivel + 1);
    }

    if (next != nullptr) {
      next->print(nivel);
    }
  }

  void print_ast(int nivel = 0) { print(nivel); }
};

inline ASTNode *make_family(NodeType op, const std::vector<ASTNode *> &kids) {
  ASTNode *node = new ASTNode(op);
  if (!kids.empty() && kids[0] != nullptr) {
    ASTNode *sibs = kids[0];
    for (size_t i = 1; i < kids.size(); i++) {
      if (kids[i] != nullptr) {
        sibs->make_siblings(kids[i]);
      }
    }
    node->adopt(sibs);
  }

  return node;
}

#endif // !AST_HPP

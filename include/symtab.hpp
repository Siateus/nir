#ifndef SYMTAB_HPP
#define SYMTAB_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class DataType { INT, FLOAT, CHAR, STRING, VOID };

struct Symbol {
  std::string name;
  DataType type;
  bool is_function;

  std::vector<DataType> param_types;
};

class Scope {
private:
  Scope *parent;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;

public:
  Scope(Scope *parent) : parent(parent) {}
  ~Scope() = default;

  bool define(std::shared_ptr<Symbol> sym) {
    if (symbols.find(sym->name) != symbols.end()) {
      return false;
    }
    symbols[sym->name] = sym;
    return true;
  }

  std::shared_ptr<Symbol> resolve(const std::string &name) {
    auto i = symbols.find(name);
    if (i != symbols.end()) {
      return i->second;
    }
    if (parent != nullptr) {
      return parent->resolve(name);
    }
    return nullptr;
  }

  Scope *getParent() const { return parent; }
};

#endif // SYMTAB_HPP

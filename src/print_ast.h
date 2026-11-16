#ifndef MUSIC_DSL_PRINT_AST_H
#define MUSIC_DSL_PRINT_AST_H

#include <cstddef>
#include <iosfwd>

#include "ast.h"

namespace dsl {

class PrintASTVisitor final : public Visitor {
public:
    explicit PrintASTVisitor(std::ostream& out);

    void visit(Program* node) override;
    void visit(Tempo* node) override;
    void visit(Play* node) override;
    void visit(Rest* node) override;
    void visit(Loop* node) override;

private:
    void printLine(const std::string& text);

    std::ostream& out_;
    std::size_t indentation_ = 0;
};

}  // namespace dsl

#endif  // MUSIC_DSL_PRINT_AST_H

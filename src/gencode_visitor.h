#ifndef MUSIC_DSL_GENCODE_VISITOR_H
#define MUSIC_DSL_GENCODE_VISITOR_H

#include <cstddef>
#include <iosfwd>

#include "ast.h"

namespace dsl {

class GenCodeVisitor final : public Visitor {
public:
    explicit GenCodeVisitor(std::ostream& out);

    void visit(Program* node) override;
    void visit(Tempo* node) override;
    void visit(Play* node) override;
    void visit(Rest* node) override;
    void visit(Loop* node) override;

private:
    long long durationToMilliseconds(float duration) const;
    void emitIndent();

    std::ostream& out_;
    int tempoBpm_ = 120;
    std::size_t indentation_ = 0;
};

}  // namespace dsl

#endif  // MUSIC_DSL_GENCODE_VISITOR_H

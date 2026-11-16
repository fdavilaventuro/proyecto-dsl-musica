#ifndef MUSIC_DSL_OPTIMIZER_H
#define MUSIC_DSL_OPTIMIZER_H

#include "ast.h"

namespace dsl {

class OptimizerVisitor final : public Visitor {
public:
    void visit(Program* node) override;
    void visit(Tempo* node) override;
    void visit(Play* node) override;
    void visit(Rest* node) override;
    void visit(Loop* node) override;

private:
    void optimizeInstructions(NodeList& instructions);
};

}  // namespace dsl

#endif  // MUSIC_DSL_OPTIMIZER_H

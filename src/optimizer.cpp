#include "optimizer.h"

#include <cstddef>

namespace dsl {

void OptimizerVisitor::visit(Program* node) {
    optimizeInstructions(node->instructions);
}

void OptimizerVisitor::visit(Tempo* /* node */) {}

void OptimizerVisitor::visit(Play* /* node */) {}

void OptimizerVisitor::visit(Rest* /* node */) {}

void OptimizerVisitor::visit(Loop* node) {
    optimizeInstructions(node->instructions);
}

void OptimizerVisitor::optimizeInstructions(NodeList& instructions) {
    std::size_t index = 0;
    while (index < instructions.size()) {
        Node* current = instructions[index].get();

        auto* loop = dynamic_cast<Loop*>(current);
        if (loop != nullptr && loop->repetitions == 0) {
            instructions.erase(instructions.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }

        current->accept(this);

        auto* currentRest = dynamic_cast<Rest*>(current);
        if (currentRest != nullptr && index > 0) {
            auto* previousRest = dynamic_cast<Rest*>(instructions[index - 1].get());
            if (previousRest != nullptr) {
                previousRest->duration += currentRest->duration;
                instructions.erase(instructions.begin() + static_cast<std::ptrdiff_t>(index));
                continue;
            }
        }

        ++index;
    }
}

}  // namespace dsl

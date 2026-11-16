#include "print_ast.h"

#include <ostream>
#include <string>

namespace dsl {

PrintASTVisitor::PrintASTVisitor(std::ostream& out) : out_(out) {}

void PrintASTVisitor::visit(Program* node) {
    printLine("Program");
    ++indentation_;
    for (const auto& instruction : node->instructions) instruction->accept(this);
    --indentation_;
}

void PrintASTVisitor::visit(Tempo* node) {
    printLine("Tempo: bpm=" + std::to_string(node->bpm));
}

void PrintASTVisitor::visit(Play* node) {
    printLine("Play: note=" + node->note + ", duration=" + std::to_string(node->duration));
}

void PrintASTVisitor::visit(Rest* node) {
    printLine("Rest: duration=" + std::to_string(node->duration));
}

void PrintASTVisitor::visit(Loop* node) {
    printLine("Loop: repetitions=" + std::to_string(node->repetitions));
    ++indentation_;
    for (const auto& instruction : node->instructions) instruction->accept(this);
    --indentation_;
}

void PrintASTVisitor::printLine(const std::string& text) {
    out_ << std::string(indentation_, '\t') << text << '\n';
}

}  // namespace dsl

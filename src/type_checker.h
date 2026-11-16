#ifndef MUSIC_DSL_TYPE_CHECKER_H
#define MUSIC_DSL_TYPE_CHECKER_H

#include <string>

#include "ast.h"

namespace dsl {

class TypeCheckerVisitor final : public Visitor {
public:
    void visit(Program* node) override;
    void visit(Tempo* node) override;
    void visit(Play* node) override;
    void visit(Rest* node) override;
    void visit(Loop* node) override;

private:
    static int noteToMidi(const std::string& note);
    static bool isValidPianoNote(const std::string& note);
};

}  // namespace dsl

#endif  // MUSIC_DSL_TYPE_CHECKER_H

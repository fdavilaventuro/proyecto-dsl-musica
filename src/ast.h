#ifndef MUSIC_DSL_AST_H
#define MUSIC_DSL_AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace dsl {

class Node;
class Program;
class Tempo;
class Play;
class Rest;
class Loop;
class Visitor;

using NodeList = std::vector<std::unique_ptr<Node>>;

// Nodo base del AST: todo nodo sabe visitarse.
class Node {
public:
    virtual ~Node() = default;

    virtual void accept(Visitor* v) = 0;
};

class Program : public Node {
public:
    explicit Program(NodeList instructions = {})
        : instructions(std::move(instructions)) {}

    void accept(Visitor* v) override;

    NodeList instructions;
};

class Tempo : public Node {
public:
    explicit Tempo(int bpm) : bpm(bpm) {}

    void accept(Visitor* v) override;

    int bpm;
};

class Play : public Node {
public:
    Play(std::string note, float duration)
        : note(std::move(note)), duration(duration) {}

    void accept(Visitor* v) override;

    std::string note;
    float duration;
};

class Rest : public Node {
public:
    explicit Rest(float duration) : duration(duration) {}

    void accept(Visitor* v) override;

    float duration;
};

class Loop : public Node {
public:
    Loop(int repetitions, NodeList instructions = {})
        : repetitions(repetitions), instructions(std::move(instructions)) {}

    void accept(Visitor* v) override;

    int repetitions;
    NodeList instructions;
};

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(Program* node) = 0;
    virtual void visit(Tempo* node) = 0;
    virtual void visit(Play* node) = 0;
    virtual void visit(Rest* node) = 0;
    virtual void visit(Loop* node) = 0;
};

// Definidas fuera de las clases porque requieren la definición completa de Visitor.
inline void Program::accept(Visitor* v) { v->visit(this); }
inline void Tempo::accept(Visitor* v) { v->visit(this); }
inline void Play::accept(Visitor* v) { v->visit(this); }
inline void Rest::accept(Visitor* v) { v->visit(this); }
inline void Loop::accept(Visitor* v) { v->visit(this); }

}  // namespace dsl

#endif  // MUSIC_DSL_AST_H

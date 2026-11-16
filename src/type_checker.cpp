#include "type_checker.h"

#include <stdexcept>
#include <string>

namespace dsl {
namespace {

constexpr int kFirstPianoMidi = 21;  // A0
constexpr int kLastPianoMidi = 108;  // C8

[[noreturn]] void invalidNote(const std::string& note) {
    throw std::runtime_error(
        "nota inválida en Play: '" + note +
        "'; debe corresponder a una de las 88 teclas del piano (A0-C8)");
}

[[noreturn]] void invalidDuration(const char* nodeName, float duration) {
    throw std::runtime_error(
        "duración inválida en " + std::string(nodeName) + ": " +
        std::to_string(duration) + "; debe ser mayor que 0");
}

}  // namespace

void TypeCheckerVisitor::visit(Program* node) {
    for (const auto& instruction : node->instructions) instruction->accept(this);
}

void TypeCheckerVisitor::visit(Tempo* /* node */) {}

void TypeCheckerVisitor::visit(Play* node) {
    if (!isValidPianoNote(node->note)) invalidNote(node->note);
    if (!(node->duration > 0.0f)) invalidDuration("Play", node->duration);
}

void TypeCheckerVisitor::visit(Rest* node) {
    if (!(node->duration > 0.0f)) invalidDuration("Rest", node->duration);
}

void TypeCheckerVisitor::visit(Loop* node) {
    if (node->repetitions < 0) {
        throw std::runtime_error(
            "repeticiones inválidas en Loop: " + std::to_string(node->repetitions) +
            "; deben ser mayores o iguales que 0");
    }

    for (const auto& instruction : node->instructions) instruction->accept(this);
}

int TypeCheckerVisitor::noteToMidi(const std::string& note) {
    if (note.size() < 2) return -1;

    int pitchClass = -1;
    switch (note[0]) {
        case 'C': pitchClass = 0; break;
        case 'D': pitchClass = 2; break;
        case 'E': pitchClass = 4; break;
        case 'F': pitchClass = 5; break;
        case 'G': pitchClass = 7; break;
        case 'A': pitchClass = 9; break;
        case 'B': pitchClass = 11; break;
        default: return -1;
    }

    std::size_t octaveIndex = 1;
    if (octaveIndex < note.size() && (note[octaveIndex] == '#' || note[octaveIndex] == 'b')) {
        pitchClass += note[octaveIndex] == '#' ? 1 : -1;
        ++octaveIndex;
    }

    if (octaveIndex + 1 != note.size() || note[octaveIndex] < '0' ||
        note[octaveIndex] > '8') {
        return -1;
    }

    const int octave = note[octaveIndex] - '0';
    return (octave + 1) * 12 + pitchClass;
}

bool TypeCheckerVisitor::isValidPianoNote(const std::string& note) {
    const int midi = noteToMidi(note);
    return midi >= kFirstPianoMidi && midi <= kLastPianoMidi;
}

}  // namespace dsl

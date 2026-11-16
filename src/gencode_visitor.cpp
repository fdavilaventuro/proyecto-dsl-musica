#include "gencode_visitor.h"

#include <cmath>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dsl {
namespace {

struct FrequencyEntry {
    std::string note;
    int frequency;
};

const char* sharpName(int pitchClass) {
    static constexpr const char* names[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return names[pitchClass];
}

const char* flatName(int pitchClass) {
    static constexpr const char* names[] = {
        "", "Db", "", "Eb", "", "", "Gb", "", "Ab", "", "Bb", ""};
    return names[pitchClass];
}

std::vector<FrequencyEntry> pianoFrequencies() {
    std::vector<FrequencyEntry> entries;
    entries.reserve(98);

    for (int midi = 21; midi <= 108; ++midi) {
        const int pitchClass = midi % 12;
        const int octave = midi / 12 - 1;
        const int frequency = static_cast<int>(
            std::lround(440.0 * std::pow(2.0, (midi - 69) / 12.0)));

        entries.push_back({std::string(sharpName(pitchClass)) + std::to_string(octave),
                           frequency});

        const char* flat = flatName(pitchClass);
        if (flat[0] != '\0') {
            entries.push_back({std::string(flat) + std::to_string(octave), frequency});
        }
    }

    return entries;
}

void emitPythonPrelude(std::ostream& out) {
    out << "import time\n";
    out << "import winsound\n\n";
    out << "FREQUENCIES = {\n";
    for (const FrequencyEntry& entry : pianoFrequencies()) {
        out << "    '" << entry.note << "': " << entry.frequency << ",\n";
    }
    out << "}\n\n";
    out << "def play_note(note, duration_ms):\n";
    out << "    freq = FREQUENCIES[note]\n";
    out << "    winsound.Beep(freq, int(duration_ms))\n\n";
}

}  // namespace

GenCodeVisitor::GenCodeVisitor(std::ostream& out) : out_(out) {}

void GenCodeVisitor::visit(Program* node) {
    tempoBpm_ = 120;
    indentation_ = 0;
    emitPythonPrelude(out_);

    for (const auto& instruction : node->instructions) instruction->accept(this);
}

void GenCodeVisitor::visit(Tempo* node) {
    if (node->bpm <= 0) {
        throw std::runtime_error(
            "no se puede generar Python: el tempo debe ser mayor que 0");
    }
    tempoBpm_ = node->bpm;
}

void GenCodeVisitor::visit(Play* node) {
    emitIndent();
    out_ << "play_note('" << node->note << "', " << durationToMilliseconds(node->duration)
         << ")\n";
}

void GenCodeVisitor::visit(Rest* node) {
    emitIndent();
    out_ << "time.sleep(" << durationToMilliseconds(node->duration) << " / 1000.0)\n";
}

void GenCodeVisitor::visit(Loop* node) {
    emitIndent();
    out_ << "for _ in range(" << node->repetitions << "):\n";

    ++indentation_;
    if (node->instructions.empty()) {
        emitIndent();
        out_ << "pass\n";
    } else {
        for (const auto& instruction : node->instructions) instruction->accept(this);
    }
    --indentation_;
}

long long GenCodeVisitor::durationToMilliseconds(float duration) const {
    if (!(duration > 0.0f)) {
        throw std::runtime_error(
            "no se puede generar Python: la duración debe ser mayor que 0");
    }
    if (tempoBpm_ <= 0) {
        throw std::runtime_error(
            "no se puede generar Python: el tempo debe ser mayor que 0");
    }

    return std::llround(static_cast<double>(duration) * 60000.0 / tempoBpm_);
}

void GenCodeVisitor::emitIndent() {
    out_ << std::string(indentation_ * 4, ' ');
}

}  // namespace dsl

#include "gencode_visitor.h"
#include "optimizer.h"
#include "parser.h"
#include "print_ast.h"
#include "type_checker.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

constexpr const char* kUsage =
    "Uso: musicc <archivo.music> [--print-ast] [--optimize] [--print-opt-ast]";

[[noreturn]] void usageError(const std::string& message) {
    throw std::runtime_error(message + "\n" + kUsage);
}

struct Options {
    std::string inputPath;
    bool printAst = false;
    bool optimize = false;
    bool printOptimizedAst = false;
};

Options parseArguments(int argc, char* argv[]) {
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--print-ast") {
            options.printAst = true;
        } else if (argument == "--optimize") {
            options.optimize = true;
        } else if (argument == "--print-opt-ast") {
            options.printOptimizedAst = true;
        } else if (!argument.empty() && argument[0] == '-') {
            usageError("opción desconocida: " + argument);
        } else if (!options.inputPath.empty()) {
            usageError("solo se puede especificar un archivo de entrada");
        } else {
            options.inputPath = argument;
        }
    }

    if (options.inputPath.empty()) usageError("falta el archivo de entrada");

    if (std::filesystem::path(options.inputPath).extension() != ".music") {
        usageError("el archivo de entrada debe tener extensión .music");
    }

    if (options.printOptimizedAst) options.optimize = true;
    return options;
}

std::string readSource(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("no se pudo abrir el archivo: " + path);

    std::ostringstream source;
    source << input.rdbuf();
    return source.str();
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const Options options = parseArguments(argc, argv);
        auto program = dsl::parse(readSource(options.inputPath));
        const std::filesystem::path generatedDirectory = "generated";
        std::filesystem::create_directories(generatedDirectory);
        std::filesystem::path outputPath =
            generatedDirectory / std::filesystem::path(options.inputPath).filename();
        outputPath.replace_extension(".py");

        dsl::TypeCheckerVisitor typeChecker;
        program->accept(&typeChecker);

        dsl::PrintASTVisitor printer(std::cout);
        if (options.printAst) {
            std::cout << "AST original:\n";
            program->accept(&printer);
        }

        if (options.optimize) {
            dsl::OptimizerVisitor optimizer;
            program->accept(&optimizer);
        }

        if (options.printOptimizedAst) {
            std::cout << "AST optimizado:\n";
            program->accept(&printer);
        }

        std::ofstream output(outputPath);
        if (!output) {
            throw std::runtime_error(
                "no se pudo crear el archivo " + outputPath.string());
        }

        dsl::GenCodeVisitor generator(output);
        program->accept(&generator);

        std::cout << "Código Python generado en " << outputPath.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

#include "codegen/PythonGenerator.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

// Read an entire file into a UTF-8 string.
bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

// Replace the source extension .bng with .py for the generated file.
std::string replaceExt(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return path + ".py";
    return path.substr(0, dot) + ".py";
}

void printBanner(const std::string& sourcePath) {
    std::printf("Bhasha Compiler\n\n");
    std::printf("Source: %s\n\n", sourcePath.c_str());
}

// Returns true to indicate success; false to indicate compile failure.
bool reportPhase(const char* phaseName, bool ok) {
    std::printf("%-12s %s\n", phaseName, ok ? "\xe2\x9c\x93" /* ✓ */
                                       : "\xe2\x9c\x97" /* ✗ */);
    return ok;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "ব্যবহার: %s <source.bng> [--run]\n", argv[0]);
        return 2;
    }
    std::string sourcePath = argv[1];
    bool runAfter = (argc >= 3 && std::string(argv[2]) == "--run");

    std::string source;
    if (!readFile(sourcePath, source)) {
        std::fprintf(stderr, "ফাইল পড়া যায়নি: %s\n", sourcePath.c_str());
        return 2;
    }

    printBanner(sourcePath);

    // ----- Lexing -----
    bhasha::Lexer lexer(std::move(source));
    auto tokens = lexer.tokenize();
    int lexErrs = static_cast<int>(lexer.getErrors().size());
    for (auto& e : lexer.getErrors()) {
        std::fprintf(stderr, "Lexical Error [লাইন %d]: %s\n", e.line, e.message.c_str());
    }
    bool lexOk = reportPhase("Lexing", lexErrs == 0);
    if (!lexOk) {
        std::printf("\nCompilation failed.\n");
        return 1;
    }

    // ----- Parsing -----
    bhasha::Parser parser(std::move(tokens));
    auto program = parser.parseProgram();
    int parseErrs = static_cast<int>(parser.getErrors().size());
    for (auto& e : parser.getErrors()) {
        std::fprintf(stderr, "Syntax Error [লাইন %d]: %s\n", e.line, e.message.c_str());
        if (!e.expected.empty() || !e.got.empty()) {
            std::fprintf(stderr, "  আপনি লিখেছেন: '%s'\n", e.got.c_str());
            if (!e.expected.empty()) std::fprintf(stderr, "  প্রত্যাশিত: %s\n", e.expected.c_str());
        }
    }
    bool parseOk = reportPhase("Parsing", parseErrs == 0);
    if (!parseOk) {
        std::printf("\nCompilation failed.\n");
        std::printf("%d error(s) found.\n", parseErrs);
        return 1;
    }

    // ----- Semantic Analysis -----
    bhasha::SemanticAnalyzer sem;
    sem.analyze(program);
    int semErrs = static_cast<int>(sem.getErrors().size());
    for (auto& e : sem.getErrors()) {
        std::fprintf(stderr, "Semantic Error [লাইন %d]: %s\n\n", e.line, e.message.c_str());
    }
    bool semOk = reportPhase("Semantic", semErrs == 0);
    if (!semOk) {
        std::printf("\nCompilation failed.\n");
        std::printf("%d error(s) found.\n", semErrs);
        return 1;
    }

    // ----- Code Generation -----
    bhasha::PythonGenerator gen;
    std::string py = gen.generate(program);
    std::string outPath = replaceExt(sourcePath);
    {
        std::ofstream out(outPath, std::ios::binary);
        out << py;
    }
    reportPhase("Codegen", true);

    std::printf("\nGenerated: %s\n", outPath.c_str());

    if (runAfter) {
        std::fflush(stdout);
        std::printf("\n--- Running generated Python ---\n");
        std::fflush(stdout);
        std::string cmd = "python3 \"" + outPath + "\"";
        int rc = std::system(cmd.c_str());
        if (rc != 0) std::printf("program exited with code %d\n", rc);
    }
    return 0;
}

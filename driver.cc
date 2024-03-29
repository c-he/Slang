#include <fstream>
#include <cassert>
#include <cctype>
#include <iostream>
#include "IR.h"
#include "absyn.h"
#include "debug.h"
#include "driver.h"
#include "target_gen.h"

extern int yyparse();

extern int yynerrs;
extern bool emptyFile;
extern bool DontLink;
extern std::string OutputFile;
extern std::shared_ptr<AST_Block> programBlock;
std::istream *lexer_ins_;

Driver::~Driver() = default;

void Driver::parse(std::string filename)
{
    assert(!filename.empty());
    this->filename = filename;

    std::ifstream infile(filename);
    if (!infile.good())
    {
        fprintf(stderr, "slang:\033[1;31m error:\033[0m no such file or directory: \'%s\'\n", filename.c_str());
        fprintf(stderr, "slang:\033[1;31m error:\033[0m no input files\n");
        exit(EXIT_FAILURE);
    }
    parse_helper(infile);
}

void Driver::parse(std::istream &iss)
{
    if (!iss.good() && iss.eof())
    {
        return;
    }
    parse_helper(iss);
}

void Driver::parse_helper(std::istream &stream)
{
    const int accept(0);

    lexer_ins_ = &stream;

    if (yyparse() != accept || yynerrs > 0)
    {
        fprintf(stderr, "%d errors generated.\n", yynerrs);
        exit(EXIT_FAILURE);
    }

    if (!emptyFile)
    {
#ifdef AST_DEBUG
        std::cout << programBlock << std::endl;
        programBlock->print("--");
#endif

        CodeGenContext context(filename);
        context.generateCode(*programBlock);
        if (yynerrs > 0)
        {
            fprintf(stderr, "%d errors generated.\n", yynerrs);
            exit(EXIT_FAILURE);
        }

        if (DontLink)
        {
            generateTarget(context, OutputFile);
        } else
        {
            generateTarget(context);
        }
    }
}

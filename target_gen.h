#ifndef SLANG_TARGET_GEN_H
#define SLANG_TARGET_GEN_H

#include <string>
#include "IR.h"

void generateTarget(CodeGenContext &context, const std::string &filename = "output.o");

#endif //SLANG_OBJ_GEN_H

#ifndef SLANG_ABSYN_H
#define SLANG_ABSYN_H

#include <iostream>
#include <vector>
#include <llvm/IR/Value.h>
#include <json/json.h>
#include <memory>
#include <string>

#include "debug.h"

using std::shared_ptr;
using std::make_shared;
using std::string;

class CodeGenContext;

class AST_Expression;

class AST_Statement;

class AST_VariableDeclaration;

typedef std::vector<std::shared_ptr<AST_Expression>> AST_ExpressionList;
typedef std::vector<std::shared_ptr<AST_Statement>> AST_StatementList;
typedef std::vector<std::shared_ptr<AST_VariableDeclaration>> AST_VariableList;

class AST_Node
{
public:
    AST_Node() = default;

    virtual ~AST_Node() = default;

    virtual std::string getTypeName() const = 0;

    virtual void print(std::string prefix) const = 0;

    virtual llvm::Value *generateCode(CodeGenContext &context)
    {
        return static_cast<llvm::Value *>(nullptr);
    }

    virtual Json::Value generateJson() const
    {
        return Json::Value();
    }

    int col;
    int row;

protected:
    const std::string DELIMINATER = ":";
    const std::string PREFIX = "--";
};

class AST_Expression : public AST_Node
{
public:
    AST_Expression() = default;

    std::string getTypeName() const override
    {
        return "AST_Expression";
    }

    void print(std::string prefix) const override
    {
        std::cout << prefix << getTypeName() << std::endl;
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        return root;
    }
};

class AST_Statement : public AST_Node
{
public:
    bool isGlobal = false;
    bool atLeastOnce = false;

    AST_Statement() = default;

    std::string getTypeName() const override
    {
        return "AST_Statement";
    }

    void print(std::string prefix) const override
    {
        std::cout << prefix << getTypeName() << std::endl;
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName() + DELIMINATER + (isGlobal ? "global" : "");
        return root;
    }
};

class AST_Double : public AST_Expression
{
public:
    double value;

    AST_Double() = default;

    explicit AST_Double(double value) : value(value)
    {}

    std::string getTypeName() const override
    {
        return "AST_Double";
    }

    void print(std::string prefix) const override
    {
        std::cout << prefix << getTypeName() << DELIMINATER << value << std::endl;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName() + DELIMINATER + std::to_string(value);
        return root;
    }
};

class AST_Integer : public AST_Expression
{
public:
    uint64_t value;

    AST_Integer() = default;

    explicit AST_Integer(uint64_t value) : value(value)
    {}

    std::string getTypeName() const override
    {
        return "AST_Integer";
    }

    void print(std::string prefix) const override
    {
        std::cout << prefix << getTypeName() << DELIMINATER << value << std::endl;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName() + DELIMINATER + std::to_string(value);
        return root;
    }

    operator AST_Double() const
    {
        return AST_Double(value);
    }
};

class AST_Identifier : public AST_Expression
{
public:
    std::string name;
    bool isType = false;
    bool isArray = false;

    shared_ptr<AST_ExpressionList> arraySize = make_shared<AST_ExpressionList>();

    AST_Identifier() = default;

    explicit AST_Identifier(std::string &name) : name(name)
    {}

    std::string getTypeName() const override
    {
        return "AST_Identifier";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << name << (isArray ? "(Array)" : "") << std::endl;
        if (isArray)
        {
            assert(arraySize->size() > 0);
            for (auto it = arraySize->begin(); it != arraySize->end(); it++)
            {
                (*it)->print(nextPrefix);
            }
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName() + DELIMINATER + name + (isArray ? "(Array)" : "");
        if (isArray)
        {
            assert(arraySize->size() > 0);
            for (auto it = arraySize->begin(); it != arraySize->end(); it++)
            {
                root["children"].append((*it)->generateJson());
            }
        }

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_MethodCall : public AST_Expression
{
public:
    const std::shared_ptr<AST_Identifier> id;
    std::shared_ptr<AST_ExpressionList> arguments = std::make_shared<AST_ExpressionList>();

    AST_MethodCall() = default;

    explicit AST_MethodCall(const std::shared_ptr<AST_Identifier> id) : id(id), arguments(nullptr)
    {}

    AST_MethodCall(const std::shared_ptr<AST_Identifier> id, std::shared_ptr<AST_ExpressionList> arguments) :
            id(id),
            arguments(arguments)
    {}

    std::string getTypeName() const override
    {
        return "AST_MethodCall";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        this->id->print(nextPrefix);
        if (arguments)
        {
            for (auto it = arguments->begin(); it != arguments->end(); it++)
            {
                (*it)->print(nextPrefix);
            }
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(id->generateJson());
        if (arguments)
        {
            for (auto it = arguments->begin(); it != arguments->end(); it++)
            {
                root["children"].append((*it)->generateJson());
            }
        }

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_BinaryOperator : public AST_Expression
{
public:
    int op;
    std::shared_ptr<AST_Expression> lhs;
    std::shared_ptr<AST_Expression> rhs;

    AST_BinaryOperator() = default;

    AST_BinaryOperator(std::shared_ptr<AST_Expression> lhs, int op, std::shared_ptr<AST_Expression> rhs) :
            lhs(lhs),
            op(op),
            rhs(rhs)
    {}

    std::string getTypeName() const override
    {
        return "AST_BinaryOperator";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << op << std::endl;

        lhs->print(nextPrefix);
        rhs->print(nextPrefix);
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName() + DELIMINATER + std::to_string(op);

        root["children"].append(lhs->generateJson());
        root["children"].append(rhs->generateJson());

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_Assignment : public AST_Expression
{
public:
    std::shared_ptr<AST_Identifier> lhs;
    std::shared_ptr<AST_Expression> rhs;

    AST_Assignment() = default;

    AST_Assignment(std::shared_ptr<AST_Identifier> lhs, std::shared_ptr<AST_Expression> rhs) :
            lhs(lhs),
            rhs(rhs)
    {}

    std::string getTypeName() const override
    {
        return "AST_Assignment";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;

        lhs->print(nextPrefix);
        rhs->print(nextPrefix);
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();

        root["children"].append(lhs->generateJson());
        root["children"].append(rhs->generateJson());

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_Block : public AST_Expression
{
public:
    std::shared_ptr<AST_StatementList> statements = std::make_shared<AST_StatementList>();

    AST_Block() = default;

    std::string getTypeName() const override
    {
        return "AST_Block";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        for (auto it = statements->begin(); it != statements->end(); it++)
        {
            (*it)->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        for (auto it = statements->begin(); it != statements->end(); it++)
        {
            root["children"].append((*it)->generateJson());
        }

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_ExpressionStatement : public AST_Statement
{
public:
    std::shared_ptr<AST_Expression> expression;

    AST_ExpressionStatement() = default;

    AST_ExpressionStatement(std::shared_ptr<AST_Expression> expression)
            : expression(expression)
    {}

    std::string getTypeName() const override
    {
        return "AST_ExpressionStatement";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        if (expression != nullptr)
        {
            expression->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        if (expression)
        {
            root["children"].append(expression->generateJson());
        }
        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_VariableDeclaration : public AST_Statement
{
public:
    const std::shared_ptr<AST_Identifier> type;
    std::shared_ptr<AST_Identifier> id;
    std::shared_ptr<AST_Expression> assignmentExpr = nullptr;

    AST_VariableDeclaration() = default;

    AST_VariableDeclaration(const std::shared_ptr<AST_Identifier> type, std::shared_ptr<AST_Identifier> id,
                            std::shared_ptr<AST_Expression> assignmentExpr = nullptr) :
            type(type),
            id(id),
            assignmentExpr(assignmentExpr)
    {
#ifdef AST_DEBUG
        std::cout << "isType = " << std::boolalpha << type->isType << std::endl;
        std::cout << "isArray = " << std::boolalpha << type->isArray << std::endl;
        std::cout << "isGlobal = " << std::boolalpha << isGlobal << std::endl;
#endif
        assert(type->isType);
        assert(!type->isArray || (type->isArray && type->arraySize != nullptr));
    }

    std::string getTypeName() const override
    {
        return "AST_VariableDeclaration";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << (isGlobal ? "[global]" : "") << std::endl;
        type->print(nextPrefix);
        id->print(nextPrefix);
        if (assignmentExpr)
        {
            assignmentExpr->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(type->generateJson());
        root["children"].append(id->generateJson());
        if (assignmentExpr)
        {
            root["children"].append(assignmentExpr->generateJson());
        }

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_FunctionDeclaration : public AST_Statement
{
public:
    std::shared_ptr<AST_Identifier> type;
    std::shared_ptr<AST_Identifier> id;
    std::shared_ptr<AST_VariableList> arguments = std::make_shared<AST_VariableList>();
    std::shared_ptr<AST_Block> block;
    bool isExternal;

    AST_FunctionDeclaration() = default;

    AST_FunctionDeclaration(std::shared_ptr<AST_Identifier> type, std::shared_ptr<AST_Identifier> id,
                            std::shared_ptr<AST_VariableList> arguments, std::shared_ptr<AST_Block> block,
                            bool isExternal = false) :
            type(type),
            id(id),
            arguments(arguments),
            block(block),
            isExternal(isExternal)
    {
        assert(type->isType);
    }

    std::string getTypeName() const override
    {
        return "AST_FunctionDeclaration";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        type->print(nextPrefix);
        id->print(nextPrefix);

        for (auto it = arguments->begin(); it != arguments->end(); it++)
        {
            (*it)->print(nextPrefix);
        }

        assert(isExternal || block != nullptr);
        if (block)
        {
            block->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(type->generateJson());
        root["children"].append(id->generateJson());

        for (auto it = arguments->begin(); it != arguments->end(); it++)
        {
            root["children"].append((*it)->generateJson());
        }

        assert(isExternal || block != nullptr);
        if (block)
        {
            root["children"].append(block->generateJson());
        }

        return root;
    };

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_StructDeclaration : public AST_Statement
{
public:
    shared_ptr<AST_Identifier> name;
    shared_ptr<AST_VariableList> members = make_shared<AST_VariableList>();

    AST_StructDeclaration()
    {}

    AST_StructDeclaration(shared_ptr<AST_Identifier> id, shared_ptr<AST_VariableList> arguments)
            : name(id), members(arguments)
    {}

    std::string getTypeName() const override
    {
        return "AST_StructDeclaration";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << name->name << std::endl;
        for (auto it = members->begin(); it != members->end(); it++)
        {
            (*it)->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName() + DELIMINATER + name->name;

        for (auto it = members->begin(); it != members->end(); it++)
        {
            root["children"].append((*it)->generateJson());
        }

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_ReturnStatement : public AST_Statement
{
public:
    std::shared_ptr<AST_Expression> expression;

    AST_ReturnStatement() = default;

    explicit AST_ReturnStatement(std::shared_ptr<AST_Expression> expression) : expression(expression)
    {}

    std::string getTypeName() const override
    {
        return "AST_ReturnStatement";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        if (expression)
        {
            expression->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        if (expression)
        {
            root["children"].append(expression->generateJson());
        }

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_IfStatement : public AST_Statement
{
public:
    std::shared_ptr<AST_Expression> condition;
    std::shared_ptr<AST_Block> trueBlock;
    std::shared_ptr<AST_Block> falseBlock;

    AST_IfStatement() = default;

    AST_IfStatement(std::shared_ptr<AST_Expression> condition, std::shared_ptr<AST_Block> trueBlock,
                    std::shared_ptr<AST_Block> falseBlock = nullptr) :
            condition(condition),
            trueBlock(trueBlock),
            falseBlock(falseBlock)
    {}

    std::string getTypeName() const override
    {
        return "AST_IfStatement";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        condition->print(nextPrefix);
        trueBlock->print(nextPrefix);
        if (falseBlock != nullptr)
        {
            falseBlock->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(condition->generateJson());
        root["children"].append(trueBlock->generateJson());
        if (falseBlock != nullptr)
        {
            root["children"].append(falseBlock->generateJson());
        }

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_ForStatement : public AST_Statement
{
public:
    std::shared_ptr<AST_Expression> initial, condition, increment;
    std::shared_ptr<AST_Block> block;

    AST_ForStatement() = default;

    AST_ForStatement(std::shared_ptr<AST_Block> block, std::shared_ptr<AST_Expression> initial = nullptr,
                     std::shared_ptr<AST_Expression> condition = nullptr,
                     std::shared_ptr<AST_Expression> increment = nullptr) :
            block(block),
            initial(initial),
            condition(condition),
            increment(increment)
    {
        if (condition == nullptr)
        {
            condition = std::make_shared<AST_Integer>(1);
        }
    }

    std::string getTypeName() const override
    {
        return "AST_ForStatement";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        if (initial)
        {
            initial->print(nextPrefix);
        }
        if (condition)
        {
            condition->print(nextPrefix);
        }
        if (increment)
        {
            increment->print(nextPrefix);
        }

        block->print(nextPrefix);
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        if (initial)
        {
            root["children"].append(initial->generateJson());
        }
        if (condition)
        {
            root["children"].append(condition->generateJson());
        }
        if (increment)
        {
            root["children"].append(increment->generateJson());
        }

        root["children"].append(block->generateJson());

        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_ArrayIndex : public AST_Expression
{
public:
    shared_ptr<AST_Identifier> arrayName;
    shared_ptr<AST_ExpressionList> expressions = make_shared<AST_ExpressionList>();

    AST_ArrayIndex()
    {}

    AST_ArrayIndex(shared_ptr<AST_Identifier> name, shared_ptr<AST_Expression> exp)
            : arrayName(name)
    {
        expressions->push_back(exp);
    }

    AST_ArrayIndex(shared_ptr<AST_Identifier> name, shared_ptr<AST_ExpressionList> list)
            : arrayName(name), expressions(list)
    {}

    std::string getTypeName() const override
    {
        return "AST_ArrayIndex";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        arrayName->print(nextPrefix);
        for (auto it = expressions->begin(); it != expressions->end(); it++)
        {
            (*it)->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(arrayName->generateJson());
        for (auto it = expressions->begin(); it != expressions->end(); it++)
        {
            root["children"].append((*it)->generateJson());
        }

        return root;
    }

    llvm::Value *generateCode(CodeGenContext &context) override;

};

class AST_ArrayAssignment : public AST_Expression
{
public:
    shared_ptr<AST_ArrayIndex> arrayIndex;
    shared_ptr<AST_Expression> expression;

    AST_ArrayAssignment()
    {}

    AST_ArrayAssignment(shared_ptr<AST_ArrayIndex> index, shared_ptr<AST_Expression> exp)
            : arrayIndex(index), expression(exp)
    {}

    std::string getTypeName() const override
    {
        return "AST_ArrayAssignment";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        arrayIndex->print(nextPrefix);
        expression->print(nextPrefix);
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(arrayIndex->generateJson());
        root["children"].append(expression->generateJson());
        return root;
    }

    llvm::Value *generateCode(CodeGenContext &context) override;

};

class AST_ArrayInitialization : public AST_Statement
{
public:
    AST_ArrayInitialization()
    {}

    shared_ptr<AST_VariableDeclaration> declaration;
    shared_ptr<AST_ExpressionList> expressionList = make_shared<AST_ExpressionList>();

    AST_ArrayInitialization(shared_ptr<AST_VariableDeclaration> dec, shared_ptr<AST_ExpressionList> list)
            : declaration(dec), expressionList(list)
    {}

    std::string getTypeName() const override
    {
        return "AST_ArrayInitialization";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << (isGlobal ? "[global]" : "") << std::endl;
        declaration->print(nextPrefix);

        for (auto it = expressionList->begin(); it != expressionList->end(); it++)
        {
            (*it)->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(declaration->generateJson());

        for (auto it = expressionList->begin(); it != expressionList->end(); it++)
        {
            root["children"].append((*it)->generateJson());
        }

        return root;
    }

    llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_StructMember : public AST_Expression
{
public:
    shared_ptr<AST_Identifier> id;
    shared_ptr<AST_Identifier> member;
    shared_ptr<AST_ArrayIndex> array;
    bool isArray;

    AST_StructMember()
    {}

    AST_StructMember(shared_ptr<AST_Identifier> structName, shared_ptr<AST_Identifier> member,
                     shared_ptr<AST_ArrayIndex> array = nullptr, bool isArray = false)
            : id(structName), member(member), array(array), isArray(isArray)
    {}

    std::string getTypeName() const override
    {
        return "AST_StructMember";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        id->print(nextPrefix);
        member->print(nextPrefix);
        if (isArray)
        {
            array->print(nextPrefix);
        }
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(id->generateJson());
        root["children"].append(member->generateJson());
        if (isArray)
        {
            root["children"].append(array->generateJson());
        }
        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_StructAssignment : public AST_Expression
{
public:
    shared_ptr<AST_StructMember> structMember;
    shared_ptr<AST_Expression> expression;

    AST_StructAssignment()
    {}

    AST_StructAssignment(shared_ptr<AST_StructMember> member, shared_ptr<AST_Expression> exp)
            : structMember(member), expression(exp)
    {}

    std::string getTypeName() const override
    {
        return "AST_StructAssignment";
    }

    void print(std::string prefix) const override
    {
        std::string nextPrefix = prefix + this->PREFIX;
        std::cout << prefix << getTypeName() << DELIMINATER << std::endl;
        structMember->print(nextPrefix);
        expression->print(nextPrefix);
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(structMember->generateJson());
        root["children"].append(expression->generateJson());
        return root;
    }

    llvm::Value *generateCode(CodeGenContext &context) override;
};

class AST_Literal : public AST_Expression
{
public:
    std::string value;

    AST_Literal() = default;

    AST_Literal(const std::string str) :
            value(str)
    {}

    std::string getTypeName() const override
    {
        return "AST_Literal";
    }

    void print(std::string prefix) const override
    {
        std::cout << prefix << getTypeName() << DELIMINATER << value << std::endl;
    }

    Json::Value generateJson() const override
    {
        Json::Value root;
        root["name"] = getTypeName() + DELIMINATER + value;
        return root;
    }

    virtual llvm::Value *generateCode(CodeGenContext &context) override;
};

//std::unique_ptr<AST_Expression> LogError(std::string str);

#endif //SLANG_ABSYN_H
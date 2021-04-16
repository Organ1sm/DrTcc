//
// Created by yw.
//

#ifndef DRTCC_GENCODE_H
#define DRTCC_GENCODE_H

#include "Type.h"
#include "Token.h"
#include "MemoryPool.h"
#include "AST.h"
#include "VM.h"

namespace DrTcc
{

    enum ClassT
    {
        ClzNotFound, ClzEnum,    //
        ClzNumber, ClzFunc,       //
        ClzBuiltin,               //
        ClzVarGlobal, ClzVarParam, ClzVarLocal,   //
    };

    enum ExpectType
    {
        ExpectNonConflictId, ExpectValidId, ExpectLvalue, ExpectPointer
    };

    struct SymbolType
    {
        AstNode *node;
        ClassT clazz;
        int data;
    };

    class GenCode
    {
        public:
            explicit GenCode(AstNode *node);

            ~GenCode() = default;

            void Eval();

        private:
            using InsType = BaseType<TokenType::Int>::type;
            using OpType = BaseType<TokenType::Int>::type;
            using TextType = BaseType<TokenType::Int>::type;
            using DataType = BaseType<TokenType::Char>::type;

            void Gen();

            void GenRec(AstNode *node);

            void Emit(InsType ins);

            void Emit(InsType ins, OpType op);

            void EmitTop(InsType ins);

            void EmitPop();

            BaseType<TokenType::Int>::type EmitOp(InsType ins);

            void EmitOp(InsType ins, int index);

            void Emit(AstNode *node);

            void Emit(AstNode *node, int ebp);

            void EmitDeref();

            void Emitl(AstNode *node);

            void Emits(Instrucitons ins);

            int Index() const
            { return text_.size(); }

            void Expect(ExpectType type, AstNode *node);

            SymbolType FindSymbol(const std::string &str);

            bool ConflictSymbol(const std::string &str);

            void AddSymbol(AstNode *node, ClassT clazz, int addr);

            void CalcLevel(AstNode *node);

            void MakeBuiltin();

            void BuiltinAdd(const std::string &name, Instrucitons ins);

        private:

            AstNode *root_;
            Token tok_;
            int ebp_{0};
            int ebpLocal_{0};
            int exprLevel_{0};
            int ptrLevel_{0};

            std::vector<TextType> text_;
            std::vector<DataType> data_;
            std::vector<std::unordered_map<std::string, SymbolType>> symbols_;
            std::unordered_map<std::string, SymbolType> builtins_;

    };

}


#endif //DRTCC_GENCODE_H

//
// Created by yw.
//

#ifndef DRTCC_PARSER_H
#define DRTCC_PARSER_H

#include "Lexer.h"
#include "AST.h"

namespace DrTcc
{
    class Parser
    {
        public:
            explicit Parser(std::string str);

            ~Parser() = default;

            AstNode *Parse();

            AstNode *root()
            { return ast.GetRoot(); }

        private:
            void Next();

            void Program();

            AstNode *Expression(OperatorType level);

            AstNode *Statement();

            void EnumDeclaration();

            void FunctionParameter();

            void FunctionBody();

            void FunctionDeclaration();

            void GlobalDeclaration();

        private:
            void Expect(bool, const std::string &);

            void MatchKeyword(TokenType t);

            void MatchType(TokenType t);

            void MatchOperator(OperatorType t);

            void MatchNumber();

            void MatchInteger();

            void Error(const std::string &info);

            TokenType ParseType();

        private:
            TokenType baseType{TokenType::NONE};
            Token tok_;
            Lexer lexer;
            AST ast;
    };

}


#endif //DRTCC_PARSER_H

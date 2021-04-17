//
// Created by yw.
//

#include "Parser.h"

#define Tokenize 0
#define GEN_AST 0

namespace DrTcc
{
    Parser::Parser(std::string str)
    {
        lexer.Input(str);
    }

    AstNode *Parser::Parse()
    {
        // 清空词法分析结果
        lexer.Reset();
        ast.Reset();
        
        // 语法分析
        Program();

        return ast.GetRoot();
    }

    void Parser::Next()
    {
        TokenType token;
        do
        {
            token = lexer.Next();
            if (token == TokenType::Error)
            {
                auto err = lexer.RecentError();
                printf("[%04d:%03d] %-12s - %s\n", err.line, err.column, tok_.ErrorStr(err.error).c_str(),
                       err.str.c_str());
            }

        } while(token == TokenType::Newline || token == TokenType::Space || token == TokenType::Comment ||
                token == TokenType::Error);
#if Tokenize
        if (token != TokenType::TokenEnd)
        {
            printf("[%04d:%03d] %-12s - %s\n", lexer.GetLastLine(), lexer.GetLastColumn(),
                   tok_.LexerTypeStr(lexer.GetType()).c_str(), lexer.Current().c_str());
        }
#endif
    }


    void Parser::Program()
    {
        Next();
        while(!lexer.IsType(TokenType::TokenEnd))
        {
            GlobalDeclaration();
        }
    }

    // 变量声明语句(全局或函数体内)
    // int [*]id [; | (...) {...}]
    void Parser::GlobalDeclaration()
    {
        baseType = TokenType::Int;
        // 处理enum枚举
        if (lexer.IsKeyword(TokenType::K_enum))
        {
            // enum [id] { a = 10, b = 20, ... }
            MatchKeyword(TokenType::K_enum);

            if (!lexer.IsOperator(OperatorType::Lbrace))   // {
            {
                MatchType(TokenType::Identifier);
            }
            ast.NewChild(AstNodeType::AstEnum);

            if (lexer.IsOperator(OperatorType::Lbrace))
            {
                // 处理枚举体
                MatchOperator(OperatorType::Lbrace);
                EnumDeclaration();
                MatchOperator(OperatorType::Rbrace);
            }
            ast.Convert(AstToType::ToParent);

            MatchOperator(OperatorType::Semi);      // ;
            return;
        }

        // 解析基本类型，即变量声明时的类型
        baseType = ParseType();
        if (baseType == TokenType::NONE)
        { baseType = TokenType::Int; }

        // 处理逗号分割的变量声明
        while(!lexer.IsOperator(OperatorType::Semi, OperatorType::Rbrace))
        {
            auto type = baseType;
            auto ptr = 0;

            // 处理指针 int ****x;
            while(lexer.IsOperator(OperatorType::Mul))
            {
                MatchOperator(OperatorType::Mul);
                ptr++;
            }

            // 不存在变量名则报错, invalid declaration
            if (!lexer.IsType(TokenType::Identifier))
            { Error("Bad global declaration"); }

            // 得到identifier
            auto id = lexer.GetBagsIdentifier();
            MatchType(TokenType::Identifier);

            // 函数声明
            if (lexer.IsOperator(OperatorType::Lparan))
            {
                ast.NewChild(AstNodeType::AstFunc);

                // 函数返回值类型 以及函数名
                auto returnTypeNode = ast.NewChild(AstNodeType::AstType, false);
                auto idNode = ast.NewChild(AstNodeType::AstId, false);

                // 处理变量声明
                returnTypeNode->data._type.type = type;
                returnTypeNode->data._type.ptr = ptr;
                ast.SetId(idNode, id);
#if GEN_AST
                printf("-----> [%04d:%03d] AST - Function Declaration > %s \n", lexer.GetLine(), lexer.GetColumn(),
                       id.c_str());
#endif
                FunctionDeclaration();
                ast.Convert(AstToType::ToParent);
            }
            else
            {
                ast.NewChild(AstNodeType::AstVarGlobal);
                auto varTypeNode = ast.NewChild(AstNodeType::AstType, false);
                auto idNode = ast.NewChild(AstNodeType::AstId, false);

                // 处理变量声明
                varTypeNode->data._type.type = type;
                varTypeNode->data._type.ptr = ptr;
                ast.SetId(idNode, id);
                ast.Convert(AstToType::ToParent);

#if 0
                printf("------> [%04d:%03d] Global> %04X '%s'\n", lexer.GetLine(), lexer.GetColumn(), id->value._int,
                       id->name.c_str());
#endif
            }
            if (lexer.IsOperator(OperatorType::Comma))   // ','
            { MatchOperator(OperatorType::Comma); }
        }

        Next();
    }

    void Parser::Expect(bool flag, const std::string &info)
    {
        if (!flag)
        { Error(info); }
    }

    void Parser::Error(const std::string &info)
    {
        printf("[%04d:%03d] ERROR: %s\n", lexer.GetLine(), lexer.GetColumn(), info.c_str());
        throw std::exception();
    }

    void Parser::MatchKeyword(TokenType t)
    {
        Expect(lexer.IsKeyword(t), std::string("expect keyword ") + tok_.KeywordStr(t));
        Next();
    }

    void Parser::MatchType(TokenType t)
    {
        Expect(lexer.IsType(t), std::string("expect type ") + tok_.LexerTypeStr(t));
        Next();
    }

    void Parser::MatchOperator(OperatorType t)
    {
        Expect(lexer.IsOperator(t), std::string("expect operator ") + tok_.OpNameStr(t));
        Next();
    }

    void Parser::MatchNumber()
    {
        Expect(lexer.IsNumber(), "expect number");
        Next();
    }

    void Parser::MatchInteger()
    {
        Expect(lexer.IsInteger(), "expect Integer");
        Next();
    }

    TokenType Parser::ParseType()
    {
        // 处理基本类型
        // 分char,short,int,long以及相应的无符号类型(无符号暂时不支持)
        // 以及float和double(暂时不支持)
        auto type = TokenType::Int;
        if (lexer.IsType(TokenType::Keyword))
        {
            auto _unsigned = false;
            if (lexer.IsKeyword(TokenType::K_unsigned))
            {
                _unsigned = true;
                MatchKeyword(TokenType::K_unsigned);
            }
            type = lexer.GetTypeof(_unsigned);
            MatchType(TokenType::Keyword);

        }

        return type;
    }

    void Parser::FunctionDeclaration()
    {   // 函数声明
        // returnType funcName (..) {}
        printf("***FunctionDeclaration***\n");

        MatchOperator(OperatorType::Lparan);
        ast.NewChild(AstNodeType::AstParam);
        FunctionParameter();
        ast.Convert(AstToType::ToParent);
        MatchOperator(OperatorType::Rparan);    // ')'
        MatchOperator(OperatorType::Lbrace);    // '{'
        ast.NewChild(AstNodeType::AstBlock);
        FunctionBody();
        ast.Convert(AstToType::ToParent);
    }

    void Parser::FunctionParameter()
    {
        printf("***FunctionParameter***\n");
        // 判断参数右括号结尾
        // int par1, int par2
        while(!lexer.IsOperator(OperatorType::Rparan))
        {
            auto varType = ParseType(); // 基本类型
            auto ptr = 0;               // 指针级数

            // pointer type
            while(lexer.IsOperator(OperatorType::Mul))
            {
                MatchOperator(OperatorType::Mul);
                ptr++;
            }

            // parameter name
            if (!lexer.IsType(TokenType::Identifier))
            {
                Error("bad parameter declaration");
            }

            auto id = lexer.GetBagsIdentifier();
            MatchType(TokenType::Identifier);

            ast.NewChild(AstNodeType::AstVarParam);
            auto typeNode = ast.NewChild(AstNodeType::AstType, false);
            auto idNode = ast.NewChild(AstNodeType::AstId, false);
            ast.Convert(AstToType::ToParent);

            // 保存变量
            typeNode->data._type.type = varType;
            typeNode->data._type.ptr = ptr;
            ast.SetId(idNode, id);

            if (lexer.IsOperator(OperatorType::Comma))
            { MatchOperator(OperatorType::Comma); }
        }
    }

    void Parser::EnumDeclaration()
    {
        // enum [id] { a = 1, b = 3, ...}
        int i = 0;
        while(!lexer.IsOperator(OperatorType::Rbrace))  // '}'
        {
            if (!lexer.IsType(TokenType::Identifier))
            { Error("bad enum identifier " + lexer.Current()); }

            MatchType(TokenType::Identifier);
            if (lexer.IsOperator(OperatorType::Assign))
            {
                // 赋值  a = 1
                Next();
                if (!lexer.IsInteger())
                { Error("bad enum initializer"); }

                i = lexer.GetInteger(); // 得到rvalue
                Next();
            }

            ast.NewChild(AstNodeType::AstEnumUnit);
            auto idNode = ast.NewChild(AstNodeType::AstId, false);
            auto intValueNode = ast.NewChild(AstNodeType::AstInt, false);

            ast.SetId(idNode, lexer.GetBagsIdentifier());
            intValueNode->data._int = i++;
            ast.Convert(AstToType::ToParent);

            if (lexer.IsOperator(OperatorType::Comma))
            {
                Next();
            }
        }


    }

    void Parser::FunctionBody()
    {
        // returnType funcName(..)
        // {
        //      parse here -> Function Body
        // }
        printf("***FunctionBody***\n");

        {
            // 1. local declarations
            // 2. statements
            // }

            while(lexer.IsBaseType())
            {
                // 基本类型
                baseType = ParseType();
                while(!lexer.IsOperator(OperatorType::Semi))  // ';'
                {
                    auto ptr = 0;
                    auto type = baseType;
                    // pointer
                    while(lexer.IsOperator(OperatorType::Mul))
                    {
                        MatchOperator(OperatorType::Mul);
                        ptr++;
                    }
                    // invalid declaration
                    if (!lexer.IsType(TokenType::Identifier))
                    { Error("bad local declaration"); }

                    auto id = lexer.GetBagsIdentifier();
                    MatchType(TokenType::Identifier);

                    ast.NewChild(AstNodeType::AstVarLocal);
                    auto varTypeNode = ast.NewChild(AstNodeType::AstType, false);
                    auto idNode = ast.NewChild(AstNodeType::AstId, false);
                    ast.Convert(AstToType::ToParent);

                    varTypeNode->data._type.type = type;
                    varTypeNode->data._type.ptr = ptr;
                    ast.SetId(idNode, id);

                    if (lexer.IsOperator(OperatorType::Comma))
                    {
                        MatchOperator(OperatorType::Comma);
                    }
                }
                MatchOperator(OperatorType::Semi);
            }

            // 标识声明与语句的分界线
            ast.NewChild(AstNodeType::AstEmpty, false)->data._int = 1;

            // statements
            while(!lexer.IsOperator(OperatorType::Rbrace))
            {
                auto stmt = Statement();
                if (stmt != nullptr)
                {
                    ast.AddChild(stmt);
                }
            }
        }
    }

    AstNode *Parser::Statement()
    {
        // there are 8 kinds of statements here:
        // 1. if (...) <statement> [else <statement>]
        // 2. while (...) <statement>
        // 3. { <statement> }
        // 4. return xxx;
        // 5. <empty statement>;
        // 6. expression; (expression end with semicolon)

        AstNode *node = nullptr;

        if (lexer.IsKeyword(TokenType::K_if))
        {
            MatchKeyword(TokenType::K_if);
            node = ast.NewNode(AstNodeType::AstIf);

            MatchOperator(OperatorType::Lparan);   //
            AST::SetChild(node, Expression(OperatorType::Assign));
            MatchOperator(OperatorType::Rparan);

            AST::SetChild(node, Statement());  // if statement
            if (lexer.IsKeyword(TokenType::K_else))
            {
                MatchKeyword(TokenType::K_else);
                AST::SetChild(node, Statement());
            }
        }

        else if (lexer.IsKeyword(TokenType::K_while))
        {
            MatchKeyword(TokenType::K_while);
            node = ast.NewNode(AstNodeType::AstWhile);

            MatchOperator(OperatorType::Lparan);
            AST::SetChild(node, Expression(OperatorType::Assign));  // while (conditon)
            MatchOperator(OperatorType::Rparan);

            AST::SetChild(node, Statement());
        }

        else if (lexer.IsOperator(OperatorType::Lbrace))
        {
//            printf("[%04d:%03d] lalala\n", lexer.GetLine(), lexer.GetColumn());

            MatchOperator(OperatorType::Lbrace);

            node = ast.NewNode(AstNodeType::AstBlock);
            while(!lexer.IsOperator(OperatorType::Rbrace))
            {
                AST::SetChild(node, Statement());
            }
            MatchOperator(OperatorType::Rbrace);
        }

        else if (lexer.IsKeyword(TokenType::K_return))
        {
            MatchKeyword(TokenType::K_return);
            node = ast.NewNode(AstNodeType::AstReturn);
            if (!lexer.IsOperator(OperatorType::Semi))
            {
                AST::SetChild(node, Expression(OperatorType::Assign));
            }

            MatchOperator(OperatorType::Semi);
        }

        else if (lexer.IsOperator(OperatorType::Semi))
        {
            // empty statement
            MatchOperator(OperatorType::Semi);
            node = ast.NewNode(AstNodeType::AstEmpty);
        }

        else
        {
            // 表达式
            // a = b; or function call();
            node = ast.NewNode(AstNodeType::AstExp);
            AST::SetChild(node, Expression(OperatorType::Assign));
            MatchOperator(OperatorType::Semi);
        }

        if (node && node->flag != tok_.ToUnderlying(AstNodeType::AstBlock) &&
            node->flag != tok_.ToUnderlying(AstNodeType::AstStmt))
        {
            auto temp = ast.NewNode(AstNodeType::AstStmt);
            AST::SetChild(temp, node);
            node = temp;
        }

        return node;
    }

    AstNode *Parser::Expression(OperatorType level)
    {
        // 表达式有多种类型，像 `(char) *a[10] = (int *) func(b > 0 ? 10 : 20);
        //
        // 1. unit_unary ::= unit | unit unary_op | unary_op unit
        // 2. expr ::= unit_unary (bin_op unit_unary ...)

        AstNode *node = nullptr;

        // unit_unary
        if (lexer.IsType(TokenType::TokenEnd))
        { Error("unexpected token EOF of expression"); }

        // 数字
        if (lexer.IsInteger())
        {
            // 获取类型
            auto type = lexer.GetType();
            switch (type)
            {
                case TokenType::Char:
                    node = ast.NewNode(AstNodeType::AstChar);
                    node->data._char = lexer.GetBagsChar();
                    break;
                case TokenType::Uchar:
                    node = ast.NewNode(AstNodeType::AstUchar);
                    node->data._uchar = lexer.GetBagsUchar();
                    break;
                case TokenType::Short:
                    node = ast.NewNode(AstNodeType::AstShort);
                    node->data._short = lexer.GetBagsShort();
                    break;
                case TokenType::Ushort:
                    node = ast.NewNode(AstNodeType::AstUshort);
                    node->data._ushort = lexer.GetBagsUshort();
                    break;
                case TokenType::Int:
                    node = ast.NewNode(AstNodeType::AstInt);
                    node->data._int = lexer.GetBagsInt();
                    break;
                case TokenType::Uint:
                    node = ast.NewNode(AstNodeType::AstUint);
                    node->data._uint = lexer.GetBagsUint();
                    break;
                case TokenType::Long:
                    node = ast.NewNode(AstNodeType::AstLong);
                    node->data._long = lexer.GetBagsLong();
                    break;
                case TokenType::Ulong:
                    node = ast.NewNode(AstNodeType::AstUlong);
                    node->data._ulong = lexer.GetBagsUlong();
                    break;
                case TokenType::Float:
                    node = ast.NewNode(AstNodeType::AstFloat);
                    node->data._float = lexer.GetBagsFloat();
                    break;
                case TokenType::Double:
                    node = ast.NewNode(AstNodeType::AstDouble);
                    node->data._double = lexer.GetBagsDouble();
                    break;
                default:
                    Error("invalid number");
                    break;
            }
            MatchNumber();
        }
        else if (lexer.IsType(TokenType::String))       // 字符串
        {
            std::stringstream ss;
            ss << lexer.GetBagsString();
#if 0
            printf("-----> [%04d:%03d] String > '%s'\n", lexer.GetLine(), lexer.GetColumn(),
                   lexer.GetBagsString().c_str());
#endif
            MatchType(TokenType::String);

            while(lexer.IsType(TokenType::String))
            {
                ss << lexer.GetBagsString();
#if 0
                printf("-----> [%04d:%03d] String > '%s'\n", lexer.GetLine(), lexer.GetColumn(),
                       lexer.GetBagsString().c_str());
#endif
                MatchType(TokenType::String);
            }
            node = ast.NewNode(AstNodeType::AstString);
            ast.SetStr(node, ss.str());
        }
        else if (lexer.IsKeyword(TokenType::K_sizeof))
        {
            // 支持 `sizeof(int)`, `sizeof(char)` and `sizeof(*...)`
            MatchKeyword(TokenType::K_sizeof);
            MatchOperator(OperatorType::Lparan);

            if (lexer.IsKeyword(TokenType::K_unsigned))
            { MatchKeyword(TokenType::K_unsigned); }

            auto size = lexer.GetSizeof();
            Next();

            while(lexer.IsOperator(OperatorType::Mul))
            {
                MatchOperator(OperatorType::Mul);
                if (size != BaseType<TokenType::Ptr>::size)
                { size = BaseType<TokenType::Ptr>::size; }
            }

            MatchOperator(OperatorType::Rparan);

            node = ast.NewNode(AstNodeType::AstInt);
            node->data._int = size;
        }
        else if (lexer.IsType(TokenType::Identifier))
        {
            // 变量
            // 1. function call 函数名调用
            // 2. Enum variable 枚举值
            // 3. global/local variable 全局/局部变量名

            auto id = lexer.GetBagsIdentifier();
            MatchType(TokenType::Identifier);

            if (lexer.IsOperator(OperatorType::Lparan))  // FUNCTION CALL
            {       // Function call
                MatchOperator(OperatorType::Lparan);
                node = ast.NewNode(AstNodeType::AstInvoke);
                ast.SetId(node, id);
#if GEN_AST
                printf("-----> [%04d:%03d] AST - Function Call > '%s'\n", lexer.GetLine(), lexer.GetColumn(),
                       lexer.GetBagsIdentifier().c_str());
#endif
                // arguments
                while(!lexer.IsOperator(OperatorType::Rparan))
                {
                    auto temp = ast.NewNode(AstNodeType::AstExpParam);
                    AST::SetChild(temp, Expression(OperatorType::Assign));
                    AST::SetChild(node, temp);

                    if (lexer.IsOperator(OperatorType::Comma))
                    {
                        MatchOperator(OperatorType::Comma);
                    }
                }
                MatchOperator(OperatorType::Rparan);
            }
            else
            {
                node = ast.NewNode(AstNodeType::AstId);
                ast.SetId(node, id);
            }
        }
        else if (lexer.IsOperator(OperatorType::Lparan))
        {
            // 强制转换 (int)
            MatchOperator(OperatorType::Lparan);
            if (lexer.IsType(TokenType::Keyword))
            {
                auto tmp = ParseType();  // 目标类型
                auto ptr = 0;

                while(lexer.IsOperator(OperatorType::Mul))
                {
                    MatchOperator(OperatorType::Mul);
                    ptr++;
                }

                MatchOperator(OperatorType::Rparan);

                node = ast.NewNode(AstNodeType::AstCast);
                node->data._type.type = tmp;
                node->data._type.ptr = ptr;
                AST::SetChild(node, Expression(OperatorType::Inc));
            }
            else
            {
                // 括号嵌套
                node = Expression(OperatorType::Assign);
                MatchOperator(OperatorType::Rparan);
            }
        }
        else if (lexer.IsOperator(OperatorType::Mul))
        {
            // 解引用  *a;
            MatchOperator(OperatorType::Mul);
            node = ast.NewNode(AstNodeType::AstSinOp);
            node->data._op.op = OperatorType::Mul;
            AST::SetChild(node, Expression(OperatorType::Inc));
        }
        else if (lexer.IsOperator(OperatorType::BitAnd))
        {
            // &a 取地址
            MatchOperator(OperatorType::BitAnd);
            node = ast.NewNode(AstNodeType::AstSinOp);
            node->data._op.op = OperatorType::BitAnd;
            AST::SetChild(node, Expression(OperatorType::Inc));
        }
        else if (lexer.IsOperator(OperatorType::LogicalNot))
        {
            // !
            MatchOperator(OperatorType::LogicalNot);
            node = ast.NewNode(AstNodeType::AstSinOp);
            node->data._op.op = OperatorType::LogicalNot;
            AST::SetChild(node, Expression(OperatorType::Inc));
        }
        else if (lexer.IsOperator(OperatorType::BitNot))
        {
            // ~
            MatchOperator(OperatorType::BitNot);
            node = ast.NewNode(AstNodeType::AstSinOp);
            node->data._op.op = OperatorType::BitNot;
            AST::SetChild(node, Expression(OperatorType::Inc));
        }
        else if (lexer.IsOperator(OperatorType::Add))
        {
            // + var
            MatchOperator(OperatorType::Add);
            node = ast.NewNode(AstNodeType::AstSinOp);
            node->data._op.op = OperatorType::Add;
            AST::SetChild(node, Expression(OperatorType::Inc));
        }
        else if (lexer.IsOperator(OperatorType::Minus))
        {
            MatchOperator(OperatorType::Minus);
            // = -1;  负数赋值
            if (lexer.IsInteger())
            {
                auto type = lexer.GetType();
                switch (type)
                {
                    case TokenType::Char:
                        node = ast.NewNode(AstNodeType::AstChar);
                        node->data._char = -lexer.GetBagsChar();
                        break;
                    case TokenType::Uchar:
                        node = ast.NewNode(AstNodeType::AstUchar);
                        node->data._uchar = -lexer.GetBagsUchar();
                        break;
                    case TokenType::Short:
                        node = ast.NewNode(AstNodeType::AstShort);
                        node->data._short = -lexer.GetBagsShort();
                        break;
                    case TokenType::Ushort:
                        node = ast.NewNode(AstNodeType::AstUshort);
                        node->data._ushort = -lexer.GetBagsUshort();
                        break;
                    case TokenType::Int:
                        node = ast.NewNode(AstNodeType::AstInt);
                        node->data._int = -lexer.GetBagsInt();
                        break;
                    case TokenType::Uint:
                        node = ast.NewNode(AstNodeType::AstUint);
                        node->data._uint = -lexer.GetBagsUint();
                        break;
                    case TokenType::Long:
                        node = ast.NewNode(AstNodeType::AstLong);
                        node->data._long = -lexer.GetBagsLong();
                        break;
                    case TokenType::Ulong:
                        node = ast.NewNode(AstNodeType::AstUlong);
                        node->data._ulong = -lexer.GetBagsUlong();
                        break;
                    case TokenType::Float:
                        node = ast.NewNode(AstNodeType::AstFloat);
                        node->data._float = -lexer.GetBagsFloat();
                        break;
                    case TokenType::Double:
                        node = ast.NewNode(AstNodeType::AstDouble);
                        node->data._double = -lexer.GetBagsDouble();
                        break;
                    default:
                        Error("invalid integer type");
                        break;
                }
                MatchInteger();
            }
            else
            {
                node = ast.NewNode(AstNodeType::AstSinOp);
                node->data._op.op = OperatorType::Minus;
                AST::SetChild(node, Expression(OperatorType::Inc));
            }
        }
        else if (lexer.IsOperator(OperatorType::Inc, OperatorType::Dec))
        {
            auto temp = lexer.GetBagsOperator();
            MatchType(TokenType::Operator);
            node = ast.NewNode(AstNodeType::AstSinOp);
            node->data._op.op = temp;
            AST::SetChild(node, Expression(OperatorType::Inc));
        }
        else
        {
            Error("Bad expression!");
        }

        // 二元表达式以及后缀运算符
        OperatorType op = OperatorType::OpStart;
        while(lexer.IsType(TokenType::Operator) &&
              tok_.OperatorPriority(op = lexer.GetBagsOperator()) <= tok_.OperatorPriority(level))
        {
            // 优先级
            auto temp = node;
            switch (op)
            {
                case OperatorType::Rparan:      // ')'
                case OperatorType::Rsquare:     // ']'
                case OperatorType::Colon:       // ':'
                    return node;
                case OperatorType::Assign:
                {
                    // var = Expression
                    MatchOperator(OperatorType::Assign);
                    node = ast.NewNode(AstNodeType::AstBinOp);
                    node->data._op.op = OperatorType::Assign;
                    AST::SetChild(node, temp);  // 将identifier置为子节点
                    AST::SetChild(node, Expression(OperatorType::Assign));
                }
                    break;
                case OperatorType::Query:
                {
                    // Expr ? a : b;
                    MatchOperator(OperatorType::Query);
                    node = ast.NewNode(AstNodeType::AstTriOp);
                    node->data._op.op = OperatorType::Query;
                    AST::SetChild(node, temp);
                    AST::SetChild(node, Expression(OperatorType::Assign));

                    if (lexer.IsOperator(OperatorType::Colon))
                    { MatchOperator(OperatorType::Colon); }
                    else
                    { Error("miss colon in condition"); }

                    AST::SetChild(node, Expression(OperatorType::Query));
                }
                    break;
                case OperatorType::Lsquare:
                {
                    MatchOperator(op);
                    node = ast.NewNode(AstNodeType::AstBinOp);
                    node->data._op.op = op;
                    AST::SetChild(node, temp);
                    AST::SetChild(node, Expression(OperatorType::Assign));
                    MatchOperator(OperatorType::Rsquare);
                }
                    break;
                case OperatorType::Inc:
                case OperatorType::Dec:
                {
                    MatchOperator(op);
                    node = ast.NewNode(AstNodeType::AstSinOp);
                    node->data._op.op = op;
                    node->data._op.data = 1;
                    AST::SetChild(node, temp);
                }
                    break;
                case OperatorType::Equal:
                case OperatorType::Add:
                case OperatorType::AddAssign:
                case OperatorType::Minus:
                case OperatorType::MinusAssign:
                case OperatorType::Mul:
                case OperatorType::MulAssign:
                case OperatorType::Divide:
                case OperatorType::DivAssign:
                case OperatorType::BitAnd:
                case OperatorType::AndAssign:
                case OperatorType::BitOr:
                case OperatorType::OrAssign:
                case OperatorType::BitXor:
                case OperatorType::XorAssign:
                case OperatorType::Mod:
                case OperatorType::ModAssign:
                case OperatorType::LessThan:
                case OperatorType::LessThanOrEqual:
                case OperatorType::GreaterThan:
                case OperatorType::GreaterThanOrEqual:
                case OperatorType::NotEqual:
                case OperatorType::LogicalAnd:
                case OperatorType::LogicalOr:
                case OperatorType::Pointer:
                case OperatorType::LeftShift:
                case OperatorType::LeftShiftAssign:
                case OperatorType::RightShift:
                case OperatorType::RightShiftAssign:
                {
                    MatchOperator(op);
                    node = ast.NewNode(AstNodeType::AstBinOp);
                    node->data._op.op = op;
                    AST::SetChild(node, temp);
                    AST::SetChild(node, Expression(op));
                }
                    break;
                default:
                    Error("compiler error, token = " + lexer.Current());
                    break;
            }
        }
//        printf("**** AstNodetype : %s***\n", tok_.AstNodeStr(node->flag).c_str());
        return node;
    }
}
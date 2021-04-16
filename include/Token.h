// Created by yw.
//

#ifndef DRTCC_TOKEN_H
#define DRTCC_TOKEN_H

#include "Type.h"

namespace DrTcc
{
    enum class TokenType
    {
            // LexerType
            // 0 - 20 : 21
            NONE, Ptr, Error, Char, Uchar, Short, Ushort, Int, Uint, Long, Ulong, Float, Double, Operator, Keyword,
            Identifier, String, Comment, Space, Newline,

            // 关键字 21 - 59 : 39
            KStart, K_auto, K_extern, K_typedef, K_return, K_void, K_register, K_static, K_pragma, K_sizeof,
            K_interrupt, K_bool, K_char, K_short, K_int, K_float, K_double, K_long, K_unsigned, K_signed, K_switch,
            K_break, K_case, K_continue, K_default, K_goto, K_const, K_volatile, K_if, K_else, K_true, K_false, K_do,
            K_while, K_for, K_enum, K_union, K_struct, KEnd,

            // 60
            TokenEnd,
    };

    /* 以下是映射运算符的枚举类 */
    enum class OperatorType
    {
            OpStart,                            // @START 0
            Assign, Equal,                      // =, ==  1 2
            Add, AddAssign,                     // +, +=  3 4
            Minus, MinusAssign,                 // -, -=  5 6
            Mul, MulAssign,                     // *, *=  7 8
            Divide, DivAssign,                  // /, /=  9 10
            BitAnd, AndAssign,                  // &, &=  11 12
            BitOr, OrAssign,                    // |, |=  13 14
            BitXor, XorAssign,                  // ^, ^=  15 16
            Mod, ModAssign,                     // %, %=  17 18
            LessThan, LessThanOrEqual,          // <, <=  19 20
            GreaterThan, GreaterThanOrEqual,    // >, >=  21 22
            LogicalNot, NotEqual,               // !, !=  23 24
            Escape, Query,                      // '\\', ? 25 26
            BitNot,                             // ~,   27
            Lparan, Rparan,                     // (, )  28 29
            Lbrace, Rbrace,                     // {, }
            Lsquare, Rsquare,                   // [, ]
            Comma,                              // ','
            Dot,                                // .
            Semi,                               // ;
            Colon,                              // :
            Inc, Dec,                           // ++, --;
            LogicalAnd, LogicalOr,              // &&, ||
            Pointer,                            // ->
            LeftShift, RightShift,              // <<, >>
            LeftShiftAssign,                    // <<=
            RightShiftAssign,                    // >>=
            Ellipsis,                           // ...
            OpEnd,                              // @END
    };

    enum class AstNodeType
    {
            AstRoot,                                // 根节点                      0
            AstEnum, AstEnumUnit,                   //                            1, 2
            AstVarGlobal, AstVarParam, AstVarLocal, // 全局, 参数, 局部变量          3, 4, 5
            AstFunc, AstParam, AstBlock,            // 函数，函数参数，语句块         6, 7, 8
            AstExp, AstExpParam,                    // 表达式， 表达式参数           9, 10
            AstStmt,                                // 语句                       11
            AstReturn,                              // 返回                       12
            AstSinOp, AstBinOp, AstTriOp,           // 操作符                      13, 14, 15
            AstIf, AstWhile,                        // if while                  16, 17
            AstInvoke,                             //                            18
            AstEmpty,                              //                            19
            AstId, AstType,                         // identifier, type          20, 21
            AstCast,                                // 转换                       22
            AstString, AstChar, AstUchar,           //                           23, 24, 25
            AstShort, AstUshort,                    //                           26, 27
            AstInt, AstUint,                        //                           28, 29
            AstLong, AstUlong,                      //                           30, 31
            AstFloat, AstDouble                    //                           32, 33
    };

/************* Defines of Types **************/
    template<TokenType>
    struct BaseType
    {
        using type = void *;
    };

#define DEFINE_BASETYPE(t, obj) \
    template<> \
    struct BaseType<t> \
    { \
        using type = obj; \
        static const int size = sizeof(obj); \
    };

    DEFINE_BASETYPE(TokenType::Ptr, void*)
    DEFINE_BASETYPE(TokenType::Char, char)
    DEFINE_BASETYPE(TokenType::Uchar, unsigned char)
    DEFINE_BASETYPE(TokenType::Short, short)
    DEFINE_BASETYPE(TokenType::Ushort, unsigned short)
    DEFINE_BASETYPE(TokenType::Int, int)
    DEFINE_BASETYPE(TokenType::Uint, unsigned int)
    DEFINE_BASETYPE(TokenType::Long, slong)
    DEFINE_BASETYPE(TokenType::Ulong, ulong)
    DEFINE_BASETYPE(TokenType::Float, float)
    DEFINE_BASETYPE(TokenType::Double, double)
    DEFINE_BASETYPE(TokenType::Operator, OperatorType)
    DEFINE_BASETYPE(TokenType::Keyword, TokenType)
    DEFINE_BASETYPE(TokenType::Identifier, std::string)
    DEFINE_BASETYPE(TokenType::String, std::string)
    DEFINE_BASETYPE(TokenType::Comment, std::string)
    DEFINE_BASETYPE(TokenType::Space, uint)
    DEFINE_BASETYPE(TokenType::Newline, uint)
    DEFINE_BASETYPE(TokenType::Error, ErrorType)

#undef DEFINE_BASETYPE

    template<class T>
    struct BaseLexerType
    {
        static const TokenType type = TokenType::NONE;
    };

#define DEFINE_CONVTYPE(t, obj) \
    template<> \
    struct BaseLexerType<obj> \
    { \
        static const TokenType type = t; \
    };

    DEFINE_CONVTYPE(TokenType::Ptr, void*)
    DEFINE_CONVTYPE(TokenType::Char, char)
    DEFINE_CONVTYPE(TokenType::Uchar, unsigned char)
    DEFINE_CONVTYPE(TokenType::Short, short)
    DEFINE_CONVTYPE(TokenType::Ushort, unsigned short)
    DEFINE_CONVTYPE(TokenType::Int, int)
    DEFINE_CONVTYPE(TokenType::Uint, unsigned int)
    DEFINE_CONVTYPE(TokenType::Long, slong)
    DEFINE_CONVTYPE(TokenType::Ulong, ulong)
    DEFINE_CONVTYPE(TokenType::Float, float)
    DEFINE_CONVTYPE(TokenType::Double, double)
    DEFINE_CONVTYPE(TokenType::String, std::string)
    DEFINE_CONVTYPE(TokenType::Error, ErrorType)

#undef DEFINE_CONVTYPE

    extern std::string LexerTypeList[];
    extern std::tuple<TokenType, std::string> KeywordStringList[];
    extern std::tuple<OperatorType, std::string, std::string, Instrucitons, int> OperatorStringList[];
    extern std::string ErrStringList[];
    extern std::tuple<AstNodeType, std::string> AstNodeTypeStringList[];


    class Token
    {
            friend class Lexer;

        public:
            Token() = default;

            ~Token() = default;

            template<typename E>
            constexpr typename std::underlying_type<E>::type ToUnderlying(E e) noexcept
            {
                return static_cast<typename std::underlying_type<E>::type>(e);
            }

            const std::string &LexerTypeStr(TokenType type)
            {
                assert(type >= TokenType::NONE && type < TokenType::KStart);
                return LexerTypeList[ToUnderlying(type)];
            }

            const std::string &KeywordStr(TokenType type)
            {
                assert(type > TokenType::KStart && type < TokenType::KEnd);
                auto index = ToUnderlying(type) - ToUnderlying(TokenType::KStart);
                return std::get<1>(KeywordStringList[index]);
            }

            const std::string &OpStr(OperatorType type)
            {
                assert(type > OperatorType::OpStart && type < OperatorType::OpEnd);
                return std::get<1>(OperatorStringList[ToUnderlying(type)]);
            }

            const std::string &OpNameStr(OperatorType type)
            {
                assert(type > OperatorType::OpStart && type < OperatorType::OpEnd);
                return std::get<2>(OperatorStringList[ToUnderlying(type)]);
            }

            int Op2Ins(OperatorType type)
            {
                assert(type > OperatorType::OpStart && type < OperatorType::OpEnd);
                auto insIndex = ToUnderlying(type);
                return std::get<3>(OperatorStringList[insIndex]);
            }

            int OperatorPriority(OperatorType type)
            {
                assert(type > OperatorType::OpStart && type < OperatorType::OpEnd);
                auto priorityIndex = ToUnderlying(type);
                return std::get<4>(OperatorStringList[priorityIndex]);
            }

            const std::string &ErrorStr(ErrorType type)
            {
                assert(type > ErrorType::ErrorStart && type < ErrorType::ErrorEnd);
                return ErrStringList[type];
            }

            const std::string &AstNodeStr(int index)
            {
                auto begin = ToUnderlying(AstNodeType::AstRoot);
                auto end = ToUnderlying(AstNodeType::AstDouble);
                assert(index >= begin && index <= end);
                return std::get<1>(AstNodeTypeStringList[index]);
            }
    };


    static int Hex2Dec(char c)
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        else
        { return -1; }
    }

    // 有符号 -> 无符号
    static TokenType UnsignedType(TokenType t)
    {
        switch (t)
        {
            case TokenType::Char:
                return TokenType::Uchar;
            case TokenType::Short:
                return TokenType::Ushort;
            case TokenType::Int:
                return TokenType::Uint;
            case TokenType::Long:
                return TokenType::Ulong;
            default:
                return t;
        }
    }

    // 根据后缀确定类型
    static TokenType DigitPostfixType(char c)
    {
        switch (c)
        {
            case 'C':
            case 'c':
                return TokenType::Char;
            case 'S':
            case 's':
                return TokenType::Short;
            case 'I':
            case 'i':
                return TokenType::Int;
            case 'l':
            case 'L':
                return TokenType::Long;
            case 'F':
            case 'f':
                return TokenType::Float;
            case 'D':
            case 'd':
                return TokenType::Double;
            default:
                return TokenType::Error;
        }
    }

    template<typename T>
    static T CalculateExp(T d, int e)
    {
        if (e == 0)
        { return d; }
        else if (e > 0)
        {
            for (int i = 0; i < e; i++)
            {
                d *= 10;
            }
        }
        else
        {
            for (int i = e; i < 0; i++)
            {
                d /= 10;
            }
        }
        return d;
    }

    // 单字符转义
    static int Escape(char c)
    {
        if (c >= '0' && c <= '9')
        { return c - '0'; }
        else
        {
            switch (c)
            {
                case 'b':
                    return '\b';
                case 'f':
                    return '\f';
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                case 'v':
                    return '\v';
                case '\'':
                    return '\'';
                case '\"':
                    return '\"';
                case '\\':
                    return '\\';
                default:
                    return -1;
            }
        }
    }

}


#endif //DRTCC_TOKEN_H

// Created by yw.
//

#include "Lexer.h"

#define INPUT_DEBUG 0;

namespace DrTcc
{
    Lexer::Lexer()
    {
        Init();
    }

    void Lexer::Input(std::string str)
    {
        text = str;
        length_ = str.length();
        assert(length_ > 0);
    }

    void Lexer::Init()
    {
        auto endIndex = static_cast<int>(TokenType::KEnd) - static_cast<int>( TokenType::KStart);
        for (size_t i = 1; i < endIndex; i++)
        {
            const auto &k = KeywordStringList[i];
            KeywordMap.insert({std::string(std::get<1>(k)), std::get<0>(k)});
        }

        int len = 0;
        endIndex = tok_.ToUnderlying(OperatorType::OpEnd);
        for (auto i = 1; i < endIndex; i++)
        {
            const auto &op = tok_.OpStr((OperatorType) i);
            len = op.length();

            if (len == 1)
            { sinOp[op[0]] = (OperatorType) i; }

            len = std::min(len, 2);
            for (auto j = 0; j < len; j++)
            {
                bitOp[j].set((uint) op[j]);
            }
        }
    }


    int Lexer::Local()
    {
        if (index_ < length_)
        { return text[index_]; }
        return -1;
    }

    int Lexer::Local(int offset)
    {
        if (index_ < length_)
        { return text[index_ + offset]; }
        return -1;
    }

    std::string Lexer::Current()
    {
        // #define LEX_T(t) base_t<l_##t>::type
        switch (type)
        {
            case TokenType::Space:
            case TokenType::Newline:
            case TokenType::Comment:
                return "...\t[" + tok_.LexerTypeStr(type) + "]";

            case TokenType::Operator:
                return text.substr(beginIndex_, index_ - beginIndex_) + "\t[" + tok_.OpNameStr(bags._operator) + "]";
            default:
                break;
        }
        return text.substr(beginIndex_, index_ - beginIndex_);
    }

    TokenType Lexer::RecordError(ErrorType error, int skip)
    {
        ErrorRecord err;
        err.line = line_;
        err.column = column_;
        err.startIdx = index_;
        err.endIdx = index_ + skip;
        err.error = error;
        err.str = text.substr(err.startIdx, err.endIdx - err.startIdx);
        records.push_back(err);
        Move(skip);

        return TokenType::Error;
    }

    void Lexer::Move(int idx, int inc)
    {
        beginIndex_ = index_;
        lastLine_ = line_;
        lastColumn_ = column_;

        if (inc < 0)
        { column_ += idx; }
        else
        {
            column_ = 1;
            line_ += inc;
        }

        index_ += idx;
    }


    TokenType Lexer::Next()
    {
        auto c = Local();
        if (c == -1)
        {
            type = TokenType::TokenEnd;
            return TokenType::TokenEnd;
        }

        type = TokenType::Error;

        if (isalpha(c) || c == '_')
        { type = ParseIdentifier(); }
        else if (isdigit(c))
        { type = ParseDigit(); }
        else if (isspace(c))
        { type = ParseSpace(); }
        else if (c == '\'')
        { type = ParseChar(); }
        else if (c == '\"')
        { type = ParseString(); }
        else if (c == '/')
        {
            auto nch = Local(1);
            if (nch == '/' || nch == '*')
            { type = ParseComment(); }
            else if (nch != -1)
            { type = ParseOperator(); }
            else
            {
                bags._operator = OperatorType::Divide;
                type = TokenType::Operator;
            }
        }
        else
        { type = ParseOperator(); }

        return type;
    }

    TokenType Lexer::ParseIdentifier()
    {
        size_t i;

        for (i = index_ + 1; i < length_ && (isalnum(text[i]) || text[i] == '_'); i++);

        // get the Match str
        auto s = text.substr(index_, i - index_);
        auto kw = KeywordMap.find(s);

        // match keyword
        if (kw != KeywordMap.end())
        {
            bags._keyword = kw->second;
            Move(s.length());
            return TokenType::Keyword;
        }

        bags._identifier = s;
        Move(s.length());
        return TokenType::Identifier;
    }

    TokenType Lexer::ParseDigit()
    {
        /* d:存储浮点数
         *
         * */
        TokenType t = TokenType::Int;
        TokenType postFix = TokenType::NONE;
        auto i = index_;
        auto n = 0ULL, _n = 0ULL;
        auto d = 0.0;

        if (Local() == '0' && (Local(1) == 'x' || Local(1) == 'X'))
        {
            t = Local(1) == 'x' ? TokenType::Int : TokenType::Uint;

            auto cc = 0;

            for (i += 2; i < length_ && ((cc = Hex2Dec(text[i])) != -1); i++)
            {
                if (t == TokenType::Double)
                { // 小数加位，溢出后自动转换
                    d *= 16.0;
                    d += cc;
                }
                else
                {
                    _n = n;
                    n <<= 4;
                    n += cc;
                }

                if (t == TokenType::Int)
                {
                    // 超过int范围，转为long
                    if (n > INT_MAX)
                    { t = TokenType::Long; }
                }
                else if (t == TokenType::Long)
                {
                    // 超过long范围，转为double
                    if (n > LONG_LONG_MAX)
                    {
                        d = (double) _n;
                        d *= 16.0;
                        d += cc;
                        t = TokenType::Double;
                    }
                }
            }

            return GetDigit(t, n, d, i);
        }

        // 解析整数部分
        for (; i < length_ && (isdigit(text[i])); i++)
        {
            if (t == TokenType::Double)
            {
                d *= 10.0;
                d += text[i] - '0';
            }
            else
            {   // 整数加位
                _n = n;
                n *= 10;
                n += text[i] - '0';
            }

            if (t == TokenType::Int)
            {
                if (n > INT_MAX)
                { t = TokenType::Long; }  // 溢出 转换成long
            }
            else if (t == TokenType::Long)
            {
                if (n > LONG_LONG_MAX)
                {
                    d = (double) _n;
                    d *= 10.0;
                    d += text[i] - '0';
                    t = TokenType::Double;
                }
            }
        }

        // 只有整数部分
        if (i == length_)
        { return GetDigit(t, n, d, i); }

        // 判断是有无后缀
        if ((postFix = DigitType(t, i)) != TokenType::Error)
        {
            Move(i - index_);
            return DigitFromInteger(postFix, n) ? postFix : t;
        }

        // 解析小数部分
        if (text[i] == '.')
        {
            sint l = ++i;
            for (; i < length_ && (isdigit(text[i])); ++i)
            {
                d *= 10.0;
                d += text[i] - '0';
            }

            l = i - l;
            if (l > 0)
            {
                d = (double) n + CalculateExp(d, -l);
                t = TokenType::Double;
            }
        }
        if (i == length_) // 只有整数和小数部分
        { return GetDigit(t, n, d, i); }

        if ((postFix = DigitType(t, i)) != TokenType::Error)
        {
            Move(i - index_);
            if (t == TokenType::Int)
            { return DigitFromInteger(postFix, n) ? postFix : t; }
            else
            { return DigitFromDouble(postFix, d) ? postFix : t; }
        }

        /*********************** 解析指数 *******************/
        // 强制转换成double
        if (text[i] == 'e' || text[i] == 'E')
        {
            bool negative = false;
            auto e = 0;

            if (t != TokenType::Double)
            {
                t = TokenType::Double;
                d = (double) n;
            }

            if (++i == length_)
            { return GetDigit(t, n, d, i); }

            if (!isdigit(text[i]))
            {
                if (text[i] == '-')
                { // 1e-1
                    if (++i == length_)
                    { return GetDigit(t, n, d, i); }
                    negative = true;
                }
                else if (text[i] == '+')
                {
                    if (++i == length_)
                    { return GetDigit(t, n, d, i); }
                }
                else
                { return GetDigit(t, n, d, i); }
            }

            // 解析指数部分
            for (; i < length_ && (isdigit(text[i])); ++i)
            {
                e *= 10;
                e += text[i] - '0';
            }
            d = CalculateExp(d, negative ? -e : e);
        }

        if ((postFix = DigitType(t, i)) != TokenType::Error)
        {
            Move(i - index_);
            if (t == TokenType::Int)
            { return DigitFromInteger(postFix, n) ? postFix : t; }
            else
            { return DigitFromDouble(postFix, d) ? postFix : t; }
        }

        return GetDigit(t, n, d, i);
    }

    TokenType
    Lexer::GetDigit(TokenType t, BaseType<TokenType::Ulong>::type n, BaseType<TokenType::Double>::type d, int i)
    {
        // 依照当前的类型返回数值
        switch (t)
        {
            case TokenType::Int:
                bags._int = (int) n;
                break;
            case TokenType::Long:
                bags._long = n;
                break;
            case TokenType::Double:
                bags._double = d;
                break;
            default:
                bags._double = d;
        }

        Move(i - index_);
        return t;
    }

    // 数字类型后缀(包括无符号)
    TokenType Lexer::DigitType(TokenType t, int i)
    {
        if (i == length_)
        { return TokenType::Error; }

        if (text[i] == 'U' || text[i] == 'u')
        {
            if (++i == length_)
            { return UnsignedType(t); }

            return UnsignedType(DigitPostfixType(text[i]));
        }
        else
        {
            if ((t = DigitPostfixType(text[i])) == TokenType::Error)
            { return TokenType::Error; }

            return t;
        }
    }

    bool Lexer::DigitFromInteger(TokenType t, BaseType<TokenType::Ulong>::type n)
    {
        switch (t)
        {
            case TokenType::Char:
                bags._char = (BaseType<TokenType::Char>::type) n;
                break;
            case TokenType::Uchar:
                bags._uchar = (BaseType<TokenType::Uchar>::type) n;
                break;
            case TokenType::Short:
                bags._short = (BaseType<TokenType::Short>::type) n;
                break;
            case TokenType::Ushort:
                bags._ushort = (BaseType<TokenType::Ushort>::type) n;
                break;
            case TokenType::Int:
                bags._int = (BaseType<TokenType::Int>::type) n;
                break;
            case TokenType::Uint:
                bags._uint = (BaseType<TokenType::Uint>::type) n;
                break;
            case TokenType::Long:
                bags._long = (BaseType<TokenType::Long>::type) n;
                break;
            case TokenType::Ulong:
                bags._ulong = (BaseType<TokenType::Ulong>::type) n;
                break;
            case TokenType::Float:
                bags._float = (BaseType<TokenType::Float>::type) n;
                break;
            case TokenType::Double:
                bags._double = (BaseType<TokenType::Double>::type) n;
                break;
            default:
                return false;
        }
        return true;
    }

    bool Lexer::DigitFromDouble(TokenType t, BaseType<TokenType::Double>::type n)
    {
        switch (t)
        {
            case TokenType::Char:
                bags._char = (BaseType<TokenType::Char>::type) n;
                break;
            case TokenType::Uchar:
                bags._uchar = (BaseType<TokenType::Uchar>::type) n;
                break;
            case TokenType::Short:
                bags._short = (BaseType<TokenType::Short>::type) n;
                break;
            case TokenType::Ushort:
                bags._ushort = (BaseType<TokenType::Ushort>::type) n;
                break;
            case TokenType::Int:
                bags._int = (BaseType<TokenType::Int>::type) n;
                break;
            case TokenType::Uint:
                bags._uint = (BaseType<TokenType::Uint>::type) n;
                break;
            case TokenType::Long:
                bags._long = (BaseType<TokenType::Long>::type) n;
                break;
            case TokenType::Ulong:
                bags._ulong = (BaseType<TokenType::Ulong>::type) n;
                break;
            case TokenType::Float:
                bags._float = (BaseType<TokenType::Float>::type) n;
                break;
            case TokenType::Double:
                bags._double = (BaseType<TokenType::Double>::type) n;
                break;
            default:
                return false;
        }
        return true;
    }


    TokenType Lexer::ParseSpace()
    {
        uint i, j;
        switch (text[index_])
        {
            case ' ':
            case '\t':
                SkipWhiteSpace();
                return TokenType::Space;
            case '\r':
            case '\n':
                SkipNewLine();
                return TokenType::Newline;
        }

        assert(!"space not match");
        return TokenType::Error;
    }

    void Lexer::SkipWhiteSpace()
    {
        uint i;
        for (i = index_ + 1; i < length_ && (text[i] == ' ' || text[i] == '\t'); ++i);
        bags._space = i - index_;
        Move(bags._space);
    }

    void Lexer::SkipNewLine()
    {
        // j记录换行符个数
        uint i, j;
        for (i = index_, j = 0; i < length_ && (text[i] == '\r' || (text[i] == '\n' ? ++j > 0 : false)); ++i);
        bags._newline = j;
        Move(i - index_, bags._newline);
    }

    TokenType Lexer::ParseChar()
    {
        // 处理转义字符
        if (Local(1) == '\\' && Local(3) == '\'')
        {
            auto c = Local(2);
            auto esCh = Escape(c);
            if (esCh != -1)
            {
                bags._char = (char) esCh;
                Move(4);
                return TokenType::Char;
            }
            return RecordError(ErrorInvalidChar, 4);
        }

        sint i;     // 字符长度
        // 寻找右边界
        for (i = 1; index_ + i < length_ && text[index_ + i] != '\'' && i <= 4; ++i);
        if (i == 1)
        { return RecordError(ErrorInvalidChar, i + 1); } // ''

        // 得到右边界index
        auto j = index_ + i;
        i++;
        if (j < length_ && text[j] == '\'')
        {
            if (text[index_ + 1] == '\\')
            {
                if (i == 3)  // '\'
                { return RecordError(ErrorInvalidChar, i); }

                if (i == 5)   // '\x?'
                {
                    if (text[index_ + 1] == '\\' && text[index_ + 2] == 'x')
                    {
                        auto esc = Hex2Dec(text[index_ + 3]);
                        if (esc != -1)
                        {
                            bags._char = (char) esc;
                            Move(i);
                            return TokenType::Char;
                        }
                    }
                    return RecordError(ErrorInvalidChar, i);
                }
                // '\x??'
                if (text[index_ + 1] == '\\' && text[index_ + 2] == 'x')
                {
                    auto esc = Hex2Dec(text[index_ + 3]);
                    if (esc != -1)
                    {
                        bags._char = (char) esc;
                        esc = Hex2Dec(text[index_ + 4]); // '\x_?'
                        if (esc != -1)
                        {
                            bags._char *= 0x10;
                            bags._char += (char) esc;
                            Move(i);
                            return TokenType::Char;
                        }
                    }
                }
                return RecordError(ErrorInvalidChar, i);
            }
            else if (i == 3)
            { // '?'
                bags._char = text[index_ + 1];
                Move(i);
                return TokenType::Char;
            }
        }

        return RecordError(ErrorInvalidChar, 1);
    }

    TokenType Lexer::ParseString()
    {
        int i = index_;

        auto prev = text[i];
        // 寻找非'\"'的第一个'"'

        for (i++; i < length_ && (prev == '\\' || text[i] != '"'); prev = text[i++]);
        auto j = i;

        if (j == length_) // EOF
        { return RecordError(ErrorInvalidString, i - index_); }

        std::stringstream ss;
        int status = 1;
        char c = 0;

        for (i = index_ + 1; i < j;)
        {

            // 状态机：
            // 0. 失败状态
            // 1. 处理字符
            // 2. 处理转义
            // 3. 处理’\x??' ,前一位十六进制转义
            // 4. 处理‘\x??', 后一位十六进制转义
            switch (status)
            {
                case 1: //处理字符
                {
                    if (text[i] == '\\')
                    { status = 2; }
                    else
                    { ss << text[i]; }
                    i++;
                }
                    break;
                case 2: // 处理转义
                {
                    if (text[i] == 'x')
                    {
                        status = 3;
                        i++;
                    }
                    else
                    {
                        auto esch = Escape(text[i]);
                        if (esch != -1)
                        {
                            ss << (char) esch;
                            i++;
                            status = 1;
                        }
                        else
                        { status = 0; }
                    }
                }
                    break;
                case 3:
                {
                    auto esch = Hex2Dec(text[i]);
                    if (esch != -1)
                    {
                        c = (char) esch;
                        status = 4;
                        i++;
                    }
                    else
                    { status = 0; }
                }
                    break;
                case 4:
                {
                    auto esch = Hex2Dec(text[i]);
                    if (esch != -1)
                    {
                        c *= 10;
                        c += (char) esch;
                        ss << c;
                        status = 1;
                        i++;
                    }
                    else
                    {
                        ss << c;
                        status = 1;
                    }
                }
                    break;
                default:
                    bags._string = text.substr(index_ + 1, j - index_ - 1);
                    Move(j - index_ + 1);
                    return TokenType::String;
            }
        }

        if (status == 1)
        {
            bags._string = ss.str();
            Move(j - index_ + 1);
            return TokenType::String;
        }

        bags._string = text.substr(index_ + 1, j - index_ + 1);
        Move(j - index_ + 1);
        return TokenType::String;
    }

    TokenType Lexer::ParseComment()
    {
        int i = index_;
        if (text[++i] == '/')       // '//'
        {
            // find '\n'
            for (++i; i < length_ && (text[i] != '\n' && text[i] != '\r'); i++);
            bags._comment = text.substr(index_ + 2, i - index_ - 2);
            Move(i - index_);
        }
        else                    // '/* comments */'
        {
            // find '*/'
            char prev = 0;
            auto newLine = 0;
            for (++i; i < length_ && (prev != '*' || text[i] != '/');)
            {
                prev = text[i++];
                prev == '\n' ? ++newLine : 0;
            }
            ++i;
            bags._comment = text.substr(index_ + 2, i - index_ - 1);
            Move(i - index_, newLine);
        }

        return TokenType::Comment;
    }

    TokenType Lexer::ParseOperator()
    {
        auto c = Local();

        if (bitOp[0].test((uint) c))            // 操作符第一个char判断非法
        {
            auto c2 = Local(1);

            if (c2 != -1 && bitOp[1].test((uint) c2))
            {
                // 操作符第三个char判断非法，否则解析双字符操作符
                // 三字符操作符
                auto c3 = Local(2);
                if (c3 != -1 && (c3 == '=' || c3 == '.'))
                {
                    auto p = OperatorType::OpStart;
                    if (c3 == '=')
                    {
                        if (c == c2)
                        {
                            if (c == '<')
                            { p = OperatorType::LeftShiftAssign; }
                            else if (c == '>')
                            { p = OperatorType::RightShiftAssign; }
                        }
                    }
                    else
                    {
                        if (c == '.' && c2 == '.')
                        { p = OperatorType::Ellipsis; }
                    }

                    if (p == OperatorType::OpStart)
                    { return RecordError(ErrorInvalidOperator, 3); }
                    else
                    {
                        bags._operator = (OperatorType) p;
                        Move(3);
                        return TokenType::Operator;
                    }
                }
                else    // 双字符操作符
                {
                    if (c2 == '=')
                    {
                        auto p = sinOp[c];
                        if (tok_.ToUnderlying(p) == 0 || p > OperatorType::LogicalNot)
                        {
                            // 单字符操作符
                            auto p = sinOp[c];
                            bags._operator = (OperatorType) p;
                            Move(1);
                            return TokenType::Operator;
                        }

                        bags._operator = (OperatorType) (tok_.ToUnderlying(p) + 1);
                        Move(2);
                        return TokenType::Operator;
                    }

                    auto p = OperatorType::OpStart;
                    if (c == c2)  // 相同位的双字符操作符
                    {
                        switch (c2)
                        {
                            case '+':
                                p = OperatorType::Inc;
                                break;
                            case '-':
                                p = OperatorType::Dec;
                                break;
                            case '&':
                                p = OperatorType::LogicalAnd;
                                break;
                            case '|':
                                p = OperatorType::LogicalOr;
                                break;
                            case '<':
                                p = OperatorType::LeftShift;
                                break;
                            case '>':
                                p = OperatorType::RightShift;
                                break;
                            default:
                                break;
                        }
                    }
                    else if (c == '-' && c2 == '>') // -> operator
                    { p = OperatorType::Pointer; }

                    if (p == OperatorType::OpStart)
                    {
                        auto p = sinOp[c];
                        if (tok_.ToUnderlying(p) == 0)
                        { return RecordError(ErrorInvalidOperator, 1); }

                        bags._operator = (OperatorType) p;
                        Move(1);
                        return TokenType::Operator;
                    }
                    else
                    {
                        bags._operator = (OperatorType) p;
                        Move(2);
                        return TokenType::Operator;
                    }

                }
            }

            else
            {
                auto p = sinOp[c];
                if (tok_.ToUnderlying(p) == 0)
                { return RecordError(ErrorInvalidOperator, 1); }

                bags._operator = p;
                Move(1);
                return TokenType::Operator;
            }
        }
        else
        { return RecordError(ErrorInvalidOperator, 1); }

    }

    void Lexer::Reset()
    {
        index_ = 0;
        beginIndex_ = 0;

        type = TokenType::NONE;
        line_ = 1;
        column_ = 1;
        lastLine_ = 1;
        lastColumn_ = 1;

        records.clear();
    }

    bool Lexer::IsBaseType() const
    {
        if (GetType() != TokenType::Keyword)
        { return false; }

        switch (GetBagsKeyword())
        {
            case TokenType::K_char:
            case TokenType::K_short:
            case TokenType::K_int:
            case TokenType::K_long:
            case TokenType::K_unsigned:
                return true;
            default:
                break;
        }

        return false;
    }

    BaseType<TokenType::Int>::type Lexer::GetInteger() const
    {
        assert(IsInteger());
        switch (type)
        {
            case TokenType::Char:
                return GetBagsChar();
            case TokenType::Uchar:
                return GetBagsUchar();
            case TokenType::Short:
                return GetBagsShort();
            case TokenType::Ushort:
                return GetBagsUshort();
            case TokenType::Int:
                return GetBagsInt();
            case TokenType::Uint:
                return GetBagsUint();
            case TokenType::Long:
                return GetBagsLong();
            case TokenType::Ulong:
                return GetBagsUlong();
            default:
                break;
        }

        return 0;
    }

    BaseType<TokenType::Int>::type Lexer::GetSizeof() const
    {
        assert(IsType(TokenType::Keyword));
        switch (GetBagsKeyword())
        {
            case TokenType::K_char:
                return BaseType<TokenType::Char>::size;
            case TokenType::K_short:
                return BaseType<TokenType::Short>::size;
            case TokenType::K_int:
                return BaseType<TokenType::Int>::size;
            case TokenType::K_long:
                return BaseType<TokenType::Long>::size;
            case TokenType::K_float:
                return BaseType<TokenType::Float>::size;
            case TokenType::Double:
                return BaseType<TokenType::Double>::size;
            default:
                assert(!"unsupported type");
                break;
        }
        return -1;
    }

    TokenType Lexer::GetTypeof(bool _unsigned)
    {
        assert(IsType(TokenType::Keyword));

        switch (GetBagsKeyword())
        {
            case TokenType::K_char:
                return TokenType(tok_.ToUnderlying(TokenType::Char) + int(_unsigned));
            case TokenType::K_short:
                return TokenType(tok_.ToUnderlying(TokenType::Short) + int(_unsigned));
            case TokenType::K_int:
                return TokenType(tok_.ToUnderlying(TokenType::Int) + int(_unsigned));
            case TokenType::K_long:
                return TokenType(tok_.ToUnderlying(TokenType::Long) + int(_unsigned));
            case TokenType::K_float:
                return TokenType(tok_.ToUnderlying(TokenType::Float) + int(_unsigned));
            case TokenType::Double:
                return TokenType(tok_.ToUnderlying(TokenType::Double) + int(_unsigned));
            case TokenType::K_void:
                return TokenType::NONE;
            default:
                assert(!"unsupported type");
                break;
        }

        return TokenType::Error;
    }


}



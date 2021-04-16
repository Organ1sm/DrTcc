// Created by yw .
//

#ifndef DRTCC_LEXER_H
#define DRTCC_LEXER_H

#include "Token.h"
#include "Type.h"

namespace DrTcc
{
    class Lexer
    {
        public:
            Lexer();

            ~Lexer() = default;

            Lexer(const Lexer &) = delete;

            Lexer &operator=(const Lexer &) = delete;

            void Input(std::string s);

            TokenType Next();

        public:
            struct ErrorRecord
            {
                int line, column;
                uint startIdx, endIdx;
                ErrorType error;
                std::string str;
            };

        public:
            int GetLine() const noexcept
            { return line_; }

            int GetColumn() const noexcept
            { return column_; }

            int GetLastLine() const noexcept
            { return lastLine_; }

            int GetLastColumn() const noexcept
            { return lastColumn_; }

            TokenType GetType() const noexcept
            { return type; }

            BaseType<TokenType::Char>::type GetBagsChar() const
            { return bags._char; }

            BaseType<TokenType::Uchar>::type GetBagsUchar() const
            { return bags._uchar; }

            BaseType<TokenType::Short>::type GetBagsShort() const
            { return bags._short; }

            BaseType<TokenType::Ushort>::type GetBagsUshort() const
            { return bags._ushort; }

            BaseType<TokenType::Int>::type GetBagsInt() const
            { return bags._int; };

            BaseType<TokenType::Uint>::type GetBagsUint() const
            { return bags._uint; }

            BaseType<TokenType::Long>::type GetBagsLong() const
            { return bags._long; };

            BaseType<TokenType::Ulong>::type GetBagsUlong() const
            { return bags._ulong; };

            BaseType<TokenType::Float>::type GetBagsFloat() const
            { return bags._float; };

            BaseType<TokenType::Double>::type GetBagsDouble() const
            { return bags._double; };

            BaseType<TokenType::Operator>::type GetBagsOperator() const
            { return bags._operator; };

            BaseType<TokenType::Keyword>::type GetBagsKeyword() const
            { return bags._keyword; };

            BaseType<TokenType::Identifier>::type GetBagsIdentifier() const
            { return bags._identifier; };

            BaseType<TokenType::String>::type GetBagsString() const
            { return bags._string; };

            BaseType<TokenType::Comment>::type GetBagsComment() const
            { return bags._comment; };

            BaseType<TokenType::Space>::type GetBagsSpace() const
            { return bags._space; };

            BaseType<TokenType::Newline>::type GetBagsNewline() const
            { return bags._newline; };

            BaseType<TokenType::Error>::type GetBagsError() const
            { return bags._error; };

            BaseType<TokenType::Char>::type GetStorageChar(int index) const
            { return storage._char[index]; }

            BaseType<TokenType::Uchar>::type GetStorageUchar(int index) const
            { return storage._uchar[index]; }

            BaseType<TokenType::Short>::type GetStorageShort(int index) const
            { return storage._short[index]; }

            BaseType<TokenType::Ushort>::type GetStorageUshort(int index) const
            { return storage._ushort[index]; }

            BaseType<TokenType::Int>::type GetStorageInt(int index) const
            { return storage._int[index]; }

            BaseType<TokenType::Uint>::type GetStorageUint(int index) const
            { return storage._uint[index]; }

            BaseType<TokenType::Long>::type GetStorageLong(int index) const
            { return storage._long[index]; };

            BaseType<TokenType::Ulong>::type GetStorageUlong(int index) const
            { return storage._ulong[index]; };

            BaseType<TokenType::Float>::type GetStorageFloat(int index) const
            { return storage._float[index]; };

            BaseType<TokenType::Double>::type GetStorageDouble(int index) const
            { return storage._double[index]; };

            BaseType<TokenType::Identifier>::type GetStorageIdentifier(int index) const
            { return storage._identifier[index]; };

            BaseType<TokenType::String>::type GetStorageString(int index) const
            { return storage._string[index]; };

            const ErrorRecord &RecentError() const
            { return records.back(); }

            bool IsType(TokenType t) const
            { return GetType() == t; }

            bool IsKeyword(TokenType t) const
            { return GetType() == TokenType::Keyword && GetBagsKeyword() == t; }

            bool IsOperator(OperatorType t) const
            { return GetType() == TokenType::Operator && GetBagsOperator() == t; }

            bool IsOperator(OperatorType t1, OperatorType t2) const
            { return GetType() == TokenType::Operator && (GetBagsOperator() == t1 || GetBagsOperator() == t2); }

            bool IsNumber() const
            { return GetType() >= TokenType::Char && GetType() <= TokenType::Double; }

            bool IsInteger() const
            { return GetType() >= TokenType::Char && GetType() <= TokenType::Ulong; }

            bool IsBaseType() const;

            BaseType<TokenType::Int>::type GetInteger() const;

            BaseType<TokenType::Int>::type GetSizeof() const;

            TokenType GetTypeof(bool);

            TokenType
            GetDigit(TokenType t, BaseType<TokenType::Ulong>::type n, BaseType<TokenType::Double>::type d, int i);

            TokenType DigitType(TokenType t, int i);

            bool DigitFromInteger(TokenType t, BaseType<TokenType::Ulong>::type n);

            bool DigitFromDouble(TokenType t, BaseType<TokenType::Double>::type n);

            std::string Current();

            void Reset();

        private:
            void SkipWhiteSpace();

            void SkipNewLine();

        private:

            void Init();

            int Local();

            int Local(int offset);

            void Move(int idx, int inc = -1);

            TokenType RecordError(ErrorType error, int skip);

            TokenType ParseIdentifier();

            TokenType ParseDigit();

            TokenType ParseSpace();

            TokenType ParseChar();

            TokenType ParseString();

            TokenType ParseComment();

            TokenType ParseOperator();

        private:
            std::unordered_map<std::string, TokenType> KeywordMap;
            std::array<OperatorType, 0x100> sinOp;
            std::bitset<128> bitOp[2];
            std::vector<ErrorRecord> records;
            std::string text;
            uint index_{0};
            uint length_{0};
            uint beginIndex_{0};     // last_index


            TokenType type;
            Token tok_;
            int line_{1};
            int column_{1};
            int lastLine_{1};
            int lastColumn_{1};

            struct
            {
                BaseType<TokenType::Char>::type _char;
                BaseType<TokenType::Uchar>::type _uchar;
                BaseType<TokenType::Short>::type _short;
                BaseType<TokenType::Ushort>::type _ushort;
                BaseType<TokenType::Int>::type _int;
                BaseType<TokenType::Uint>::type _uint;
                BaseType<TokenType::Long>::type _long;
                BaseType<TokenType::Ulong>::type _ulong;
                BaseType<TokenType::Float>::type _float;
                BaseType<TokenType::Double>::type _double;
                BaseType<TokenType::Operator>::type _operator;
                BaseType<TokenType::Keyword>::type _keyword;
                BaseType<TokenType::Identifier>::type _identifier;
                BaseType<TokenType::String>::type _string;
                BaseType<TokenType::Comment>::type _comment;
                BaseType<TokenType::Space>::type _space;
                BaseType<TokenType::Newline>::type _newline;
                BaseType<TokenType::Error>::type _error;
            } bags;

            struct
            {
                std::vector<BaseType<TokenType::Char>::type> _char;
                std::vector<BaseType<TokenType::Uchar>::type> _uchar;
                std::vector<BaseType<TokenType::Short>::type> _short;
                std::vector<BaseType<TokenType::Ushort>::type> _ushort;
                std::vector<BaseType<TokenType::Int>::type> _int;
                std::vector<BaseType<TokenType::Uint>::type> _uint;
                std::vector<BaseType<TokenType::Long>::type> _long;
                std::vector<BaseType<TokenType::Ulong>::type> _ulong;
                std::vector<BaseType<TokenType::Float>::type> _float;
                std::vector<BaseType<TokenType::Double>::type> _double;
                std::vector<BaseType<TokenType::Operator>::type> _operator;
                std::vector<BaseType<TokenType::Keyword>::type> _keyword;
                std::vector<BaseType<TokenType::Identifier>::type> _identifier;
                std::vector<BaseType<TokenType::String>::type> _string;
                std::vector<BaseType<TokenType::Comment>::type> _comment;
                std::vector<BaseType<TokenType::Space>::type> _space;
                std::vector<BaseType<TokenType::Newline>::type> _newline;
                std::vector<BaseType<TokenType::Error>::type> _error;
            } storage;
    };

}
#endif //DRTCC_LEXER_H
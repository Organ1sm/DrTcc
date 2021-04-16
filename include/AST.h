//
// Created by yw.
//

#ifndef DRTCC_AST_H
#define DRTCC_AST_H

#include "MemoryPool.h"
#include "Token.h"

#define AST_NODE_MEM (32 * 1024)
#define AST_STR_MEM (16 * 1024)

namespace DrTcc
{
    enum class AstToType
    {
            ToParent, ToPrev, ToNext, ToChild
    };


    struct AstNode
    {
        uint32_t flag;      // 节点类型

        union
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
            const char *_string;            // 存放identifier名

            struct
            {
                TokenType type;
                sint ptr;
            } _type;

            struct
            {
                OperatorType op;        // 名称
                sint data;              // 前置 后置
            } _op;

            struct
            {
                sint _1, _2;
            } _ins;
        } data;

        // 广义表
        AstNode *parent;    // 父节点
        AstNode *prev;      // 左兄弟
        AstNode *next;      // 右兄弟
        AstNode *child;     // 最左儿子
    };

    class AST
    {
        public:
            AST();

            ~AST() = default;

            AstNode *GetRoot();

            AstNode *NewNode(AstNodeType type);

            AstNode *NewChild(AstNodeType type, bool step = true);

            AstNode *NewSibling(AstNodeType type, bool step = true);

            AstNode *AddChild(AstNode *);

            static AstNode *SetChild(AstNode *node, AstNode *child);

            static AstNode *SetSibling(AstNode *node, AstNode *sibling);

            static int ChildrenSize(AstNode *node);

            void SetId(AstNode *node, const std::string &str);

            void SetStr(AstNode *node, const std::string &str);

            static std::string DisplayStr(AstNode *node);

            void Convert(AstToType type);

            static void Print(AstNode *node, int level, std::ostream &os);

            void Reset();

        private:
            void Init();

        private:
            Token tok1_;
            MemoryPool<AST_NODE_MEM> nodes;     // 全局ast节点管理
            MemoryPool<AST_STR_MEM> strings;     // 全局字符串管理
            std::unordered_map<std::string, const char *> vars;// 变量名查找
            AstNode *root;
            AstNode *current;

    };
}


#endif //DRTCC_AST_H

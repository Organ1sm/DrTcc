//
// Created by yw.
//

#include "AST.h"

namespace DrTcc
{
    AST::AST()
    {
        Init();
    }

    void AST::Init()
    {
        root = NewNode(AstNodeType::AstRoot);
        current = root;
    }

    AstNode *AST::GetRoot()
    {
        if (!vars.empty())
        { vars.clear(); }

        return root;
    }

    AstNode *AST::NewNode(AstNodeType type)
    {
        if (nodes.Available() < 64)
        {
            printf("AST ERROR: 'nodes' out of memory\n");
            throw std::exception();
        }

        auto node = nodes.Alloc<AstNode>();
        memset(node, 0, sizeof(AstNode));
        node->flag = tok1_.ToUnderlying(type);

        return node;
    }

    AstNode *AST::NewChild(AstNodeType type, bool step)
    {
        auto node = NewNode(type);
        SetChild(current, node);
        if (step)
        { current = node; }

        return node;
    }

    AstNode *AST::NewSibling(AstNodeType type, bool step)
    {
        auto node = NewNode(type);
        SetSibling(current, node);

        if (step)
        { current = node; }

        return node;
    }

    AstNode *AST::SetChild(AstNode *node, AstNode *child)
    {
        child->parent = node;
        if (node->child == nullptr)
        {
            node->child = child;
            child->prev = child->next = child;
        }
        else        // 添加到尾部，也就是右边
        {
            child->prev = node->child->prev;
            child->next = node->child;
            node->child->prev->next = child;
            node->child->prev = child;
        }
        return node;
    }

    AstNode *AST::SetSibling(AstNode *node, AstNode *sibling)
    {
        sibling->parent = node->parent;
        sibling->prev = node;
        sibling->next = node->next;
        node->next = sibling;
        return sibling;
    }

    int AST::ChildrenSize(AstNode *node)
    {
        if (!node || !node->child)
        { return 0; }

        node = node->child;
        auto i = node;
        auto count = 0;

        do
        {
            count++;
            i = i->next;
        }
        while(i != node);

        return count;
    }

    AstNode *AST::AddChild(AstNode *node)
    {
        return SetChild(current, node);
    }

    void AST::SetId(AstNode *node, const std::string &str)
    {
        // 从变量名表中查找
        auto f = vars.find(str);
        if (f != vars.end())
        {
            node->data._string = f->second;
        }
        else
        {   // 没有就插入
            SetStr(node, str);
            vars.insert(std::make_pair(str, node->data._string));
        }
    }

    void AST::SetStr(AstNode *node, const std::string &str)
    {
        if (strings.Available() < 64)
        {
            printf("AST ERROR: 'strings' out of memory\n");
            throw std::exception();
        }

        auto len = str.length();
        auto s = strings.AllocArray<char>(len + 1);
        memcpy(s, str.c_str(), len);
        s[len] = 0;
        node->data._string = s;
    }

    void AST::Reset()
    {
        nodes.Clear();
        strings.Clear();
        vars.clear();
        Init();
    }

    void AST::Convert(AstToType type)
    {
        switch (type)
        {
            case AstToType::ToParent:
                current = current->parent;
                break;
            case AstToType::ToPrev:
                current = current->prev;
                break;
            case AstToType::ToNext:
                current = current->next;
                break;
            case AstToType::ToChild:
                current = current->child;
                break;
        }
    }

    std::string AST::DisplayStr(AstNode *node)
    {
        std::stringstream ss;
        for (auto c = node->data._string; *c != 0; c++)
        {
            if (isprint(*c))
            { ss << *c; }
            else
            {
                if (*c == '\n')
                { ss << "\\n"; }
                else
                { ss << "."; }
            }
        }

        return ss.str();
    }

    template<typename T>
    static void AstRecursion(AstNode *node, int level, std::ostream &os, T f)
    {
        if (node == nullptr)
        { return; }

        auto i = node;
        if (i->next == i)
        {
            f(i, level, os);
            return;
        }
        f(i, level, os);
        i = i->next;
        while(i != node)
        {
            f(i, level, os);
            i = i->next;
        }
    }

    void AST::Print(AstNode *node, int level, std::ostream &os)
    {
        Token tok_;
        if (node == nullptr)
        { return; }

        auto rec = [&](auto n, auto l, auto &os) { AST::Print(n, l, os); };
        auto type = (AstNodeType) node->flag;

        switch (type)
        {
            case AstNodeType::AstRoot:      // 根结点
                AstRecursion(node->child, level, os, rec);
                break;
            case AstNodeType::AstEnum:      // 枚举
                os << "enum {" << std::endl;
                AstRecursion(node->child, level + 1, os, rec);
                os << "};" << std::endl;
                break;
            case AstNodeType::AstEnumUnit:
                os << std::setw(level * 4);
                rec(node->child, level, os);        // id
                os << " = ";
                rec(node->child->next, level, os);  // int
                if (node->next != node->parent->child)
                {
                    os << ',';
                }
                os << std::endl;
                break;
            case AstNodeType::AstVarGlobal:
                rec(node->child, level, os);        // type
                os << ' ';
                rec(node->child->next, level, os);  // id
                os << ';' << std::endl;
                break;
            case AstNodeType::AstVarParam:
                rec(node->child, level, os);        // type
                os << ' ';
                rec(node->child->next, level, os);  // id
                if (node->next != node->parent->child)
                {
                    os << ", ";
                }
                break;
            case AstNodeType::AstVarLocal:
                os << std::setfill(' ') << std::setw(level * 4) << "";
                rec(node->child, level, os);        // type
                os << ' ';
                rec(node->child->next, level, os);  // id
                os << ';' << std::endl;
                break;
            case AstNodeType::AstFunc:
            {
                auto _node = node->child;
                rec(_node, level, os); // Function return type
                os << ' ';
                _node = _node->next;
                rec(_node, level, os); // function id
                os << '(';
                _node = _node->next;
                rec(_node, level, os); // param
                os << ')' << ' ';
                _node = _node->next;
                rec(_node, level, os); // block
                os << std::endl;
            }
                break;

            case AstNodeType::AstParam:
                AstRecursion(node->child, level, os, rec);
                break;

            case AstNodeType::AstBlock:
                if (node->parent->flag == tok_.ToUnderlying(AstNodeType::AstBlock))
                {
                    os << std::setfill(' ') << std::setw(level * 4) << "";
                }
                os << '{' << std::endl;
                AstRecursion(node->child, level + 1, os, rec); // statement
                os << std::setfill(' ') << std::setw(level * 4) << "";
                os << '}';
                break;

            case AstNodeType::AstStmt:
                os << std::setfill(' ') << std::setw(level * 4) << "";
                AstRecursion(node->child, level, os, rec);
                break;
            case AstNodeType::AstReturn:
                os << "return";
                if (node->child != nullptr)
                {
                    os << ' ';
                    rec(node->child, level, os);
                }
                os << ';' << std::endl;
                break;
            case AstNodeType::AstExp:
                rec(node->child, level, os);        // expression
                os << ';' << std::endl;
                break;
            case AstNodeType::AstExpParam:
                rec(node->child, level, os);        // param
                if (node->next != node->parent->child)
                { os << ", "; }
                break;
            case AstNodeType::AstSinOp:
                if (node->data._op.data == 0)
                { // 前置
                    os << tok_.OpStr(node->data._op.op);
                    rec(node->child, level, os); // exp
                }
                else
                { // 后置
                    rec(node->child, level, os); // exp
                    os << tok_.OpStr(node->data._op.op);
                }
                break;

            case AstNodeType::AstBinOp:
            {
                bool paran = false;
                switch ((AstNodeType) node->parent->flag)
                {
                    case AstNodeType::AstSinOp:
                    case AstNodeType::AstBinOp:
                    case AstNodeType::AstTriOp:
                        if (node->parent->data._op.op == OperatorType::Lsquare)
                        {
                            paran = false;
                        }
                        else if (node->data._op.op < node->parent->data._op.op)
                        {
                            paran = true;
                        }
                        break;
                    default:
                        break;
                }
                if (paran)
                {
                    os << '(';
                }
                if (node->data._op.op == OperatorType::Lsquare)
                {
                    rec(node->child, level, os); // exp
                    os << '[';
                    rec(node->child->next, level, os);
                    os << ']'; // index
                }
                else
                {
                    rec(node->child, level, os); // exp1
                    os << ' ' << tok_.OpStr(node->data._op.op) << ' ';
                    rec(node->child->next, level, os); // exp2
                }
                if (paran)
                {
                    os << ')';
                }
            }
                break;

            case AstNodeType::AstTriOp:   // ? : true : false;
                if (node->data._op.op == OperatorType::Query)
                {
                    rec(node->child, level, os);       // cond
                    os << " ? ";
                    rec(node->child->next, level, os); // true
                    os << " : ";
                    rec(node->child->prev, level, os); // false
                }
                break;

            case AstNodeType::AstIf:
            {
                os << "if (";
                rec(node->child, level, os);    // condition
                os << ')';
                auto _block = node->child->next->flag == tok_.ToUnderlying(AstNodeType::AstBlock);
                if (_block)
                {
                    os << ' ';
                    rec(node->child->next, level, os);
                }
                else
                {
                    os << std::endl;
                    rec(node->child->next, level + 1, os);
                }
                if (node->child->next != node->child->prev)
                {
                    if (_block)
                    {
                        os << ' ';
                    }
                    else
                    {
                        os << std::setfill(' ') << std::setw(level * 4) << "";
                    }
                    os << "else";
                    auto _else = node->child->prev;
                    _block = _else->flag == tok_.ToUnderlying(AstNodeType::AstBlock);
                    if (_block)
                    {
                        os << ' ';
                        rec(_else, level, os);
                        os << std::endl;
                    }
                    else
                    {
                        if (_else->flag == tok_.ToUnderlying(AstNodeType::AstStmt))
                        {
                            os << ' ';
                            rec(_else->child, level, os);
                        }
                        else
                        {
                            os << std::endl;
                            rec(_else, level + 1, os);
                        }
                    }
                }
                else
                {
                    if (_block)
                    {
                        os << std::endl;
                    }
                }
            }
                break;

            case AstNodeType::AstWhile:
                os << "while (";
                rec(node->child, level, os);
                os << ") ";
                rec(node->child->next, level, os);
                os << std::endl;
                break;

            case AstNodeType::AstInvoke:
                os << node->data._string << '('; // id
                AstRecursion(node->child, level, os, rec); // param
                os << ')';
                break;

            case AstNodeType::AstEmpty:
                break;
            case AstNodeType::AstId:
                os << node->data._string;
                break;

            case AstNodeType::AstType:
                os << tok_.LexerTypeStr(node->data._type.type);
                if (node->data._type.ptr > 0)
                {
                    os << std::setfill('*') << std::setw(node->data._type.ptr) << "";
                }
                break;

            case AstNodeType::AstCast:
                os << '(';
                os << tok_.LexerTypeStr(node->data._type.type);
                if (node->data._type.ptr > 0)
                {
                    os << std::setfill('*') << std::setw(node->data._type.ptr) << "";
                }
                os << ')';
                rec(node->child, level, os);
                break;

            case AstNodeType::AstString:
            {
                os << '"' << DisplayStr(node) << '"';
            }
                break;
            case AstNodeType::AstChar:
                if (isprint(node->data._char))
                { os << '\'' << node->data._char << '\''; }
                else if (node->data._char == '\n')
                { os << "'\\n'"; }
                else
                {
                    os << "'\\x" << std::setiosflags(std::ios::uppercase) << std::hex << std::setfill('0')
                       << std::setw(2) << (unsigned int) node->data._char << '\'';
                }
                break;
            case AstNodeType::AstUchar:
                os << (unsigned int) node->data._uchar;
                break;
            case AstNodeType::AstShort:
                os << node->data._short;
                break;
            case AstNodeType::AstUshort:
                os << node->data._ushort;
                break;
            case AstNodeType::AstInt:
                os << node->data._int;
                break;
            case AstNodeType::AstUint:
                os << node->data._uint;
                break;
            case AstNodeType::AstLong:
                os << node->data._long;
                break;
            case AstNodeType::AstUlong:
                os << node->data._ulong;
                break;
            case AstNodeType::AstFloat:
                os << node->data._float;
                break;
            case AstNodeType::AstDouble:
                os << node->data._double;
                break;
        }
    }

}

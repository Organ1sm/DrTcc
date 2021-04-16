//
// Created by yw.
//

#include "GenCode.h"

#define GEN_DEBUG 1
namespace DrTcc
{
    std::string ClassStringList[] = {"NotFound", "Enum", "Number", "Func", "Builtin", "Global", "Param", "Local",};

    const std::string &ClassStr(ClassT type)
    {
        assert(type >= ClzNotFound && type <= ClzVarLocal);
        return ClassStringList[type];
    }


    GenCode::GenCode(AstNode *node) : root_(node)
    {
        MakeBuiltin();
        Gen();
    }

    void GenCode::MakeBuiltin()
    {
        symbols_.emplace_back();
        BuiltinAdd("printf", PRTF);
        BuiltinAdd("memcmp", MCMP);
        BuiltinAdd("exit", EXIT);
        BuiltinAdd("memset", MSET);
        BuiltinAdd("open", OPEN);
        BuiltinAdd("read", READ);
        BuiltinAdd("close", CLOS);
        BuiltinAdd("malloc", MALC);
        BuiltinAdd("trace", TRAC);
        BuiltinAdd("trans", TRAN);
    }

    void GenCode::BuiltinAdd(const std::string &name, Instrucitons ins)
    {
        SymbolType sym;
        sym.node = nullptr;
        sym.clazz = ClzBuiltin;
        sym.data = ins;
        builtins_.insert(std::make_pair(name, sym));
    }

    void GenCode::Gen()
    {
        GenRec(root_);
    }

    template<typename T>
    static void AstRecursion(AstNode *node, T func)
    {
        if (node == nullptr)
        { return; }

        auto i = node;
        do
        {
            func(i);
            i = i->next;
        } while(i != node);
    }

    static int SizeType(TokenType type)
    {
        switch (type)
        {
            case TokenType::Char:
                return BaseType<TokenType::Char>::size;
            case TokenType::Uchar:
                return BaseType<TokenType::Uchar>::size;
            case TokenType::Short:
                return BaseType<TokenType::Short>::size;
            case TokenType::Ushort:
                return BaseType<TokenType::Ushort>::size;
            case TokenType::Int:
                return BaseType<TokenType::Int>::size;
            case TokenType::Uint:
                return BaseType<TokenType::Uint>::size;
            case TokenType::Long:
                return BaseType<TokenType::Long>::size;
            case TokenType::Ulong:
                return BaseType<TokenType::Ulong>::size;
            case TokenType::Float:
                return BaseType<TokenType::Float>::size;
            case TokenType::Double:
                return BaseType<TokenType::Double>::size;
            default:
                assert(0);
                break;
        }
    }

    static int Align4(int n)
    {
        if (n % 4 == 0)
        {
            return n;
        }
        return (n | 3) + 1;
    }

    static int SizeId(AstNode *node)
    {
        auto i = 0;
        if (node->prev->data._type.ptr > 0)
        {
            i = BaseType<TokenType::Ptr>::size;
        }
        else
        {
            return SizeType(node->prev->data._type.type);
        }
        return i;
    }

    static int SizeInc(int expr, int ptr)
    {
        if (ptr == 0)
        { return 1; }

        if (ptr == 1)
        { return expr; }

        return BaseType<TokenType::Ptr>::size;
    }

#if GEN_DEBUG

    static std::string TypeStr(AstNode *node)
    {
        static Token tk;
        std::stringstream ss;
        ss << tk.LexerTypeStr(node->prev->data._type.type);
        auto n = node->prev->data._type.ptr;
        for (int i = 0; i < n; ++i)
        {
            ss << '*';
        }

        return ss.str();
    }

#endif


    void GenCode::GenRec(AstNode *node)
    {
        if (node == nullptr)
        { return; }

        auto recFunc = [&](auto n) { this->GenRec(n); };
        auto type = (AstNodeType) node->flag;

//        printf("AstNodeType [%d] : %s\n", node->flag, tok_.AstNodeStr(node->flag).c_str());
        switch (type)
        {
            case AstNodeType::AstRoot:                  // 根结点，全局声明
                AstRecursion(node->child, recFunc);
                break;
            case AstNodeType::AstEnum:
                AstRecursion(node->child, recFunc);
                break;
            case AstNodeType::AstEnumUnit:
                AddSymbol(node->child, ClzEnum, node->child->next->data._int);
                break;
            case AstNodeType::AstVarGlobal:
                AddSymbol(node->child->next, ClzVarGlobal, 0);
                break;
            case AstNodeType::AstVarParam:
                AddSymbol(node->child->next, ClzVarParam, 0);
                break;
            case AstNodeType::AstVarLocal:
                AddSymbol(node->child->next, ClzVarLocal, 0);
                break;
            case AstNodeType::AstFunc:
            {
                printf("---Gen Function %s---\n", node->child->next->data._string);
                auto _node = node->child;        // return Type
                _node = _node->next;             // identifier
                AddSymbol(node->child->next, ClzFunc, Index());
                symbols_.emplace_back();
#if GEN_DEBUG
                auto id = _node->data._string;
                printf("[DEBUG] Func::enter(\"%s\")\n", id);
#endif
                ebp_ = 0;
                _node = _node->next;             // param
                recFunc(_node);
                ebp_ += 4;
                ebpLocal_ = ebp_;
                _node = _node->next;             // block
                recFunc(_node);
#if GEN_DEBUG
                printf("[DEBUG] Func::leave(\"%s\")\n", id);
#endif
                Emit(LEV);
                symbols_.pop_back();
            }
                break;
            case AstNodeType::AstParam:
                AstRecursion(node->child, recFunc);
                break;
            case AstNodeType::AstBlock:
                symbols_.emplace_back();
#if GEN_DEBUG
                printf("[DEBUG] Block::enter\n");
#endif

                AstRecursion(node->child, recFunc);

#if GEN_DEBUG
                printf("[DEBUG] Block::leave\n");
#endif
                symbols_.pop_back();
                break;
            case AstNodeType::AstStmt:
                AstRecursion(node->child, recFunc);
                break;
            case AstNodeType::AstReturn:
#if GEN_DEBUG
                printf("[DEBUG] Func::return\n");
#endif
                if (node->child != nullptr)
                {
                    recFunc(node->child);
                }
                Emit(LEV);
                break;
            case AstNodeType::AstExp:
                recFunc(node->child);
                break;
            case AstNodeType::AstExpParam:
                recFunc(node->child);
                Emit(PUSH);
                break;
            case AstNodeType::AstSinOp:
                if (node->data._op.data == 0)           // 前置
                {
                    switch (node->data._op.op)
                    {
                        case OperatorType::Add:
                            recFunc(node->child);
                            break;
                        case OperatorType::Minus:
                            Emit(IMM, -1);
                            Emit(PUSH);
                            recFunc(node->child);
                            Emit(MUL);
                            break;
                        case OperatorType::Inc:
                        case OperatorType::Dec:
                        {
                            recFunc(node->child);                       // lvalue
                            auto _expr = exprLevel_;
                            auto _ptr = ptrLevel_;
                            auto i = text_.back();
                            Expect(ExpectLvalue, node->child);      // 验证左值
                            EmitTop(PUSH);                          // 改载入指令为压栈指令，将左值地址压栈
                            Emit(i);                                    // 取出左值
                            Emit(PUSH);                             // 压入左值
                            Emit(IMM, SizeInc(_expr, _ptr));
                            Emit(tok_.Op2Ins(node->data._op.op));
                            Emits((Instrucitons) i);                        // 存储指令
                        }
                            break;
                        case OperatorType::LogicalNot:
                            recFunc(node->child);           // lvalue
                            Emit(PUSH);
                            Emit(IMM, 0);
                            Emit(EQ);
                            break;
                        case OperatorType::BitNot:
                            recFunc(node->child);            // lvalue
                            Emit(PUSH);
                            Emit(IMM, -1);
                            Emit(XOR);
                            break;
                        case OperatorType::BitAnd:          // 取地址
                            recFunc(node->child);           // lvalue
                            Expect(ExpectLvalue, node->child); // 验证左值
                            EmitPop();              // 去掉一个读取指令
                            ptrLevel_++;
                            break;
                        case OperatorType::Mul:         // 解引用
                            recFunc(node->child); // exp
                            EmitDeref();
                            if (ptrLevel_ > 0)
                            {
                                ptrLevel_--;
                            }
                            break;
                        default:
                            printf("ast_sinop::unsupported prefix op \"%s\"\n", tok_.OpStr(node->data._op.op).c_str());
                            throw std::exception();
                    }
                }
                else                                    // 后置
                {
                    switch (node->data._op.op)
                    {
                        case OperatorType::Inc:
                        case OperatorType::Dec:
                        {
                            recFunc(node->child); // lvalue
                            auto _expr = exprLevel_;
                            auto _ptr = ptrLevel_;
                            auto i = text_.back();
                            Expect(ExpectLvalue, node->child); // 验证左值
                            EmitTop(PUSH); // 改载入指令为压栈指令，将左值地址压栈
                            Emit(i); // 取出左值
                            Emit(PUSH); // 压入左值
                            Emit(IMM, SizeInc(_expr, _ptr)); // 增量到ax
                            auto ins = tok_.Op2Ins(node->data._op.op);
                            Emit(ins);
                            Emits((Instrucitons) i); // 存储指令，变量未修改
                            Emit(PUSH); // 压入当前值
                            Emit(IMM, SizeInc(_expr, _ptr)); // 增量到ax
                            Emit(ins == ADD ? SUB : ADD); // 仅修改ax
                        }
                            break;
                        default:
                            printf("AstSinOp::unsupported postfix op \"%s\"\n", tok_.OpStr(node->data._op.op).c_str());
                            throw std::exception();
                    }
                }
                break;
            case AstNodeType::AstBinOp:
            {
                if (node->data._op.op == OperatorType::Lsquare)
                {
                    recFunc(node->child); // exp
                    auto _expr = exprLevel_;
                    auto _ptr = ptrLevel_;
                    if (_ptr == 0)
                    {
                        Expect(ExpectPointer, node->child);
                    }
                    Emit(PUSH); // 压入数组地址
                    recFunc(node->child->next); // index
                    auto n = SizeInc(_expr, _ptr);
                    if (n > 1)
                    {
                        Emit(PUSH);
                        Emit(IMM, n);
                        Emit(MUL);
                        Emit(ADD);
                        Emit(LI);
                    }
                    else
                    {
                        Emit(ADD);
                        Emit(LC);
                    }
                    exprLevel_ = _expr;
                    ptrLevel_ = _ptr - 1;
                }
                else
                { // 二元运算
                    switch (node->data._op.op)
                    {
                        case OperatorType::Equal:
                        case OperatorType::Mul:
                        case OperatorType::Divide:
                        case OperatorType::BitAnd:
                        case OperatorType::BitOr:
                        case OperatorType::BitXor:
                        case OperatorType::Mod:
                        case OperatorType::LessThan:
                        case OperatorType::LessThanOrEqual:
                        case OperatorType::GreaterThan:
                        case OperatorType::GreaterThanOrEqual:
                        case OperatorType::NotEqual:
                        case OperatorType::LeftShift:
                        case OperatorType::RightShift:
                        {
                            recFunc(node->child); // exp1
                            auto _expr = exprLevel_;
                            auto _ptr = ptrLevel_;
                            Emit(PUSH);
                            recFunc(node->child->next); // exp2
                            auto _expr2 = exprLevel_;
                            auto _ptr2 = ptrLevel_;
                            Emit(tok_.Op2Ins(node->data._op.op));
                            // 后面会详细做静态分析
                            exprLevel_ = std::max(_expr, _expr2);
                            ptrLevel_ = std::max(_ptr, _ptr2);
                        }
                            break;
                        case OperatorType::LogicalAnd:
                        case OperatorType::LogicalOr:
                        {
                            recFunc(node->child); // exp1
                            auto a = EmitOp(tok_.Op2Ins(node->data._op.op)); // 短路优化
                            recFunc(node->child->next); // exp2
                            EmitOp(Index(), a); // a = exit
                            exprLevel_ = 4;
                            ptrLevel_ = 0;
                        }
                            break;
                        case OperatorType::Assign:
                        {
                            recFunc(node->child); // lvalue
                            auto _expr = exprLevel_;
                            auto _ptr = ptrLevel_; // 保存静态分析类型
                            auto i = text_.back();
                            Expect(ExpectLvalue, node->child); // 验证左值
                            EmitTop(PUSH); // 改载入指令为压栈指令，将左值地址压栈
                            recFunc(node->child->next); // rvalue
                            Emits((Instrucitons) i); // 存储指令
                            exprLevel_ = _expr;
                            ptrLevel_ = _ptr; // 还原静态分析类型
                        }
                            break;
                        case OperatorType::Add:
                        case OperatorType::Minus:
                        {
                            recFunc(node->child); // exp1
                            auto _expr = exprLevel_;
                            auto _ptr = ptrLevel_;
                            Emit(PUSH);
                            recFunc(node->child->next); // exp2
                            auto _expr2 = exprLevel_;
                            auto _ptr2 = ptrLevel_;
                            if (_ptr > 0 && _ptr2 == 0)
                            { // 指针+常量
                                if (_expr > 1)
                                {
                                    Emit(PUSH);
                                    Emit(IMM);
                                    Emit(_expr);
                                    Emit(MUL);
                                }
                            }
                            Emit(tok_.Op2Ins(node->data._op.op));
                            exprLevel_ = std::max(_expr, _expr2);
                            ptrLevel_ = std::max(_ptr, _ptr2);
                        }
                            break;
                        case OperatorType::AddAssign:
                        case OperatorType::MinusAssign:
                        case OperatorType::MulAssign:
                        case OperatorType::DivAssign:
                        case OperatorType::AndAssign:
                        case OperatorType::OrAssign:
                        case OperatorType::XorAssign:
                        case OperatorType::ModAssign:
                        case OperatorType::LeftShiftAssign:
                        case OperatorType::RightShiftAssign:
                        {
                            recFunc(node->child); // lvalue
                            auto _expr = exprLevel_;
                            auto _ptr = ptrLevel_; // 保存静态分析类型
                            auto i = text_.back();
                            Expect(ExpectLvalue, node->child); // 验证左值
                            EmitTop(PUSH); // 改载入指令为压栈指令，将左值地址压栈
                            Emit(i); // 取出左值
                            Emit(PUSH);
                            recFunc(node->child->next); // rvalue
                            Emit(tok_.Op2Ins(node->data._op.op)); // 进行二元操作
                            Emits((Instrucitons) i); // 存储指令
                            exprLevel_ = _expr;
                            ptrLevel_ = _ptr; // 还原静态分析类型
                        }
                            break;
                        default:
                            printf("ast_binop::unsupported op \"%s\"\n", tok_.OpStr(node->data._op.op).c_str());
                            throw std::exception();
                    }
                }
            }
                break;
            case AstNodeType::AstTriOp:
                if (node->data._op.op == OperatorType::Query)
                {
                    recFunc(node->child); // cond
                    auto a = EmitOp(JZ);
                    recFunc(node->child->next); // true
                    auto b = EmitOp(JMP);
                    EmitOp(Index(), a);
                    recFunc(node->child->prev); // false
                    EmitOp(Index(), b);
                }
                break;
            case AstNodeType::AstIf:
            {
                //
                // if (...) <statement> [else <statement>]
                //
                //   if (...)           <cond>
                //                      JZ a
                //     <statement>      <statement>
                //   else:              JMP b
                // a:
                //     <statement>      <statement>
                // b:                   b:
                //
                recFunc(node->child); // if exp
                if (node->child->next == node->child->prev)
                { // 没有else
                    auto b = EmitOp(JZ); // JZ b 条件不满足时跳转到出口
                    recFunc(node->child->next); // if stmt
                    EmitOp(Index(), b); // b = 出口
                }
                else
                { // 有else
                    auto a = EmitOp(JZ); // JZ a 条件不满足时跳转到else块之前
                    recFunc(node->child->next); // if stmt
                    auto _else = node->child->prev;
                    auto b = EmitOp(JMP); // b = true stmt，条件true时运行至此，到达出口
                    EmitOp(Index(), a); // JZ a 跳转至此，为else块之前
                    recFunc(_else); // else stmt
                    EmitOp(Index(), b); // b = 出口
                }
            }
                break;
            case AstNodeType::AstWhile:
            {
                //
                // a:                     a:
                //    while (<cond>)        <cond>
                //                          JZ b
                //     <statement>          <statement>
                //                          JMP a
                // b:                     b:
                //
                auto a = Index(); // a = 循环起始
                recFunc(node->child); // cond
                auto b = EmitOp(JZ); // JZ b
                recFunc(node->child->next); // true stmt
                Emit(JMP, a); // JMP a
                EmitOp(Index(), b); // b = 出口
                recFunc(node->child);
            }
                break;
            case AstNodeType::AstInvoke:
            {
                printf("Gen Function Call: %s\n", node->data._string);
                auto sym = FindSymbol(node->data._string);
#if 0
                printf("[DEBUG] Id::Invoke(\"%s\", %s)\n", node->data._string, ClassStr(sym.clazz).c_str());
#endif
                if (sym.clazz == ClzFunc)
                { // 定义的函数
                    AstRecursion(node->child, recFunc); // param
                    Emit(CALL, sym.data); // call func addr
                }
                else if (sym.clazz == ClzBuiltin)
                { // 内建函数
                    AstRecursion(node->child, recFunc); // param
                    Emit((Instrucitons) sym.data); // builtin-inst
                }
                else
                { // 非法
                    Expect(ExpectValidId, node);
                }
                auto n = AST::ChildrenSize(node); // param count
                if (n > 0)
                { // 清除参数
                    Emit(ADJ, n);
                }
            }
                break;

            case AstNodeType::AstEmpty:
            {
                if (node->data._int == 1)
                {
#if GEN_DEBUG
                    printf("[DEBUG] Func::----\n");
#endif
                    Emit(ENT, ebpLocal_ - ebp_);
                }
            }
                break;

            case AstNodeType::AstId:
            {
                auto sym = FindSymbol(node->data._string);
                switch (sym.clazz)
                {
                    case ClzEnum: // 枚举
                        Emit(IMM, sym.data); // 替换成常量
                        exprLevel_ = 4;
                        ptrLevel_ = 0;
#if GEN_DEBUG
                        printf("[DEBUG] Id::Enum(\"%s\", %s, IMM %d)\n", node->data._string,
                               ClassStr(sym.clazz).c_str(), sym.data);
#endif
                        break;
                    case ClzVarGlobal:
                        Emit(IMM, sym.data);
                        Emit(LOAD); // 载入data段数据
                        Emitl(sym.node);
                        CalcLevel(sym.node); // 静态分析类型
#if GEN_DEBUG
                        printf("[DEBUG] Id::Global(\"%s\", %s, LOAD %d)\n", node->data._string,
                               ClassStr(sym.clazz).c_str(), sym.data);
#endif
                        break;
                    case ClzVarParam:
                        Emit(LEA, ebp_ - sym.data);
                        Emitl(sym.node);
                        CalcLevel(sym.node); // 静态分析类型
#if GEN_DEBUG
                        printf("[DEBUG] Id::Param(\"%s\", %s, LEA %d)\n", node->data._string,
                               ClassStr(sym.clazz).c_str(), ebp_ - sym.data);
#endif
                        break;
                    case ClzVarLocal:
                        Emit(LEA, ebp_ - sym.data);
                        Emitl(sym.node);
                        CalcLevel(sym.node); // 静态分析类型
#if GEN_DEBUG
                        printf("[DEBUG] Id::Local(\"%s\", %s, LEA %d)\n", node->data._string,
                               ClassStr(sym.clazz).c_str(), ebp_ - sym.data);
#endif
                        break;
                    default: // 非法
                        Expect(ExpectValidId, node);
                        break;
                }
            }
                break;
            case AstNodeType::AstType:
                break;
            case AstNodeType::AstCast:
                recFunc(node->child);
                exprLevel_ = SizeType(node->data._type.type); // 修正静态分析类型
                ptrLevel_ = node->data._type.ptr;
                break;
            case AstNodeType::AstChar:
            case AstNodeType::AstUchar:
            case AstNodeType::AstShort:
            case AstNodeType::AstUshort:
            case AstNodeType::AstInt:
            case AstNodeType::AstUint:
            case AstNodeType::AstLong:
            case AstNodeType::AstUlong:
            case AstNodeType::AstFloat:
            case AstNodeType::AstDouble:
            case AstNodeType::AstString:
                Emit(node);
                break;
        }

    }

    void GenCode::AddSymbol(AstNode *node, ClassT clazz, int addr)
    {

        Expect(ExpectNonConflictId, node);
        SymbolType sym{.node = node, .clazz = clazz, .data = addr};

        switch (clazz)
        {
            case ClzEnum:
#if GEN_DEBUG
                printf("[DEBUG] Symbol::add(\"%s\", %s, %d)\n", node->data._string, ClassStr(clazz).c_str(), addr);
#endif
                break;
            case ClzNumber:
                break;
            case ClzFunc:
                break;
            case ClzVarGlobal:
            {
                // 得到变量size
                auto n = SizeId(node);
                sym.data = (int) data_.size();
                for (int i = 0; i < n; ++i)
                {
                    data_.push_back(0);
                }

#if GEN_DEBUG
                printf("[DEBUG] Symbol::add(\"%s %s\", %s, %d)\n", TypeStr(node).c_str(), node->data._string,
                       ClassStr(clazz).c_str(), addr);
                printf("[DEBUG] Symbol::var_global(used: %d, now: %d)\n", n, data_.size());
#endif
            }
                break;

            case ClzVarParam:           // 形参
            {
                sym.data = ebp_;
                auto n = Align4(SizeId(node));
                ebp_ += n;
#if GEN_DEBUG
                printf("[DEBUG] Symbol::add(\"%s %s\", %s, %d)\n", TypeStr(node).c_str(), node->data._string,
                       ClassStr(clazz).c_str(), addr);
                printf("[DEBUG] Symbol::var_param(used: %d, now: %d)\n", n, ebp_);
#endif
            }
                break;
            case ClzVarLocal:       // 局部变量 栈
            {
                auto n = Align4(SizeId(node));
                ebpLocal_ += n;
                sym.data = ebpLocal_;
#if GEN_DEBUG
                printf("[DEBUG] Symbol::add(\"%s %s\", %s, %d)\n", TypeStr(node).c_str(), node->data._string,
                       ClassStr(clazz).c_str(), addr);
                printf("[DEBUG] Symbol::var_local(used: %d, now: %d)\n", n, ebpLocal_);
#endif
                break;
            }
            default:
                break;
        }
//        printf("-- Add Symbol-> %s--\n", node->data._string);
        symbols_.back().insert(std::make_pair(node->data._string, sym));
    }

    void GenCode::Expect(ExpectType type, AstNode *node)
    {
        switch (type)
        {
            case ExpectNonConflictId:
                if (ConflictSymbol(node->data._string))
                {
                    printf("duplicate id: \"%s\"\n", node->data._string);
                    throw std::exception();
                }
                break;
            case ExpectValidId:
            {
                printf("undefined id: \"%s\"\n", node->data._string);
                throw std::exception();
            }
            case ExpectLvalue:
                if (text_.back() != LC && text_.back() != LI)
                {
                    std::stringstream ss;
                    AST::Print(node, 0, ss);
                    printf("invalid lvalue: \"%s\"\n", ss.str().c_str());
                    throw std::exception();
                }
                break;
            case ExpectPointer:
            {
                std::stringstream ss;
                AST::Print(node, 0, ss);
                printf("not a pointer: \"%s\"\n", ss.str().c_str());
                throw std::exception();
            }
        }
    }

    bool GenCode::ConflictSymbol(const std::string &str)
    {
        // 内建函数冲突
        if (builtins_.find(str) != builtins_.end())
        { return true; }

        if (symbols_.back().find(str) != symbols_.back().end())
        {
            printf("find\n");
            return false;
        }

        for (auto i = symbols_.rbegin() + 1; i != symbols_.rend(); i++)
        {
            auto f = i->find(str);
            if (f != i->end())
            {
                // 允许变量名覆盖
                if (f->second.clazz == ClzVarGlobal || f->second.clazz == ClzVarLocal)
                { continue; }

                return true;
            }
        }

        return false;
    }

    void GenCode::Emit(GenCode::InsType ins)
    {
        text_.push_back(ins);
    }

    void GenCode::Emit(GenCode::InsType ins, GenCode::OpType op)
    {
        text_.push_back(ins);
        text_.push_back(op);
    }

    void GenCode::EmitTop(GenCode::InsType ins)
    {
        text_.back() = ins;
    }

    void GenCode::EmitPop()
    {
        text_.pop_back();
    }

    BaseType<TokenType::Int>::type GenCode::EmitOp(GenCode::InsType ins)
    {
        text_.push_back(ins);
        auto i = Index();
        text_.push_back(0U);
        return i;
    }

    void GenCode::EmitOp(GenCode::InsType ins, int index)
    {
        text_[index] = ins;
    }

    void GenCode::Emit(AstNode *node)
    {
        switch ((AstNodeType) node->flag)
        {
            case AstNodeType::AstChar:
                Emit(IMM, (int) (node->data._char));
                exprLevel_ = BaseType<TokenType::Char>::size;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstUchar:
                Emit(IMM, (int) (node->data._uchar));
                exprLevel_ = BaseType<TokenType::Uchar>::size;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstShort:
                Emit(IMM, (int) (node->data._short));
                exprLevel_ = BaseType<TokenType::Short>::size;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstUshort:
                Emit(IMM, (int) (node->data._ushort));
                exprLevel_ = BaseType<TokenType::Ushort>::size;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstInt:
                Emit(IMM, (int) (node->data._int));
                exprLevel_ = BaseType<TokenType::Int>::size;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstUint:
                Emit(IMM, (int) (node->data._uint));
                exprLevel_ = BaseType<TokenType::Uint>::size;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstFloat:
                Emit(IMM, (int) (node->data._float));
                exprLevel_ = BaseType<TokenType::Float>::size;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstLong:
            case AstNodeType::AstUlong:
            case AstNodeType::AstDouble:
                Emit(IMX); // 载入8字节
                Emit(node->data._ins._1);
                Emit(node->data._ins._2);
                exprLevel_ = 8;
                ptrLevel_ = 0;
                break;
            case AstNodeType::AstString:
            {
                auto addr = data_.size();
                auto s = node->data._string;
                while(*s)
                {
                    data_.push_back(*s++); // 拷贝字符串至data段
                }
                data_.push_back(0);
                auto idx = data_.size() % 4;
                if (idx != 0)
                {
                    idx = 4 - idx;
                    for (auto i = 0; i < idx; ++i)
                    {
                        data_.push_back(0); // 对齐
                    }
                }
#if GEN_DEBUG
                printf("[DEBUG] Id::String(\"%s\", %d-%d)\n", AST::DisplayStr(node).c_str(), addr, data_.size() - 1);
#endif
                Emit(IMM, addr);
                Emit(LOAD); // 载入data段指令
                exprLevel_ = 1;
                ptrLevel_ = 1;
            }
                break;
            default:
                printf("Emit::unsupported type\n");
                throw std::exception();
                break;
        }
    }

    void GenCode::Emit(AstNode *node, int ebp)
    {
        Emit(ebp - node->data._int);
    }

    void GenCode::EmitDeref()
    {
        if (ptrLevel_ == 1)
        {
            switch (exprLevel_)
            {
                case 1:
                    Emit(LC);
                    break;
                case 4:
                    Emit(LI);
                    break;
                default:
                    printf("emit_deref::unsupported type\n");
                    throw std::exception();
            }
        }
        else   // > 1 and ...
        {
            Emit(LI);
        }
    }

    void GenCode::Emitl(AstNode *node)
    {
        auto n = SizeId(node);
        switch (n)
        {
            case 1:
                Emit(LC);
                break;
            case 4:
                Emit(LI);
                break;
            default:
                assert(!"unsupported type");
                break;
        }
    }

    void GenCode::Emits(Instrucitons ins)
    {
        switch (ins)
        {
            case LC:
                Emit(SC);
                break;
            case LI:
                Emit(SI);
                break;
            default:
                assert(!"unsupported type");
                break;
        }
    }

    SymbolType GenCode::FindSymbol(const std::string &str)
    {
        // 找内建函数
        auto f = builtins_.find(str);
        if (f != builtins_.end())
        {
            return f->second;
        }
        // 找符号表
        for (auto i = symbols_.rbegin(); i != symbols_.rend(); i++)
        {
            f = i->find(str);
            if (f != i->end())
            {
                return f->second;
            }
        }
        return SymbolType{.node = nullptr, .clazz = ClzNotFound, .data = 0};
    }

    void GenCode::CalcLevel(AstNode *node)
    {
        ptrLevel_ = node->prev->data._type.ptr;
        switch (node->prev->data._type.type)
        {
            case TokenType::Char:
                exprLevel_ = BaseType<TokenType::Char>::size;
                break;
            case TokenType::Uchar:
                exprLevel_ = BaseType<TokenType::Uchar>::size;
                break;
            case TokenType::Short:
                exprLevel_ = BaseType<TokenType::Short>::size;
                break;
            case TokenType::Ushort:
                exprLevel_ = BaseType<TokenType::Ushort>::size;
                break;
            case TokenType::Int:
                exprLevel_ = BaseType<TokenType::Int>::size;
                break;
            case TokenType::Uint:
                exprLevel_ = BaseType<TokenType::Uint>::size;
                break;
            case TokenType::Long:
                exprLevel_ = BaseType<TokenType::Long>::size;
                break;
            case TokenType::Ulong:
                exprLevel_ = BaseType<TokenType::Ulong>::size;
                break;
            case TokenType::Float:
                exprLevel_ = BaseType<TokenType::Float>::size;
                break;
            case TokenType::Double:
                exprLevel_ = BaseType<TokenType::Double>::size;
                break;
            default:
                assert(0);
                break;
        }

    }

    void GenCode::Eval()
    {
        // find the main function that is start of execution.
        auto entry = symbols_[0].find("main");
        if (entry == symbols_[0].end())
        {
            printf("main() not defined\n");
            throw std::exception();
        }
//        for (const auto& it : text_)
//        {
//            std::cout <<  it << " " ;
//        }
//        printf("\n----------data:---------\n");
//        for (const auto& it : data_)
//        {
//            std::cout <<  it << " " ;
//        }
//        std::cout << std::endl;
        VM vm(text_, data_);
        vm.Exec(entry->second.data);
    }
}
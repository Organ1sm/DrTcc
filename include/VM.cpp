//
// Created by yw.
//

#include "VM.h"
#include "GenCode.h"

int globalArgc;
char **globalArgv;
#define VM_DEBUG 0
#define INSTRUCTION_DEBUG  0

namespace DrTcc
{
#define INC_PTR 4
#define VMM_ARG(s, p) ((s) + p * INC_PTR)
#define VMM_ARGS(t, n) VmmGet(t - (n) * INC_PTR)


    VM::VM(const std::vector<TextType> &text, const std::vector<DataType> &data)
    {
        VmmInit();
        uint32_t pa;        // physical address

        // 映射 4KB的代码空间
        {
            auto size = PAGE_SIZE / sizeof(int);
            for (uint32_t i = 0, start = 0; start < text.size(); ++i, start += size)
            {
                VmmMap(USER_BASE + PAGE_SIZE * i, (uint32_t) PmmAlloc(), PTE_U | PTE_P | PTE_R); // 用户代码空间
                if (VmmIsmap(USER_BASE + PAGE_SIZE * i, &pa))
                {
                    auto s = start + size > text.size() ? (text.size() & (size - 1)) : size;
                    for (uint32_t j = 0; j < s; ++j)
                    {
                        *((uint32_t *) pa + j) = (uint) text[start + j];
#if VM_DEBUG
                        printf("Text:[%p]> [%08X] %08X\n", (int *) pa + j, USER_BASE + PAGE_SIZE * i + j * 4,
                               VmmGet<uint32_t>(USER_BASE + PAGE_SIZE * i + j * 4));
#endif
                    }
                }
            }
        }

        /* 映射4KB的数据空间 */
        {
            auto size = PAGE_SIZE;
            for (uint32_t i = 0, start = 0; start < data.size(); ++i, start += size)
            {
                VmmMap(DATA_BASE + PAGE_SIZE * i, (uint32_t) PmmAlloc(), PTE_U | PTE_P | PTE_R); // 用户数据空间
                if (VmmIsmap(DATA_BASE + PAGE_SIZE * i, &pa))
                {
                    auto s = start + size > data.size() ? ((sint) data.size() & (size - 1)) : size;
                    for (uint32_t j = 0; j < s; ++j)
                    {
                        *((char *) pa + j) = data[start + j];
#if VM_DEBUG
                        printf("Data:[%p]> [%08X] %d\n", (char *) pa + j, DATA_BASE + PAGE_SIZE * i + j,
                               VmmGet<byte>(DATA_BASE + PAGE_SIZE * i + j));
#endif
                    }
                }
            }
        }


        /* 映射4KB的栈空间 */
        VmmMap(STACK_BASE, (uint32_t) PmmAlloc(), PTE_U | PTE_P | PTE_R); // 用户栈空间
        /* 映射16KB的堆空间 */
        {
            auto head = heap_.AllocArray<byte>(PAGE_SIZE * (HEAP_SIZE + 2));
#if VM_DEBUG
            printf("HEAP> ALLOC=%p\n", head);
#endif
            heapHead = head; // 得到内存池起始地址
            heap_.FreeArray(heapHead);
            heapHead = (byte *) PAGE_ALIGN_UP((uint32_t) head);
#if VM_DEBUG
            printf("HEAP> HEAD=%p\n", heapHead);
#endif
            memset(heapHead, 0, PAGE_SIZE * HEAP_SIZE);
            for (int i = 0; i < HEAP_SIZE; ++i)
            {
                VmmMap(HEAP_BASE + PAGE_SIZE * i, (uint32_t) heapHead + PAGE_SIZE * i, PTE_U | PTE_P | PTE_R);
            }
        }
    }

    VM::~VM()
    {
        free(pgd_kern);
        free(pte_kern);
    }

    void VM::VmmInit()
    {
        // 给内核页表分配 PTE_SIZE * 4 字节内存
        // 4 * 1024
        pgd_kern = (pde_t *) malloc(PTE_SIZE * sizeof(pde_t));
        memset(pgd_kern, 0, PTE_SIZE * sizeof(pde_t));

        // 分配PTE_COUNT * PTE_SIZE * sizeof(pte_t)字节内存
        pte_kern = (pte_t *) malloc(PTE_COUNT * PTE_SIZE * sizeof(pte_t));
        memset(pte_kern, 0, PTE_COUNT * PTE_SIZE * sizeof(pte_t));

        pageDir = pgd_kern;

        uint32_t i;
        // map 4G memory, physcial address = virtual address
        for (i = 0; i < PTE_SIZE; i++)
        {
            pgd_kern[i] = (uint32_t) pte_kern[i] | PTE_P | PTE_R | PTE_K;
        }

        uint32_t *pte = (uint32_t *) pte_kern;
        for (i = 1; i < PTE_COUNT * PTE_SIZE; i++)
        {
            pte[i] = (i << 12) | PTE_P | PTE_R | PTE_K; // i是页表号
        }
    }

    // 虚页映射
    // va = 虚拟地址  pa = 物理地址
    void VM::VmmMap(uint32_t va, uint32_t pa, uint32_t flags)
    {
        uint32_t pdeIndex = PDE_INDEX(va);   // 页目录号
        uint32_t pteIndex = PTE_INDEX(va);   // 页表号

        // 找到页表
        auto pte = (pte_t *) (pageDir[pdeIndex] & PAGE_MASK);

        if (!pte)
        { // 缺页
            if (va >= USER_BASE)            // 用户地址即转换
            {
                // 申请页框，用作新页表
                pte = (pte_t *) PmmAlloc();
                // 设置页表
                pageDir[pdeIndex] = (uint32_t) pte | PTE_P | flags;
                // 设置页表项
                pte[pteIndex] = (pa & PAGE_MASK) | PTE_P | flags;
            }
            else
            {
                // 取得内核页表
                pte = (pte_t *) (pgd_kern[pdeIndex] & PAGE_MASK);
                // 设置页表
                pageDir[pdeIndex] = (uint32_t) pte | PTE_P | flags;
            }
        }
        else
        {
            // pte存在
            pte[pteIndex] = (pa & PAGE_MASK) | PTE_P | flags;
        }
#if VM_DEBUG
        printf("MEMMAP> V=%08X P=%08X\n", va, pa);
#endif
    }

    uint32_t VM::PmmAlloc()
    {
        auto ptr = (uint32_t) memory_.AllocArray<byte>(PAGE_SIZE * 2);
        auto page = PAGE_ALIGN_UP(ptr);
        memset((void *) page, 0, PAGE_SIZE);

        return page;
    }

    void VM::VmmUnmap(pde_t *pde, uint32_t va)
    {
        uint32_t pdeIndex = PDE_INDEX(va);
        uint32_t pteIndex = PTE_INDEX(va);

        auto *pte = (pte_t *) (pde[pdeIndex] & PAGE_MASK);

        if (!pte)
        { return; }

        pte[pteIndex] = 0; // 清空页表项，此时有效位为零
    }

    int VM::VmmIsmap(uint32_t va, uint32_t *pa) const
    {
        uint32_t pdeIndex = PDE_INDEX(va);
        uint32_t pteIndex = PTE_INDEX(va);

        pte_t *pte = (pte_t *) (pageDir[pdeIndex] & PAGE_MASK);
        if (!pte)
        {
            return 0; // 页表不存在
        }
        if (pte[pteIndex] != 0 && (pte[pteIndex] & PTE_P) && pa)
        {
            *pa = pte[pteIndex] & PAGE_MASK; // 计算物理页面
            return 1; // 页面存在
        }
        return 0; // 页表项不存在
    }

    template<class T>
    T VM::VmmGet(uint32_t va)
    {
        uint32_t pa;
        if (VmmIsmap(va, &pa))
        {
            return *(T *) ((byte *) pa + OFFSET_INDEX(va));
        }
        VmmMap(va, PmmAlloc(), PTE_U | PTE_P | PTE_R);
#if VM_DEBUG
        printf("VMMGET> Invalid VA: %08X\n", va);
#endif
        throw std::exception();
        return VmmGet<T>(va);
    }

    template<class T>
    T VM::VmmSet(uint32_t va, T value)
    {

        uint32_t pa;
        if (VmmIsmap(va, &pa))
        {
            *(T *) ((byte *) pa + OFFSET_INDEX(va)) = value;
            return value;
        }
        VmmMap(va, PmmAlloc(), PTE_U | PTE_P | PTE_R);
#if VM_DEBUG
        printf("VMMSET> Invalid VA: %08X\n", va);
#endif
        throw std::exception();
        return VmmSet(va, value);
    }

    char *VM::VmmGetStr(uint32_t va)
    {
        uint32_t pa;
        if (VmmIsmap(va, &pa))
        {
            return (char *) pa + OFFSET_INDEX(va);
        }
        VmmMap(va, PmmAlloc(), PTE_U | PTE_P | PTE_R);
#if VM_DEBUG
        printf("VMMSTR> Invalid VA: %08X\n", va);
#endif
        assert(0);
        return VmmGetStr(va);
    }

    void VM::VmmSetStr(uint32_t va, const char *value)
    {
        auto len = strlen(value);
        for (uint32_t i = 0; i < len; i++)
        {
            VmmSet(va + i, value[i]);
        }
        VmmSet(va + len, '\0');
    }

    uint32_t VM::VmmMalloc(uint32_t size)
    {
#if VM_DEBUG
        printf("MALLOC> Available: %08X\n", heap_.Available() * 0x10);
#endif
        auto ptr = heap_.AllocArray<byte>(size);
        if (ptr == nullptr)
        {
            printf("111\n");
            printf("out of memory");
            exit(-1);
        }

        if (ptr < heapHead)
        {
            heap_.AllocArray<byte>(heapHead - ptr);

#if VM_DEBUG
            printf("MALLOC> Skip %08X bytes\n", heapHead - ptr);
#endif

            return VmmMalloc(size);
        }
        if (ptr + size >= heapHead + HEAP_SIZE * PAGE_SIZE)
        {
            printf("000\n");
            printf("out of memory");
            exit(-1);
        }
        auto va = VmmPa2va(HEAP_BASE, HEAP_SIZE, ((uint32_t) ptr - (uint32_t) heapHead));

#if VM_DEBUG
        printf("MALLOC> V=%08X P=%p> %08X bytes\n", va, ptr, size);
#endif

        return va;
    }

    uint32_t VM::VmmMemcmp(uint32_t src, uint32_t dst, uint32_t count)
    {
        for (uint32_t i = 0; i < count; i++)
        {
#if VM_DEBUG
            printf("MEMCMP> '%c':'%c'\n", VmmGet<byte>(src + i), VmmGet<byte>(dst + i));
#endif
            if (VmmGet<byte>(src + i) > VmmGet<byte>(dst + i))
            {
                return 1;
            }
            if (VmmGet<byte>(src + i) < VmmGet<byte>(dst + i))
            {
                return -1;
            }
        }
        return 0;
    }

    uint32_t VM::VmmMemset(uint32_t va, uint32_t value, uint32_t count)
    {
#if VM_DEBUG
        uint32_t pa;
        if (VmmIsmap(va, &pa))
        {
            pa |= OFFSET_INDEX(va);
            printf("MEMSET> V=%08X P=%08X S=%08X\n", va, pa, count);
        }
        else
        {
            printf("MEMSET> V=%08X P=ERROR S=%08X\n", va, count);
        }
#endif
        for (uint32_t i = 0; i < count; i++)
        {
#if VM_DEBUG
            printf("MEMSET> V=%08X\n", va + i);
#endif
            VmmSet<byte>(va + i, value);
        }
        return 0;
    }

    template<class T>
    void VM::VmmPushStack(uint32_t &sp, T value)
    {
        sp -= sizeof(T);
        VmmSet(sp, value);
    }

    template<class T>
    T VM::VmmPopStack(uint32_t &sp)
    {
        T t = VmmGet(sp);
        sp += sizeof(T);
        return t;
    }

    void VM::InitArgs(uint32_t *args, uint32_t sp, uint32_t pc, bool converted)
    {
        auto num = VmmGet(pc + INC_PTR); /* 利用之后的ADJ清栈指令知道函数调用的参数个数 */
        auto tmp = VMM_ARG(sp, num);
        for (int k = 0; k < num; k++)
        {
            auto arg = VMM_ARGS(tmp, k + 1);
            if (converted && (arg & DATA_BASE))
            {
                args[k] = (uint32_t) VmmGetStr(arg);
            }
            else
            {
                args[k] = (uint32_t) arg;
            }
        }
    }

    int VM::Exec(int entry)
    {
        auto poolSize = PAGE_SIZE;
        auto stack = STACK_BASE;
        auto data = DATA_BASE;
        auto base = USER_BASE;

        auto sp = stack + poolSize; // 4KB / sizeof(int) = 1024

        {
            auto argvs = VmmMalloc(globalArgc * INC_PTR);
            for (auto i = 0; i < globalArgc; i++)
            {
                auto str = VmmMalloc(256);
                VmmSetStr(str, globalArgv[i]);
                VmmSet(argvs + INC_PTR * i, str);
            }

            VmmPushStack(sp, EXIT);
            VmmPushStack(sp, PUSH);
            auto tmp = sp;
            VmmPushStack(sp, globalArgc);
            VmmPushStack(sp, argvs);
            VmmPushStack(sp, tmp);
        }

        auto pc = USER_BASE + entry * INC_PTR;
        auto ax = 0;
        auto bp = 0;
        bool log = false;

#if 1
        if (log)
        {
            printf("\n---------------- STACK BEGIN <<<< \n");
            printf("AX: %08X BP: %08X SP: %08X\n", ax, bp, sp);
            for (uint32_t i = sp; i < STACK_BASE + PAGE_SIZE; i += 4)
            {
                printf("[%08X]> %08X\n", i, VmmGet<uint32_t>(i));
            }
            printf("---------------- STACK END >>>>\n\n");
        }
#endif

        auto cycle = 0;
        uint32_t args[6];
        while(true)
        {
            cycle++;
            auto op = VmmGet(pc); // get next operation code
            pc += INC_PTR;

#if INSTRUCTION_DEBUG
            assert(op <= EXIT);
            // print debug info
            if (true)
            {
                printf("%04d> [%08X] %02d %.4s", cycle, pc, op,
                       &"NOP, LEA ,IMM ,IMX ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,SI  ,LC  ,SC  ,PUSH,LOAD,"
                        "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                        "OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,TRAC,TRAN,EXIT"[op * 5]);
                if (op == PUSH)
                {
                    printf(" %08X\n", (uint32_t) ax);
                }
                else if (op <= ADJ)
                {
                    printf(" %d\n", VmmGet(pc));
                }
                else
                {
                    printf("\n");
                }
            }
#endif
            switch (op)
            {
                case IMM:
                {
                    ax = VmmGet(pc);
                    pc += INC_PTR;
                } /* load immediate value to ax */
                    break;
                case LI:
                {
                    ax = VmmGet(ax);
                } /* load integer to ax, address in ax */
                    break;
                case SI:
                {
                    VmmSet(VmmPopStack(sp), ax);
                } /* save integer to address, value in ax, address on stack */
                    break;
                case LC:
                {
                    ax = VmmGet<byte>(ax);
                } /* load integer to ax, address in ax */
                    break;
                case SC:
                {
                    VmmSet<byte>(VmmPopStack(sp), ax & 0xff);
                } /* save integer to address, value in ax, address on stack */
                    break;
                case LOAD:
                {
                    ax = data | ((ax) & (PAGE_SIZE - 1));
                } /* load the value of ax, segment = DATA_BASE */
                    break;
                case PUSH:
                {
                    VmmPushStack(sp, ax);
                } /* push the value of ax onto the stack */
                    break;
                case JMP:
                {
                    pc = base + VmmGet(pc) * INC_PTR;
                } /* jump to the address */
                    break;
                case JZ:
                {
                    pc = ax ? pc + INC_PTR : (base + VmmGet(pc) * INC_PTR);
                } /* jump if ax is zero */
                    break;
                case JNZ:
                {
                    pc = ax ? (base + VmmGet(pc) * INC_PTR) : pc + INC_PTR;
                } /* jump if ax is zero */
                    break;
                case CALL:
                {
                    VmmPushStack(sp, pc + INC_PTR);
                    pc = base + VmmGet(pc) * INC_PTR;
#if VM_DEBUG
                    printf("CALL> PC=%08X\n", pc);
#endif
                } /* call subroutine */
                    /* break;case RET: {pc = (int *)*sp++;} // return from subroutine; */
                    break;
                case ENT:
                {
                    VmmPushStack(sp, bp);
                    bp = sp;
                    sp = sp - VmmGet(pc);
                    pc += INC_PTR;
                } /* make new stack frame */
                    break;
                case ADJ:
                {
                    sp = sp + VmmGet(pc) * INC_PTR;
                    pc += INC_PTR;
                } /* add esp, <size> */
                    break;
                case LEV:
                {
                    sp = bp;
                    bp = VmmPopStack(sp);
                    pc = VmmPopStack(sp);
#if VM_DEBUG
                    printf("RETURN> PC=%08X\n", pc);
#endif
                } /* restore call frame and PC */
                    break;
                case LEA:
                {
                    ax = bp + VmmGet(pc);
                    pc += INC_PTR;
                } /* load address for arguments. */
                    break;
                case OR:
                    ax = VmmPopStack(sp) | ax;
                    break;
                case XOR:
                    ax = VmmPopStack(sp) ^ ax;
                    break;
                case AND:
                    ax = VmmPopStack(sp) & ax;
                    break;
                case EQ:
                    ax = VmmPopStack(sp) == ax;
                    break;
                case NE:
                    ax = VmmPopStack(sp) != ax;
                    break;
                case LT:
                    ax = VmmPopStack(sp) < ax;
                    break;
                case LE:
                    ax = VmmPopStack(sp) <= ax;
                    break;
                case GT:
                    ax = VmmPopStack(sp) > ax;
                    break;
                case GE:
                    ax = VmmPopStack(sp) >= ax;
                    break;
                case SHL:
                    ax = VmmPopStack(sp) << ax;
                    break;
                case SHR:
                    ax = VmmPopStack(sp) >> ax;
                    break;
                case ADD:
                    ax = VmmPopStack(sp) + ax;
                    break;
                case SUB:
                    ax = VmmPopStack(sp) - ax;
                    break;
                case MUL:
                    ax = VmmPopStack(sp) * ax;
                    break;
                case DIV:
                    ax = VmmPopStack(sp) / ax;
                    break;
                case MOD:
                    ax = VmmPopStack(sp) % ax;
                    break;
                    // --------------------------------------
                case PRTF:
                {
                    InitArgs(args, sp, pc);
                    ax = printf(VmmGetStr(args[0]), args[1], args[2], args[3], args[4], args[5]);
                }
                    break;
                case EXIT:
                {
                    printf("exit(%d)\n", ax);
                    return ax;
                }
                    break;
                case OPEN:
                {
                    InitArgs(args, sp, pc);
                    ax = (int) fopen(VmmGetStr(args[0]), "rb");
#if VM_DEBUG
                    printf("OPEN> name=%s fd=%08X\n", VmmSetStr(args[0]), ax);
#endif
                }
                    break;
                case READ:
                {
                    InitArgs(args, sp, pc);
#if VM_DEBUG
                    printf("READ> src=%p size=%08X fd=%08X\n", VmmSetStr(args[1]), args[2], args[0]);
#endif
                    ax = (int) fread(VmmGetStr(args[1]), 1, (size_t) args[2], (FILE *) args[0]);
                    if (ax > 0)
                    {
                        rewind((FILE *) args[0]); // 坑：避免重复读取
                        ax = (int) fread(VmmGetStr(args[1]), 1, (size_t) ax, (FILE *) args[0]);
                        VmmGetStr(args[1])[ax] = 0;
#if VM_DEBUG
                        printf("READ> %s\n", VmmSetStr(args[1]));
#endif
                    }
                }
                    break;
                case CLOS:
                {
                    InitArgs(args, sp, pc);
                    ax = (int) fclose((FILE *) args[0]);
                }
                    break;
                case MALC:
                {
                    InitArgs(args, sp, pc);
                    ax = (int) VmmMalloc((uint32_t) args[0]);
                }
                    break;
                case MSET:
                {
                    InitArgs(args, sp, pc);
#if 0
                    printf("MEMSET> PTR=%08X SIZE=%08X VAL=%d\n", (uint32_t)VmmSetStr(args[0]), (uint32_t)args[2], (uint32_t)args[1]);
#endif
                    ax = (int) VmmMemset(args[0], (uint32_t) args[1], (uint32_t) args[2]);
                }
                    break;
                case MCMP:
                {
                    InitArgs(args, sp, pc);
                    ax = (int) VmmMemcmp(args[0], args[1], (uint32_t) args[2]);
                }
                    break;
                case TRAC:
                {
                    InitArgs(args, sp, pc);
                    ax = log;
                    log = args[0] != 0;
                }
                    break;
                case TRAN:
                {
                    InitArgs(args, sp, pc);
                    ax = (uint32_t) VmmGetStr(args[0]);
                }
                    break;
                default:
                {
                    printf("AX: %08X BP: %08X SP: %08X PC: %08X\n", ax, bp, sp, pc);
                    for (uint32_t i = sp; i < STACK_BASE + PAGE_SIZE; i += 4)
                    {
                        printf("[%08X]> %08X\n", i, VmmGet<uint32_t>(i));
                    }
                    printf("unknown instruction:%d\n", op);
                    throw std::exception();
                    exit(-1);
                }
            }

#if 1
            if (log)
            {
                printf("\n---------------- STACK BEGIN <<<< \n");
                printf("AX: %08X BP: %08X SP: %08X PC: %08X\n", ax, bp, sp, pc);
                for (uint32_t i = sp; i < STACK_BASE + PAGE_SIZE; i += 4)
                {
                    printf("[%08X]> %08X\n", i, VmmGet<uint32_t>(i));
                }
                printf("---------------- STACK END >>>>\n\n");
            }
#endif
        }
        return 0;
    }


}

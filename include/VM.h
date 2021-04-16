//
// Created by yw.
//

#ifndef DRTCC_VM_H
#define DRTCC_VM_H

#include "Type.h"
#include "MemoryPool.h"
// 对于一个32位虚拟地址（virtual address）
// 32-22: 页目录号 | 21-12: 页表号 | 11-0: 页内偏移

/* 4k per page */
#define PAGE_SIZE 4096

/* 页掩码，取高20位 */
#define PAGE_MASK 0xfffff000

/* 地址对齐 */
#define PAGE_ALIGN_DOWN(x) ((x) & PAGE_MASK)
#define PAGE_ALIGN_UP(x) (((x) + PAGE_SIZE - 1) & PAGE_MASK)

/* 分析地址 */
#define PDE_INDEX(x) (((x) >> 22) & 0x3ff)  // 获得地址对应的页目录号
#define PTE_INDEX(x) (((x) >> 12) & 0x3ff)  // 获得页表号
#define OFFSET_INDEX(x) ((x) & 0xfff)       // 获得页内偏移

// 页目录项、页表项用uint32表示即可
using pde_t = uint32_t;
using pte_t = uint32_t;

/* 页目录大小 1024 */
#define PDE_SIZE ( PAGE_SIZE / sizeof(pte_t) )
/* 页表大小 1024 */
#define PTE_SIZE ( PAGE_SIZE / sizeof(pde_t) )
#define PTE_COUNT 1024

/* pde&pdt attribute */
#define PTE_P   0x1     // 有效位 Present
#define PTE_R   0x2     // 读写位 Read/Write, can be read&write when set
#define PTE_U   0x4     // 用户位 User / Kern
#define PTE_K   0x0     // 内核位 User / Kern

/*
 *  Memory address from 0x00000000 to 0xffffffff
 */
//                                 //  +---------------------+  --> 0xffffffff
/* 用户代码段基址 */                  //  |        ...          |
#define USER_BASE 0xc0000000       //   |       heap         |
/* 用户数据段基址 */                  //  +--------------------+  --> 0xf0000000
#define DATA_BASE 0xd0000000       //   |        ...         |
/* 用户栈基址 */                     //   |       stack        |
#define STACK_BASE 0xe0000000      //   +--------------------+  --> 0xe0000000
/* 用户堆基址 */                     //   |       data         |
#define HEAP_BASE 0xf0000000       //   +--------------------+  --> 0xd0000000
/* 用户堆大小 */                     //   |       text         |
#define HEAP_SIZE 1000             //   +--------------------+  --> 0xc0000000
/* 段掩码 */                        //   |                    |
#define SEGMENT_MASK 0x0fffffff    //   +--------------------+  --> 0x00000000


/* 物理内存(单位：16B) */
#define PHY_MEM (16 * 1024)
/* 堆内存(单位：16B) */
#define HEAP_MEM (256 * 1024)

namespace DrTcc
{
    class VM
    {
            using TextType = BaseType<TokenType::Int>::type;
            using DataType = BaseType<TokenType::Char>::type;
        public:
            explicit VM(const std::vector<TextType> &text, const std::vector<DataType> &data);

            ~VM();

            int Exec(int entry = -1);

        private:
            // 初始化页表
            void VmmInit();

            // 虚页映射
            void VmmMap(uint32_t va, uint32_t pa, uint32_t flags);

            // 解除映射
            void VmmUnmap(pde_t *pde, uint32_t va);

            // 查询分页情况
            int VmmIsmap(uint32_t va, uint32_t *pa) const;

            // 申请页框
            uint32_t PmmAlloc();

            template<class T = int>
            T VmmGet(uint32_t va);

            template<class T = int>
            T VmmSet(uint32_t va, T value);

            char *VmmGetStr(uint32_t va);

            void VmmSetStr(uint32_t va, const char *value);

            uint32_t VmmMalloc(uint32_t size);

            static uint32_t VmmPa2va(uint32_t base, uint32_t size, uint32_t pa)
            {
                return base + (pa & (SEGMENT_MASK));
            }

            uint32_t VmmMemset(uint32_t va, uint32_t value, uint32_t count);

            uint32_t VmmMemcmp(uint32_t src, uint32_t dst, uint32_t count);

            template<class T = int>
            void VmmPushStack(uint32_t &sp, T value);

            template<class T = int>
            T VmmPopStack(uint32_t &sp);

            void InitArgs(uint32_t *args, uint32_t sp, uint32_t pc, bool converted = false);


        private:
            /* 内核页表 = PTE_SIZE * PAGE_SIZE */
            pde_t *pgd_kern;
            /* 内核页表内容 = PTE_COUNT * PTE_SIZE * PAGE_SIZE */
            pde_t *pte_kern;

            // 物理内存 -> PHY_MEM * 16B
            MemoryPool<PHY_MEM> memory_;
            // 页表目录指针
            pde_t *pageDir{nullptr};
            // 堆内存
            MemoryPool<HEAP_MEM> heap_;
            byte *heapHead;

    };
}


#endif //DRTCC_VM_H

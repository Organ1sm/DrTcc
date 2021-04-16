//
// Created by yw.
//

#ifndef DRTCC_MEMORYPOOL_H
#define DRTCC_MEMORYPOOL_H

#include "Type.h"
#include "Token.h"

namespace DrTcc
{
    // 默认的内存分配策略
    template<size_t DefaultSize = 0x10000>
    class DefaultAllocator
    {
        public:
            static const size_t DEFAULT_ALLOC_BLOCK_SIZE = DefaultSize;

            template<class T>
            T *__Alloc()
            {
                return new T;
            }

            template<class T>
            T *__AllocArray(uint size)
            {
                return new T[size];
            }

            template<class T, class ... TArgs>
            T *__AllocArgs(const TArgs &&... args)
            {
                return new T(std::forward(args)...);
            }

            template<class T, class ... TArgs>
            T *__AllocArrayArgs(uint size, const TArgs &&... args)
            {
                return new T[size];
            }

            template<class T>
            bool __Free(T *t)
            {
                delete t;
                return true;
            }

            template<class T>
            bool __FreeArray(T *t)
            {
                delete[] t;
                return true;
            }
    };


    // 原始内存池
    template<class Allocator, size_t DefaultSize = Allocator::DEFAULT_ALLOC_BLOCK_SIZE>
    class LegacyMemoryPool
    {
            // 块
            struct block
            {
                size_t size; // 数据部分的大小
                uint flag;   // 参数
                block *prev; // 前指针
                block *next; // 后指针
            };

            // 块参数
            enum BlockFlag
            {
                BLOCK_USING = 0, BLOCK_MARK = 1,
            };

            // 内存管理接口
            Allocator allocator;

            // 块的元信息部分的大小
            static const size_t BLOCK_SIZE = sizeof(block);
            // 块大小掩码
            static const uint BLOCK_SIZE_MASK = BLOCK_SIZE - 1;

            // 块链表头指针
            block *blockHead{nullptr};
            // 用于循环遍历的指针
            block *blockCurrent{nullptr};
            // 空闲块数
            size_t blockAvailableSize{0};


            // 块大小对齐
            static size_t BlockAlign(size_t size)
            {
                if ((size & BLOCK_SIZE_MASK) == 0)
                {
                    return size / BLOCK_SIZE;
                }
                return (size / BLOCK_SIZE) + 1;
            }

            // 块初始化
            static void BlockInit(block *blk, size_t size)
            {
                blk->size = size;
                blk->flag = 0;
                blk->prev = nullptr;
                blk->next = nullptr;
            }

            // 块连接
            static void BlockConnect(block *blk, block *newBlock)
            {
                newBlock->prev = blk;
                newBlock->next = blk->next;
                newBlock->next->prev = newBlock;
                blk->next = newBlock;
            }

            // 二块合并
            static size_t BlockMerge(block *blk, block *next)
            {
                auto tmp = next->size + 1;
                next->prev = blk;
                blk->size += tmp;
                blk->next = next->next;
                return tmp;
            }

            // 三块合并
            static size_t BlockMerge(block *prev, block *blk, block *next)
            {
                auto tmp = blk->size + next->size + 2;
                next->prev = prev;
                prev->size += tmp;
                prev->next = next->next;
                return tmp;
            }

            // 块设置参数
            static void BlockSetFlag(block *blk, BlockFlag flag, uint value)
            {
                if (value)
                { blk->flag |= 1 << flag; }
                else
                { blk->flag &= ~(1 << flag); }
            }

            // 块获取参数
            static uint BlockGetFlag(block *blk, BlockFlag flag)
            {
                return (blk->flag & (1 << flag)) != 0 ? 1 : 0;
            }


            // ------------------------ //

            // 创建内存池
            void _Create()
            {
                blockHead = allocator.template __AllocArray<block>(DEFAULT_ALLOC_BLOCK_SIZE);
                assert(blockHead);
                _Init();
            }

            // 初始化内存池
            void _Init()
            {
                blockAvailableSize = DEFAULT_ALLOC_BLOCK_SIZE - 1;
                BlockInit(blockHead, blockAvailableSize);
                blockHead->prev = blockHead->next = blockHead;
                blockCurrent = blockHead;
            }

            // 销毁内存池
            void _Destroy()
            {
                allocator.__FreeArray(blockHead);
            }

            // 申请内存
            void *_Alloc(size_t size)
            {
                if (size == 0)
                { return nullptr; }

                auto oldSize = size;
                size = BlockAlign(size);
                if (size >= blockAvailableSize)
                { return nullptr; }


                if (blockCurrent == blockHead)
                {
                    return AllocFreeBlock(size);
                }

                auto blk = blockCurrent;
                do
                {
                    if (BlockGetFlag(blk, BLOCK_USING) == 0 && blk->size >= size)
                    {
                        blockCurrent = blk;
                        return AllocFreeBlock(size);
                    }
                    blk = blk->next;
                } while(blk != blockCurrent);

                return nullptr;
            }

            // 查找空闲块
            void *AllocFreeBlock(size_t size)
            {
                if (blockCurrent->size == size) // 申请的大小正好是空闲块大小
                {
                    return AllocCurBlock(size + 1);
                }
                // 申请的空间小于空闲块大小，将空闲块分裂
                auto newSize = blockCurrent->size - size - 1;
                if (newSize == 0)
                {
                    return AllocCurBlock(size);
                } // 分裂后的新块空间过低，放弃分裂

                block *newBlk = blockCurrent + size + 1;
                BlockInit(newBlk, newSize);
                BlockConnect(blockCurrent, newBlk);
                return AllocCurBlock(size);
            }

            // 直接使用当前的空闲块
            void *AllocCurBlock(size_t size)
            {
                // 直接使用空闲块
                BlockSetFlag(blockCurrent, BLOCK_USING, 1); // 设置标志为可用
                blockCurrent->size = size;
                blockAvailableSize -= size + 1;
                auto cur = static_cast<void *>(blockCurrent + 1);
                blockCurrent = blockCurrent->next; // 指向后一个块

                return cur;
            }

            // 释放内存
            bool _Free(void *p)
            {
                block *blk = static_cast<block *>(p);
                --blk; // 自减得到块的元信息头
                if (!VerifyAddress(blk))
                {
                    return false;
                }
                if (blk->next == blk) // 只有一个块
                {
                    BlockSetFlag(blk, BLOCK_USING, 0);
                    return true;
                }
                if (blk->prev == blk->next && BlockGetFlag(blk->prev, BLOCK_USING) == 0) // 只有两个块
                {
                    _Init(); // 两个块都空闲，直接初始化
                    return true;
                }
                auto is_prev_free = BlockGetFlag(blk->prev, BLOCK_USING) == 0 && blk->prev < blk;
                auto is_next_free = BlockGetFlag(blk->next, BLOCK_USING) == 0 && blk < blk->next;
                auto bit = (is_prev_free << 1) + is_next_free;
                switch (bit)
                {
                    case 0:
                        blockAvailableSize += blk->size + 1;
                        BlockSetFlag(blk, BLOCK_USING, 0);
                        break;
                    case 1:
                        blockAvailableSize += BlockMerge(blk, blk->next);
                        break;
                    case 2:
                        blockAvailableSize += BlockMerge(blk->prev, blk);
                        break;
                    case 3:
                        blockAvailableSize += BlockMerge(blk->prev, blk, blk->next);
                        break;
                    default:
                        break;
                }
                return true;
            }

            // 验证地址是否合法
            bool VerifyAddress(block *blk)
            {
                if (blk < blockHead || blk > blockHead + DEFAULT_ALLOC_MEMORY_SIZE - 1)
                {
                    return false;
                }
                return (blk->next->prev == blk) && (blk->prev->next == blk) && (BlockGetFlag(blk, BLOCK_USING) == 1);
            }

            // 重新分配内存
            void *_Realloc(void *p, uint newSize, uint clsSize)
            {
                block *blk = static_cast<block *>(p);
                --blk; // 自减得到块的元信息头
                if (!VerifyAddress(blk))
                {
                    return nullptr;
                }
                auto size = BlockAlign(newSize * clsSize); // 计算新的内存大小
                auto new_ = _Alloc(size);
                if (!new_)
                {
                    // 空间不足
                    _Free(blk);
                    return nullptr;
                }
                auto oldSize = blk->size;
                memmove(new_, p, sizeof(block) * __min(oldSize, size)); // 移动内存
                _Free(p);

                return new_;
            }

        public:

            // 默认的块总数
            static const size_t DEFAULT_ALLOC_BLOCK_SIZE = DefaultSize;
            // 默认的内存总量
            static const size_t DEFAULT_ALLOC_MEMORY_SIZE = BLOCK_SIZE * DEFAULT_ALLOC_BLOCK_SIZE;

            LegacyMemoryPool()
            {
                _Create();
            }

            ~LegacyMemoryPool()
            {
                _Destroy();
            }

            template<class T>
            T *Alloc()
            {
                return static_cast<T *>(_Alloc(sizeof(T)));
            }

            template<class T>
            T *AllocArray(uint count)
            {
                return static_cast<T *>(_Alloc(count * sizeof(T)));
            }

            template<class T, class ...TArgs>
            T *AllocArgs(const TArgs &&... args)
            {
                T *obj = static_cast<T *>(_Alloc(sizeof(T)));
                (*obj)(std::forward(args)...);
                return obj;
            }

            template<class T, class ...TArgs>
            T *AllocArrayArgs(uint count, const TArgs &&... args)
            {
                T *obj = static_cast<T *>(_Alloc(count * sizeof(T)));
                for (uint i = 0; i < count; ++i)
                {
                    (obj[i])(std::forward(args)...);
                }
                return obj;
            }

            template<class T>
            T *Realloc(T *obj, uint newSize)
            {
                return static_cast<T *>(_Realloc(obj, newSize, sizeof(T)));
            }

            template<class T>
            bool Free(T *obj)
            {
                return _Free(obj);
            }

            template<class T>
            bool FreeArray(T *obj)
            {
                return _Free(obj);
            }

            size_t Available() const
            {
                return blockAvailableSize;
            }

            void Clear()
            {
                _Init();
            }
    };


    // 基于原始内存池的内存分配策略
    template<class Allocator = DefaultAllocator<>, size_t DefaultSize = Allocator::DEFAULT_ALLOC_BLOCK_SIZE>
    class LegacyMemoryPoolAllocator
    {
            LegacyMemoryPool<Allocator, DefaultSize> memoryPool;

        public:
            static const size_t DEFAULT_ALLOC_BLOCK_SIZE = DefaultSize - 2;

            template<class T>
            T *__Alloc()
            {
                return memoryPool.template Alloc<T>();
            }

            template<class T>
            T *__AllocArray(uint count)
            {
                return memoryPool.template AllocArray<T>(count);
            }

            template<class T, class ... TArgs>
            T *__AllocArgs(const TArgs &&... args)
            {
                return memoryPool.template AllocArgs<T>(std::forward(args)...);
            }

            template<class T, class ... TArgs>
            T *__AllocArrayArgs(uint count, const TArgs &&... args)
            {
                return memoryPool.template AllocArrayArgs<T>(count, std::forward(args)...);
            }

            template<class T>
            T *__Realloc(T *t, uint newSize)
            {
                return memoryPool.template Realloc<T>(t, newSize);
            }

            template<class T>
            bool __Free(T *t)
            {
                return memoryPool.template Free(t);
            }

            template<class T>
            bool __FreeArray(T *t)
            {
                return memoryPool.FreeArray(t);
            }
    };

    template<size_t DefaultSize = DefaultAllocator<>::DEFAULT_ALLOC_BLOCK_SIZE> using MemoryPool = LegacyMemoryPool<LegacyMemoryPoolAllocator<DefaultAllocator<>, DefaultSize>>;
}


#endif //DRTCC_MEMORYPOOL_H

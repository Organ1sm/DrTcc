# DrTcc (Dr. Tiny C Compiler)

**DrTcc**(玩具项目)使用  `C++ 14` 实现了一个C语言子集，基于栈虚拟机 , 既像解释器又像编译器。这个项目旨在满足我的好奇心和好玩心。

## 介绍

在开始启动这个项目之前，是想实现一个算法可视化平台，因此学习了[write-a-C-interpreter](https://github.com/lotabout/write-a-C-interpreter) ，给了我实现的方向。代码量不多但我依然觉得结构臃肿，写的不够优雅，没怎么发挥 `C++`的实力。等学过 `Lua` 的VM 和 Compiler Optimization再来review code。

## 文件目录结构

```
DrTcc
├─bin
│      happy.txt
│      xc.txt
├─include							
│      AST.cpp
│      AST.h
│      GenCode.cpp
│      GenCode.h
│      ImportSTL.h
│      Lexer.cpp
│      Lexer.h
│      MemoryPool.h
│      Parser.cpp
│      Parser.h
│      Token.cpp
│      Token.h
│      Type.h
│      VM.cpp
│      VM.h
│
├─src
│      main.cpp
```



## 特性

1. 手写递归下降Parser
2. 输入源码生成AST(abstract syntax trees)，实现简易内存池管理AST结点，采用POD类型
3. 在生成AST结点时和IR时语义分析和简易的类型检查
4. 指令集参考[write-a-C-interpreter](https://github.com/lotabout/write-a-C-interpreter)，根据AST生成IR
5. 栈虚拟机实现虚拟内存(采用二级页表结构),执行IR,实现物理内存隔离

## 调试信息

```c++
// located in include/Parser.cpp
#define Tokenize 0		// 输出词法分析后的Token流
#define GEN_AST 0		// 输出AST

// located in include/GenCode.cpp
#define GEN_DEBUG 0		// 输出AST-->IR

// located in include/VM.cpp	
#define VM_DEBUG 0				// 输出VM内存信息
#define INSTRUCTION_DEBUG  0	// 输出VM字节码
```



## 改进方向..

1. stack based -> register based
2. 编译器后端优化.. 

## 功能/支持

- [x] 全局/局部变量及初始化
- [x] 函数调用以及递归调用，函数形参
- [x] 枚举
- [x]  `if, while`语句
- [x] 一元，二元，三元运算 
- [x]  `sizeof`运算
- [x] 取址和解引用
- [x] 类型转换



## Test & 截图

以此代码为例：

```c
int fibonacci(int i)
{
    if (i <= 1)
        return 1;
    return fibonacci(i - 1) + fibonacci(i - 2);
}

int main()
{
    int i;
    i = 0;
    printf("----- fibonacci -----\n");
    while (i <= 10)
    {
        printf("fibonacci(%2d) = %d\n", i, fibonacci(i));
        i++;
    }
    return 0;
}
```

![](Screenhot/result.png)



## Build  & Run

只支持32位，使用 `Cmake MinGw` build

```
make 
```

`bin/xc.txt` 是[write-a-C-interpreter](https://github.com/lotabout/write-a-C-interpreter) 源码，`happy.exe`是我们的最终程序，我们可以：

```
.\happy.exe SourceCodeFile
```

也可以递归下去实现[write-a-C-interpreter](https://github.com/lotabout/write-a-C-interpreter)的自举

```
.\happy xc.txt xc.txt ... 
```

或者

```
.\happy xc.txt YourSourceCodeFile
```

# Licence

The original code is licenced with MIT
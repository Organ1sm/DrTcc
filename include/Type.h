// Created by yw.
//

#ifndef DRTCC_TYPE_H
#define DRTCC_TYPE_H

#include "ImportSTL.h"

namespace DrTcc
{
#ifdef _WIN32
    using uint8 = unsigned __int8;
#elif __linux__
    using uint8 = unsigned char;
#endif

    using sint = signed int;
    using uint = unsigned int;
    using slong = long long;
    using ulong = unsigned long long;
    using byte = uint8;


    enum ErrorType
    {
        ErrorStart, ErrorInvalidChar, ErrorInvalidOperator, ErrorInvalidDigit, ErrorInvalidString, ErrorEnd
    };

    /*指令枚举类*/
    enum Instrucitons
    {
        NOP, LEA, IMM, IMX, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, SI, LC, SC, PUSH, LOAD, OR, XOR, AND, EQ, NE, LT, GT,
        LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD, OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, TRAC, TRAN, EXIT
    };
}

#endif //DRTCC_TYPE_H

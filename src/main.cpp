#include <iostream>
#include "GenCode.h"
#include "Parser.h"

#define Test 0
extern int globalArgc;
extern char **globalArgv;

void CompileAndRun(const std::string &sourceCode);

int main(int argc, char **argv)
{
    globalArgc = argc;
    globalArgv = argv;
#if Test
    std::string txt = R"(
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
})";
    CompileAndRun(txt);
#else
    globalArgc--;
    globalArgv++;

    if (globalArgc < 1)
    {
        std::cout << "Usage: DrTcc file..\n";
        return -1;
    }

    std::ifstream sourceFile(*globalArgv);
    std::istreambuf_iterator<char> begin_(sourceFile);
    std::istreambuf_iterator<char> end_;

    std::string sourceCode_(begin_, end_);

    if (sourceCode_.empty())
    { exit(-1); }

    try
    {
        CompileAndRun(sourceCode_);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << '\n';
    }


#endif
    return 0;
}

void CompileAndRun(const std::string &sourceCode)
{
    DrTcc::Parser parser(sourceCode);
    DrTcc::AstNode *root = parser.Parse();
    DrTcc::GenCode genCode(root);
    genCode.Eval();
}
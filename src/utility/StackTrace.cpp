#include "StackTrace.h"
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <cxxabi.h>
#include <cstdlib>
#endif

namespace Internal
{

std::string StackTrace::capture(int skipFrames, int maxFrames)
{
    std::stringstream ss;

#ifdef _WIN32
    // Windows implementation
    void*  stack[64];
    HANDLE process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);

    WORD frames = CaptureStackBackTrace(skipFrames, maxFrames, stack, NULL);

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    if (!symbol)
    {
        return "  [Failed to allocate symbol info]\n";
    }

    symbol->MaxNameLen   = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (WORD i = 0; i < frames; i++)
    {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        ss << "  [" << i << "] " << symbol->Name << " (0x" << std::hex << symbol->Address << std::dec << ")\n";
    }

    free(symbol);
#else
    // Linux/Unix implementation
    void* stack[64];
    int   frames  = backtrace(stack, maxFrames + skipFrames);
    char** symbols = backtrace_symbols(stack, frames);

    if (symbols)
    {
        for (int i = skipFrames; i < frames; i++)
        {
            // Try to demangle C++ names
            std::string symbol(symbols[i]);

            // Find mangled name between '(' and '+'
            size_t start = symbol.find('(');
            size_t end   = symbol.find('+');

            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string mangled = symbol.substr(start + 1, end - start - 1);

                int   status;
                char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);

                if (status == 0 && demangled)
                {
                    ss << "  [" << (i - skipFrames) << "] " << demangled << "\n";
                    free(demangled);
                }
                else
                {
                    ss << "  [" << (i - skipFrames) << "] " << symbols[i] << "\n";
                }
            }
            else
            {
                ss << "  [" << (i - skipFrames) << "] " << symbols[i] << "\n";
            }
        }
        free(symbols);
    }
    else
    {
        ss << "  [Failed to get backtrace symbols]\n";
    }
#endif

    return ss.str();
}

}  // namespace Internal

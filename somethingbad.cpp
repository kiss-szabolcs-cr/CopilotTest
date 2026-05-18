#include <cstring>

void UnsafeCopy(const char* input)
{
    char buffer[8];
    strcpy(buffer, input); // intentionally unsafe for CodeQL test
}
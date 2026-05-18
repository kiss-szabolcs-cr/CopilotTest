#include <cstring>
#include <iostream>

int main(int argc, char** argv)
{
    char buffer[8];

    if (argc > 1)
    {
        strcpy(buffer, argv[1]); // intentionally unsafe
        std::cout << buffer << std::endl;
    }

    return 0;
}
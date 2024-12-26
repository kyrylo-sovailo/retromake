#include "shared.h"
#include "static.h"
#include <iostream>

int main()
{
    ping_shared();
    ping_static();
    std::cout << "I am executable" << std::endl;
    return 0;
}

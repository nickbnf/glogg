#include <iostream>
#include <limits>

int main()
{
    std::cout << std::numeric_limits<unsigned int>::max() << std::endl;
    for (auto line = 0ull; line < std::numeric_limits<unsigned int>::max(); ++line)
    {
        std::cout << (line % 10) << "\n";
    }

    return 0;
}
#include <iostream>

int main()
{
    char* env = std::getenv("USER");
    std::cout << "Your user is:" << env << "\n";
    return 0;
}

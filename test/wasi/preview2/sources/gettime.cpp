#include <iostream>
#include <chrono>
#include <ctime>

int main()
{
    std::cout << "time: " << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
    std::cout << "time: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
}

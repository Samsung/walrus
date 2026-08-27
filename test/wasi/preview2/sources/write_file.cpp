#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
    std::ofstream output(argv[1]);
    if (output.fail() || output.bad() || !output.is_open()) {
        std::cout << "Error: Cannot open file!\n";
        return 1;
    }

    output << "Hello world!\n";
    return 0;
}

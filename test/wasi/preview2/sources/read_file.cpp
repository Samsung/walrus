#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Error: no arguments supplied!\n";
        return 1;
    }

    std::fstream input(argv[1], std::ios::in);
    if (input.fail() || input.bad() || !input.is_open()) {
        std::cout << "Error: Cannot open file!\n";
        return 1;
    }

    std::string line;
    while (std::getline(input, line)) {
        std::cout << line << "\n";
    }

    input.close();


    return 0;
}

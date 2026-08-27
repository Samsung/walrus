#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <iostream>

int main(int argc, char* argv[])
{
    struct stat sb;
    stat(argv[1], &sb);

    std::cout << "file info:\n"
              << std::endl;
    std::cout << sb.st_dev << std::endl;
    std::cout << sb.st_ino << std::endl;
    std::cout << sb.st_mode << std::endl;
    std::cout << sb.st_nlink << std::endl;
    std::cout << sb.st_uid << std::endl;
    std::cout << sb.st_gid << std::endl;
    std::cout << sb.st_rdev << std::endl;
    std::cout << sb.st_size << std::endl;
    std::cout << sb.st_blksize << std::endl;
    std::cout << sb.st_blocks << std::endl;
    std::cout << sb.st_atime << std::endl;
    std::cout << sb.st_mtime << std::endl;
    std::cout << sb.st_ctime << std::endl;

    return 0;
}

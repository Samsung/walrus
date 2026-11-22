#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

int main(int argc, char* argv[])
{
    struct stat sb;
    if (stat(argv[1], &sb) != 0) {
        abort();
    }

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

    if (!S_ISREG(sb.st_mode) || sb.st_mtime < 1000000000 || sb.st_mtime > 4000000000) {
        abort();
    }

    struct stat dirStat;
    if (stat("/var", &dirStat) != 0 || !S_ISDIR(dirStat.st_mode)) {
        abort();
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        abort();
    }

    struct stat fileStat;
    if (fstat(fd, &fileStat) != 0 || !S_ISREG(fileStat.st_mode) || fileStat.st_mtime != sb.st_mtime) {
        abort();
    }

    if ((fcntl(fd, F_GETFL) & O_ACCMODE) != O_RDONLY) {
        abort();
    }

    close(fd);
    return 0;
}

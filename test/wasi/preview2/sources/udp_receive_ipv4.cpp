#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("socket() failed\n");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("bind() failed\n");
        return 1;
    }

    printf("listening on 127.0.0.1:12345\n");
    fflush(stdout);

    char buf[1024];
    ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0, NULL, NULL);
    if (n < 0) {
        printf("recvfrom() failed\n");
        return 1;
    }

    buf[n] = '\0';
    printf("received: %s\n", buf);

    close(fd);
    return 0;
}

#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return 1;
    }

    const char* msg = "1";
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    dest.sin_port = htons(12345);
    ssize_t sent = sendto(sock, msg, 1, 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

    close(sock);
    return 0;
}

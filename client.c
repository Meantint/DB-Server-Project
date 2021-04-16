#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int fd;
char read_buffer[100];

int Init(char* ip_address)
{
    int ret = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;  // IPv4
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = inet_addr(ip_address);

    ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        return -1;
    }

    return ret;
}

int main()
{
    char text1[100], text2[100];
    while (1) {
        memset(read_buffer, 0, sizeof(read_buffer));
        memset(text1, 0, sizeof(text1));
        memset(text2, 0, sizeof(text2));

        scanf("%s", text1);
        if (strcmp(text1, "exit") == 0) {
            printf("client exit\n");
            shutdown(fd, SHUT_WR);  // 연결을 종료한다.

            close(fd);
            exit(1);
        }
        else if (strcmp(text1, "clear") == 0) {
            write(fd, text1, sizeof(text1));

            int read_length = read(fd, read_buffer, sizeof(read_buffer));
            printf("Server response : %s\n", read_buffer);
        }
        else if (strcmp(text1, "connect") == 0) {  // connect
            scanf("%s", text2);

            int ret = Init(text2);
            if (ret == -1) {
                printf("INIT ERROR\n");
            }
        }
        else if (strcmp(text1, "save") == 0) {
            scanf("%s", text2);
            write(fd, text1, sizeof(text1));
            write(fd, text2, sizeof(text2));

            memset(read_buffer, 0, 100);
            int read_length = read(fd, read_buffer, sizeof(read_buffer));
            printf("Server response : %s\n", read_buffer);
        }
        else if (strcmp(text1, "read") == 0) {
            scanf("%s", text2);
            write(fd, text1, sizeof(text1));
            write(fd, text2, sizeof(text2));

            memset(read_buffer, 0, 100);
            int read_length = read(fd, read_buffer, sizeof(read_buffer));
            printf("Server response(Key value) : %s\n", read_buffer);
        }
        else {
            printf("CMD ERROR!\n");
            return 0;
        }
    }

    return 0;
}

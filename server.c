#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int fd, new_fd;
char read_buffer[100];

FILE *file_db_pointer;
FILE *file_change_pointer;

// 클라이언트와의 통신을 위한 함수
int Init()
{
    int ret = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("socket error\n");
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;  // IPv4

    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = 0;

    ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (ret == -1) {
        printf("bind error\n");
        printf("Error code: %d\n", errno);
        return ret;
    }

    ret = listen(fd, 2);
    if (ret == -1) {
        printf("listen error\n");
        return ret;
    }

    printf("LISTEN..\n");
    return ret;
}

// SIGINT 시그널을 받은 경우에 동장하는 함수.
// client에 EOF를 보낸다.
void SigExit()
{
    printf("\nSignal Exit\n");

    shutdown(new_fd, SHUT_WR);

    close(new_fd);
    close(fd);

    exit(0);
}

int main()
{
    int file_line_length = -1;

    signal(SIGINT, SigExit);  // CTRL + C

    int ret = Init();
    if (ret == -1) {
        printf("Init ERROR!\n");
        return 0;
    }

    struct sockaddr new_addr = {0};
    int len;

    new_fd = accept(fd, &new_addr, &len);
    printf("ACCEPT\n");

    // DB 파일을 가져온다.
    file_db_pointer = fopen("DB.txt", "r+");

    while (1) {
        fseek(file_db_pointer, 0, SEEK_SET);  // file cursor의 위치를 초기화한다
        memset(read_buffer, 0, sizeof(read_buffer));
        int read_length = read(new_fd, read_buffer, sizeof(read_buffer));

        if (read_length == 0) {  // client로 부터 EOF 받음
            printf("client request : exit\n");
            return 0;
        }

        // clear
        // DB.txt 파일 내용을 모두 지운다.
        if (strcmp(read_buffer, "clear") == 0) {
            printf("client request : clear\n");
            system("rm DB.txt && touch DB.txt");
            file_db_pointer = fopen("DB.txt", "r+");
            write(new_fd, "CLEAR OK", sizeof("CLEAR OK"));
        }
        // save
        // key:value 값 추가 혹은 기존의 key:value 값 변경
        else if (strcmp(read_buffer, "save") == 0) {
            system("touch change.txt");
            file_change_pointer = fopen("change.txt", "w");

            printf("client request : save\n");
            memset(read_buffer, 0, sizeof(read_buffer));
            int read_length = read(new_fd, read_buffer, sizeof(read_buffer));

            read_buffer[strcspn(read_buffer, "\n")] = 0;

            // (key, value)로 나누기
            char read_key[100], read_value[100];
            int read_key_idx = 0, read_value_idx = 0;
            memset(read_key, 0, sizeof(read_key));
            memset(read_value, 0, sizeof(read_value));

            int is_key = 1;  // ':' 만나기 전까지 1, 이후 0, key와 value를 분리하기 위해 사용
            for (int i = 0; i < read_length; ++i) {
                if (read_buffer[i] == ':') {
                    is_key = 0;
                    read_key[read_key_idx] = 0;
                    continue;
                }
                if (is_key == 1) {
                    read_key[read_key_idx] = read_buffer[i];
                    ++read_key_idx;
                }
                else {
                    read_value[read_value_idx] = read_buffer[i];
                    ++read_value_idx;
                }
            }
            read_value[read_value_idx] = 0;

            int find_line = 0;
            char buffer[100];

            // DB.txt의 몇 번째 라인인지 찾기
            char find_key[100];
            while ((fgets(buffer, 99, file_db_pointer)) != NULL) {  // (key, value)로 나누기
                buffer[strcspn(buffer, "\n")] = 0;
                int find_key_idx = 0;
                memset(find_key, 0, sizeof(find_key));

                for (int i = 0; buffer[i] != 0; ++i) {
                    if (buffer[i] == ':') {
                        find_key[find_key_idx] = 0;
                        break;
                    }
                    find_key[find_key_idx] = buffer[i];
                    ++find_key_idx;
                }
                if (strcmp(read_key, find_key) == 0) {
                    break;
                }
                ++find_line;
            }
            fseek(file_db_pointer, 0, SEEK_SET);  // file cursor 초기화

            int count = 0;
            int is_not_find = 1;
            // 특정 라인만 변경하기
            // ! DB 개행에 문제가 있어서 나중에 고쳐야함
            while ((fgets(buffer, 99, file_db_pointer)) != NULL) {
                if (strlen(buffer) == 0) {
                    continue;
                }
                if (count == find_line) {
                    fputs(read_buffer, file_change_pointer);
                    fputs("\n", file_change_pointer);
                    is_not_find = 0;
                }
                else {
                    // if (count != 0) {
                    //     fputs("\n", file_change_pointer);
                    // }
                    fputs(buffer, file_change_pointer);
                    //fputs("\n", file_change_pointer);
                }
                ++count;
            }

            // 기존에 없던 키라면
            if (is_not_find == 1) {
                if (find_line != 0) {
                    fputs("\n", file_change_pointer);
                }
                fputs(read_buffer, file_change_pointer);
            }
            fclose(file_change_pointer);
            system("cp change.txt DB.txt && rm change.txt");
            write(new_fd, "SAVE OK", sizeof("SAVE OK"));
        }
        else if (strcmp(read_buffer, "read") == 0) {
            printf("client request : read\n");
            memset(read_buffer, 0, sizeof(read_buffer));
            int read_length = read(new_fd, read_buffer, sizeof(read_buffer));

            read_buffer[strcspn(read_buffer, "\n")] = 0;

            char buffer[100];
            // DB.txt의 key, value 탐색
            char find_key[100], find_value[100];
            while ((fgets(buffer, 99, file_db_pointer)) != NULL) {  // (key, value)로 나누기
                buffer[strcspn(buffer, "\n")] = 0;

                int find_key_idx = 0, find_value_idx = 0;
                memset(find_key, 0, sizeof(find_key));
                memset(find_value, 0, sizeof(find_value));

                int is_key = 1;  // ':' 만나기 전까지 1, 이후 0
                for (int i = 0; buffer[i] != 0; ++i) {
                    if (buffer[i] == ':') {
                        is_key = 0;
                        find_key[find_key_idx] = 0;
                        continue;
                    }
                    if (is_key == 1) {
                        find_key[find_key_idx] = buffer[i];
                        ++find_key_idx;
                    }
                    else {
                        find_value[find_value_idx] = buffer[i];
                        ++find_value_idx;
                    }
                }
                find_value[find_value_idx] = 0;
                if (strcmp(read_buffer, find_key) == 0) {
                    break;
                }
            }
            write(new_fd, find_value, sizeof(find_value));
        }
    }

    fclose(file_db_pointer);

    return 0;
}

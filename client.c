#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

void parse_connect(int argc, char** argv, int* server_fd) {
    int opt;
    char* ip_addr = "127.0.0.1";
    int port = 25555;
    char* prog = argv[0];

    while ((opt = getopt(argc, argv, "i:p:h")) != -1) {
        switch (opt) {
            case 'i': ip_addr = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'h':
                printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n", prog);
                printf("-i IP_address Default to \"127.0.0.1\";\n");
                printf("-p port_number Default to 25555;\n");
                printf("-h Display this help info.\n");
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    *server_fd = sockfd;
}

int main(int argc, char** argv) {
    int server_fd;
    parse_connect(argc, argv, &server_fd);

    char buffer[1024];
    fd_set readfds;
    int maxfd = server_fd > STDIN_FILENO ? server_fd : STDIN_FILENO;

    int n = recv(server_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(server_fd);
        return 0;
    }
    buffer[n] = '\0';
    printf("%s", buffer);

    if (fgets(buffer, sizeof(buffer), stdin)) {
        send(server_fd, buffer, strlen(buffer), 0);
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(server_fd, &readfds);

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int r = recv(server_fd, buffer, sizeof(buffer) - 1, 0);
            if (r <= 0) break;
            buffer[r] = '\0';
            printf("%s", buffer);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            int r = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (r > 0) {
                buffer[r] = '\0';
                send(server_fd, buffer, strlen(buffer), 0);
            }
        }
    }

    close(server_fd);
    return 0;
}

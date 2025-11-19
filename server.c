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

#define MAX_PLAYERS 3
#define MAX_QUESTIONS 50
#define BUFFER_SIZE 1024

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player {
    int fd;
    int score;
    char name[128];
};

int read_questions(struct Entry* arr, char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening question file");
        exit(EXIT_FAILURE);
    }
    char line[1024];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(line) <= 1) continue;
        line[strcspn(line, "\n")] = '\0';
        strncpy(arr[count].prompt, line, sizeof(arr[count].prompt));
        if (!fgets(line, sizeof(line), fp)) break;
        char* token = strtok(line, " \n");
        for (int i = 0; i < 3 && token; ++i) {
            strncpy(arr[count].options[i], token, sizeof(arr[count].options[i]));
            token = strtok(NULL, " \n");
        }
        if (!fgets(line, sizeof(line), fp)) break;
        line[strcspn(line, "\n")] = '\0';
        for (int i = 0; i < 3; ++i) {
            if (strcmp(line, arr[count].options[i]) == 0) {
                arr[count].answer_idx = i;
                break;
            }
        }
        count++;
        if (count >= MAX_QUESTIONS) break;
    }
    fclose(fp);
    return count;
}

int main(int argc, char** argv) {
    int opt;
    char* qfile = "qshort.txt";
    char* ip_addr = "127.0.0.1";
    int port = 25555;
    char* prog = argv[0];

    while ((opt = getopt(argc, argv, "f:i:p:h")) != -1) {
        switch (opt) {
            case 'f': qfile = optarg; break;
            case 'i': ip_addr = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'h':
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", prog);
                printf("-f question_file Default to \"qshort.txt\";\n");
                printf("-i IP_address Default to \"127.0.0.1\";\n");
                printf("-p port_number Default to 25555;\n");
                printf("-h Display this help info.\n");
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    int tower_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (tower_socket_fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in tower_addr;
    memset(&tower_addr, 0, sizeof(tower_addr));
    tower_addr.sin_family      = AF_INET;
    tower_addr.sin_port        = htons(port);
    if (inet_pton(AF_INET, ip_addr, &tower_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        exit(EXIT_FAILURE);
    }

    if (bind(tower_socket_fd, (struct sockaddr*)&tower_addr, sizeof(tower_addr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }

    if (listen(tower_socket_fd, MAX_PLAYERS) == 0) {
        printf("Welcome to 392 Trivia!\n");
    } else {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct Entry questions[MAX_QUESTIONS];
    int qcount = read_questions(questions, qfile);

    struct Player players[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        players[i].fd = -1;
        players[i].score = 0;
        players[i].name[0] = '\0';
    }

    struct sockaddr_in incoming_msg_addr;
    socklen_t addr_size = sizeof(incoming_msg_addr);

    for (int i = 0; i < MAX_PLAYERS; ++i) {
        int client_fd = accept(tower_socket_fd,
                               (struct sockaddr*)&incoming_msg_addr,
                               &addr_size);
        if (client_fd < 0) { perror("accept"); exit(EXIT_FAILURE); }

        printf("New connection detected!\n");
        const char* ask = "Please type your name:\n";
        send(client_fd, ask, strlen(ask), 0);

        char namebuf[128];
        int recvbytes = recv(client_fd, namebuf, sizeof(namebuf) - 1, 0);
        if (recvbytes <= 0) {
            printf("Lost connection!\n");
            close(client_fd);
            --i;
            continue;
        }
        namebuf[recvbytes] = '\0';
        namebuf[strcspn(namebuf, "\n")] = '\0';
        printf("Hi %s!\n", namebuf);

        players[i].fd = client_fd;
        strncpy(players[i].name, namebuf, sizeof(players[i].name));
    }
    printf("The game starts now!\n");

    for (int q = 0; q < qcount; ++q) {
        printf("Question %d: %s\n", q + 1, questions[q].prompt);
        for (int j = 0; j < 3; ++j) {
            printf("%d: %s\n", j + 1, questions[q].options[j]);
        }
        char msg[2048];
        int len = snprintf(msg, sizeof(msg),
            "Question %d: %s\n"
            "Press 1: %s\n"
            "Press 2: %s\n"
            "Press 3: %s\n",
            q + 1, questions[q].prompt,
            questions[q].options[0],
            questions[q].options[1],
            questions[q].options[2]
        );
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            if (players[i].fd != -1)
                send(players[i].fd, msg, len, 0);
        }

        int answered = -1;
        while (answered == -1) {
            fd_set active_fd_set;
            FD_ZERO(&active_fd_set);
            FD_SET(tower_socket_fd, &active_fd_set);
            int max_socket = tower_socket_fd;
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                if (players[i].fd > -1) {
                    FD_SET(players[i].fd, &active_fd_set);
                    if (players[i].fd > max_socket)
                        max_socket = players[i].fd;
                }
            }

            if (select(max_socket + 1, &active_fd_set, NULL, NULL, NULL) < 0) {
                perror("select()");
                exit(EXIT_FAILURE);
            }

            if (FD_ISSET(tower_socket_fd, &active_fd_set)) {
                int new_fd = accept(tower_socket_fd,
                                    (struct sockaddr*)&incoming_msg_addr,
                                    &addr_size);
                if (new_fd >= 0) {
                    printf("Max connection reached!\n");
                    close(new_fd);
                }
                continue;
            }

            for (int i = 0; i < MAX_PLAYERS; ++i) {
                if (players[i].fd > -1 && FD_ISSET(players[i].fd, &active_fd_set)) {
                    char buf[BUFFER_SIZE];
                    int r = recv(players[i].fd, buf, sizeof(buf) - 1, 0);
                    if (r <= 0) {
                        printf("Lost connection!\n");
                        close(players[i].fd);
                        players[i].fd = -1;
                    } else {
                        buf[r] = '\0';
                        int choice = buf[0] - '0';
                        if (choice - 1 == questions[q].answer_idx)
                            players[i].score++;
                        else
                            players[i].score--;
                        answered = i;
                    }
                    break;
                }
            }
        }

        char answer_msg[256];
        snprintf(answer_msg, sizeof(answer_msg),
                 "Correct answer: %s\n",
                 questions[q].options[questions[q].answer_idx]);
        printf("%s", answer_msg);
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            if (players[i].fd != -1)
                send(players[i].fd, answer_msg, strlen(answer_msg), 0);
        }
    }

    int best = 0;
    for (int i = 1; i < MAX_PLAYERS; ++i)
        if (players[i].score > players[best].score)
            best = i;
    printf("Congrats, %s!\n", players[best].name);

    for (int i = 0; i < MAX_PLAYERS; ++i)
        if (players[i].fd != -1)
            close(players[i].fd);
    close(tower_socket_fd);
    return 0;
}

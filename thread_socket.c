#include "thread_socket.h"

pthread_attr_t attr = {0};
int LOG = 0;
int local_fd = 0;
char ip[16] = {0};

int main(int argc, char **argv) {
    struct sockaddr_in local_addr = {0};
    int on = 1;
    int opt = 0;
    int daemon = 0;
    unsigned int port = 8080;
    pid_t pid = 0;
    pid_t sid = 0;

    if (argc == 1)
        usage(*argv, EXIT_FAILURE);
    for (;(opt = getopt(argc, argv, "p:u:r:dlh")) != -1;) {
        switch(opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                daemon = 1;
                break;
            case 'l':
                LOG = 1;
                break;
            case 'u':
                if (setuid(atoi(optarg)))
                    perror("Setuid error");
                break;
            case 'r':
                strncpy(ip, optarg, 15);
                break;
            case 'h':
                usage(*argv, EXIT_SUCCESS);
                break;
            case ':':
                printf("Missing argument after: -%c\n", optopt);
                usage(*argv, EXIT_FAILURE);
                break;
            case '?':
                printf("Invalid argument: -%c\n", optopt);
                usage(*argv, EXIT_FAILURE);
            default:
                usage(*argv, EXIT_FAILURE);
        }
    }

    if (*ip == '\0')
        strncpy(ip, SERVER_ADDR, 15);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, signal_terminate);
    if ((local_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Create local server socket file descriptor fail");
        close(local_fd);
        exit(EXIT_FAILURE);
    }
    setsockopt(local_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    memset(&local_addr, '\0', sizeof(struct sockaddr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(local_fd, (const struct sockaddr *)&local_addr, sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "Bind %s:%u error", inet_ntoa(local_addr.sin_addr), port);
        perror(" ");
        exit(EXIT_FAILURE);
        close(local_fd);
    }
    if (listen(local_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
        close(local_fd);
    }
    printf("Listen on %s:%u.\n", inet_ntoa(local_addr.sin_addr), port);
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 256 * 1024);
    if (daemon) {
        if ((pid = fork()) == 0) {
            if ((sid = setsid()) < 0) {
                perror("setsid error");
                exit(EXIT_FAILURE);
            }
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            main_loop(local_fd);
        } else {
            printf("The PID of %s is %d.\n", *argv, pid);
            close(local_fd);
        }
    } else
        main_loop(local_fd);

    return 0;
}


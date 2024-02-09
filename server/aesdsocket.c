#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SZ 1024
#define PORT 9000
#define TEST_FILE "/var/tmp/aesdsocketdata"

volatile sig_atomic_t running = 1;

void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        running = 0;
        syslog(LOG_INFO, "Caught signal, exiting");
    }
}

void cleanup(int socket_server) {
    syslog(LOG_INFO, "Cleaning up and exiting");

    if (shutdown(socket_server, SHUT_RDWR) == -1) {
        perror("Error shutting down server socket");
    }

    if (close(socket_server) == -1) {
        perror("Error closing server socket");
    }

    unlink(TEST_FILE);

    closelog();
}

void daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error forking");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent process exits
        exit(EXIT_SUCCESS);
    }

    // Child process continues
    umask(0);
    if (setsid() < 0) {
        perror("Error setting session ID");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void handle_client(int client_socket, int socket_server)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if (getpeername(client_socket, (struct sockaddr*)&client_addr, &client_addr_len) == 0) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    }

    char buffer[BUFFER_SZ];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), /*flags=*/0)) > 0) {
        syslog(LOG_INFO, "Received %s with length %ld", buffer, bytes_received);

        int data_fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        write(data_fd, buffer, bytes_received);
        close(data_fd);

        if (memchr(buffer, '\n', bytes_received) != NULL) {
            int read_fp = open(TEST_FILE, O_RDONLY);
            if (read_fp == -1) {
                perror("Error opening data file for reading");
                cleanup(socket_server);
                exit(EXIT_FAILURE);
            }

            char file_buffer[BUFFER_SZ];
            ssize_t bytes_read;

            while ((bytes_read = read(read_fp, file_buffer, sizeof(file_buffer))) > 0) {
                syslog(LOG_INFO, "Read %s with length %ld", file_buffer, bytes_read);
                send(client_socket, file_buffer, bytes_read, 0);
            }

            close(read_fp);
            break;
        }
    }

    syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
    close(client_socket);
}

int main(int argc, char* argv[])
{
    int daemon_mode = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Starting aesdsocket");

    // Set up signal handlers using sigaction for better control
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error setting up signal handlers");
        return -1;
    }

    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server == -1) {
        perror("Error creating socket");
        cleanup(socket_server);
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    const int enable = 1;
    if (setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        cleanup(socket_server);
        return -1;
    }

    if (bind(socket_server, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind socket.");
        cleanup(socket_server);
        return -1;
    }

    if (listen(socket_server, /*backlog=*/5) == -1) {
        perror("Failed to listen connections.");
        cleanup(socket_server);
        return -1;
    }

    if (daemon_mode) {
        daemonize();
    }

    while (running == 1) {
        int client_socket = accept(socket_server, /*sockaddr=*/NULL, /*addrlen=*/NULL);
        if (client_socket == -1) {
            perror("Unable to accept connection.");
            cleanup(socket_server);
            return -1;
        }

        handle_client(client_socket, socket_server);
    }

    cleanup(socket_server);
    return 0;
}

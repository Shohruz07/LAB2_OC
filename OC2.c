#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

volatile sig_atomic_t g_got_sighup = 0;
int g_accepted_socket = -1;

void handle_signal(int in_signum) {
    if (in_signum == SIGHUP) {
        g_got_sighup = 1;
    } else {
        printf("Received signal %d\n", in_signum);
    }
}

int main() {
    struct sigaction signal_action;
    signal_action.sa_handler = handle_signal;
    signal_action.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &signal_action, NULL);

    // Блок создания сокета
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080\n");

    char buffer[1024];

    fd_set read_fds;
    FD_ZERO(&read_fds);

    // Блок обработки
    while (1) {
        if (g_got_sighup) {
            printf("Received SIGHUP signal. Closing the server.\n");
            if (g_accepted_socket != -1) {
                close(g_accepted_socket);
            }
            exit(0);
        }

        FD_SET(server_socket, &read_fds);

        struct timespec timeout;
        timeout.tv_sec = 1;
        timeout.tv_nsec = 0;
        
        sigset_t mask;
        int result = pselect(server_socket + 1, &read_fds, NULL, NULL, &timeout, &mask);

        if (result == -1) {
            if (errno == EINTR) {
                printf("'pselect' was interrupted by a signal.\n");
                continue;
            } else {
                perror("Error in pselect");
                break;
            }
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            if ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len)) == -1) {
                perror("Error accepting connection");
                continue;
            }

            printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            if (g_accepted_socket == -1) {
                g_accepted_socket = client_socket;
            } else {
                close(client_socket);
            }
        }

        size_t bytes_received = recv(g_accepted_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Received %ld bytes: %s\n", bytes_received, buffer);
        } else if (bytes_received == 0) {
            printf("Connection closed by client.\n");
            close(g_accepted_socket);
            g_accepted_socket = -1;
        } 
        else {
            perror("Error receiving data");
        }
    }
    return 0;
}
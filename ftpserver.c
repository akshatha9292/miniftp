#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 2121
#define BUFFER_SIZE 1024

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_received] = '\0';

        if (strncmp(buffer, "UPLOAD ", 7) == 0) {
            char *filename = buffer + 7;
            FILE *fp = fopen(filename, "wb");
            if (fp == NULL) {
                perror("File open error");
                send(client_socket, "File open error", 16, 0);
                continue;
            }

            printf("Uploading file: %s\n", filename);

            while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
                if (strncmp(buffer, "END", 3) == 0) break;
                fwrite(buffer, sizeof(char), bytes_received, fp);
            }

            fclose(fp);
            send(client_socket, "Upload complete", 15, 0);
            printf("Upload complete: %s\n", filename);

        } else if (strncmp(buffer, "DOWNLOAD ", 9) == 0) {
            char *filename = buffer + 9;
            FILE *fp = fopen(filename, "rb");
            if (fp == NULL) {
                perror("File open error");
                send(client_socket, "File open error", 16, 0);
                continue;
            }

            printf("Downloading file: %s\n", filename);

            while ((bytes_received = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
                send(client_socket, buffer, bytes_received, 0);
            }

            send(client_socket, "END", 3, 0);
            fclose(fp);
            printf("Download complete: %s\n", filename);

        } else if (strncmp(buffer, "VIEW ", 5) == 0) {
            char *filename = buffer + 5;
            FILE *fp = fopen(filename, "rb");
            if (fp == NULL) {
                perror("File open error");
                send(client_socket, "File open error", 16, 0);
                continue;
            }

            send(client_socket, "Viewing file", 13, 0);
            while ((bytes_received = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
                send(client_socket, buffer, bytes_received, 0);
            }

            send(client_socket, "END", 3, 0);
            fclose(fp);
            printf("View complete: %s\n", filename);

        } else if (strncmp(buffer, "LIST", 4) == 0) {
            // Execute the 'ls -l' command in the current directory
            FILE *fp = popen("ls -l", "r");
            if (fp == NULL) {
                perror("popen error");
                continue;
            }

            while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                send(client_socket, buffer, strlen(buffer), 0);
            }

            pclose(fp);
            send(client_socket, "END", 3, 0);
            printf("List complete\n");

        } else if (strncmp(buffer, "DELETE ", 7) == 0) {
            char *filename = buffer + 7;
            if (remove(filename) == 0) {
                send(client_socket, "Delete complete", 16, 0);
                printf("Deleted file: %s\n", filename);
            } else {
                perror("Delete error");
                send(client_socket, "Delete failed", 13, 0);
            }

        } else if (strncmp(buffer, "EXIT", 4) == 0) {
            printf("Client exited\n");
            break;
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t thread_id;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("FTP Server listening on port %d\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        int *new_client_socket = malloc(sizeof(int));
        if (new_client_socket == NULL) {
            perror("Memory allocation failed");
            close(client_socket);
            continue;
        }

        *new_client_socket = client_socket;
        if (pthread_create(&thread_id, NULL, handle_client, new_client_socket) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            free(new_client_socket);
        }
        pthread_detach(thread_id);
    }

    close(server_socket);
    return 0;
}





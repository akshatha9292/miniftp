#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1" // Change to the server's IP address
#define PORT 2121
#define BUFFER_SIZE 1024

void upload_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("File open error");
        return;
    }
    send(sockfd, "UPLOAD ", 7, 0);
    send(sockfd, filename, strlen(filename), 0);
    while (fread(buffer, sizeof(char), sizeof(buffer), fp) > 0) {
        send(sockfd, buffer, sizeof(buffer), 0);
    }
    send(sockfd, "END", 3, 0);
    fclose(fp);
    printf("Upload complete\n");
}

void download_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    send(sockfd, "DOWNLOAD ", 9, 0);
    send(sockfd, filename, strlen(filename), 0);
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("File open error");
        return;
    }
    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        if (strncmp(buffer, "END", 3) == 0) break;
        fwrite(buffer, sizeof(char), bytes_received, fp);
    }
    fclose(fp);
    printf("Download complete\n");
}

void view_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    send(sockfd, "VIEW ", 5, 0);
    send(sockfd, filename, strlen(filename), 0);

    printf("Viewing file: %s\n", filename);

    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        if (strncmp(buffer, "END", 3) == 0) break;
        fwrite(buffer, sizeof(char), bytes_received, stdout); // Write to stdout directly
    }
    printf("\nView complete\n");
}

void list_files(int sockfd) {
    char buffer[BUFFER_SIZE];
    send(sockfd, "LIST", 4, 0);
    while (recv(sockfd, buffer, sizeof(buffer), 0) > 0) {
        if (strncmp(buffer, "END", 3) == 0) break;
        printf("%s", buffer);
    }
}

void delete_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    send(sockfd, "DELETE ", 7, 0);
    send(sockfd, filename, strlen(filename), 0);
    recv(sockfd, buffer, sizeof(buffer), 0);
    printf("%s\n", buffer);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char command[10], filename[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("Enter command (UPLOAD/DOWNLOAD/VIEW/LIST/DELETE/EXIT): ");
        scanf("%s", command);
        
        if (strcmp(command, "EXIT") == 0) {
            send(sockfd, "EXIT", 4, 0);
            break;
        }
        
        if (strcmp(command, "UPLOAD") == 0) {
            printf("Enter filename to upload: ");
            scanf("%s", filename);
            upload_file(sockfd, filename);
        } else if (strcmp(command, "DOWNLOAD") == 0) {
            printf("Enter filename to download: ");
            scanf("%s", filename);
            download_file(sockfd, filename);
        } else if (strcmp(command, "VIEW") == 0) {
            printf("Enter filename to view: ");
            scanf("%s", filename);
            view_file(sockfd, filename);
        } else if (strcmp(command, "LIST") == 0) {
            list_files(sockfd);
        } else if (strcmp(command, "DELETE") == 0) {
            printf("Enter filename to delete: ");
            scanf("%s", filename);
            delete_file(sockfd, filename);
        } else {
            printf("Invalid command\n");
        }
    }

    close(sockfd);
    return 0;
}

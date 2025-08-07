#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // close
#include <arpa/inet.h>   // socket, bind, listen, accept

#define PORT 8080
#define RESPONSE \
    "HTTP/1.1 200 OK\r\n" \
    "Content-Type: text/html\r\n" \
    "Connection: close\r\n\r\n" \
    "<html><body><h1>Hello from C!</h1></body></html>\r\n"

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен на http://localhost:%d\n", PORT);

    client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len);
    if (client_fd < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    read(client_fd, buffer, sizeof(buffer));
    printf("Запрос от клиента:\n%s\n", buffer);

    write(client_fd, RESPONSE, strlen(RESPONSE));

    close(client_fd);
    close(server_fd);

    return 0;
}

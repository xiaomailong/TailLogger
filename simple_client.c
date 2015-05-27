#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    int socket_fd;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    int bytes_received, bytes_sent, integer_buffer;
    socklen_t address_length;

    if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("client: socket");
        return 1;
    }

    memset(&client_address, 0, sizeof(struct sockaddr_un));
    client_address.sun_family = AF_UNIX;
    strcpy(client_address.sun_path, "/tmp/CI_UNIX_SOCK_CLIENT");

    unlink("/tmp/CI_UNIX_SOCK_CLIENT");
    if (bind(socket_fd, (const struct sockaddr *) &client_address,
        sizeof(struct sockaddr_un)) < 0)
    {
        perror("client: bind");
        return 1;
    }

    memset(&server_address, 0, sizeof(struct sockaddr_un));
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, "/tmp/CI_UNIX_SOCK_SRV");

    integer_buffer = 5;

    bytes_sent = sendto(socket_fd, (char *)&integer_buffer, sizeof(int), 0,
        (struct sockaddr *) &server_address,
        sizeof(struct sockaddr_un));

    address_length = sizeof(struct sockaddr_un);
    bytes_received = recvfrom(socket_fd, (char *)&integer_buffer, sizeof(int), 0,
        (struct sockaddr *) &(server_address),
        &address_length);

    close(socket_fd);

    if (bytes_received != sizeof(int))
    {
        printf("wrong size datagram\n");
        return 1;
    }

    printf("%d\n", integer_buffer);

    return 0;
}

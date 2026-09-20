/* Servidor NBD oldstyle minimo: somente leitura, um cliente por vez. */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define NBD_REQUEST_MAGIC 0x25609513U
#define NBD_REPLY_MAGIC   0x67446698U
#define NBD_CMD_READ      0U
#define NBD_CMD_WRITE     1U
#define NBD_CMD_DISC      2U
#define NBD_FLAG_HAS_FLAGS 1U
#define NBD_FLAG_READ_ONLY 2U
#define NBD_FLAG_FIXED_NEWSTYLE 1U
#define NBD_OPT_EXPORT_NAME 1U
#define MAX_REQUEST (1024U * 1024U)

struct nbd_request {
    uint32_t magic;
    uint32_t type;
    uint64_t handle;
    uint64_t from;
    uint32_t len;
} __attribute__((packed));

struct nbd_reply {
    uint32_t magic;
    uint32_t error;
    uint64_t handle;
} __attribute__((packed));

static uint64_t swap64(uint64_t value)
{
    return ((uint64_t)htonl((uint32_t)(value >> 32))) |
           ((uint64_t)htonl((uint32_t)value) << 32);
}

static int read_full(int fd, void *buffer, size_t length)
{
    unsigned char *cursor = buffer;
    while (length != 0) {
        ssize_t got = read(fd, cursor, length);
        if (got == 0)
            return 0;
        if (got < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        cursor += got;
        length -= (size_t)got;
    }
    return 1;
}

static int write_full(int fd, const void *buffer, size_t length)
{
    const unsigned char *cursor = buffer;
    while (length != 0) {
        ssize_t sent = write(fd, cursor, length);
        if (sent < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        cursor += sent;
        length -= (size_t)sent;
    }
    return 0;
}

static int discard_full(int fd, uint32_t length)
{
    unsigned char buffer[512];
    while (length != 0) {
        size_t chunk = length > sizeof(buffer) ? sizeof(buffer) : length;
        if (read_full(fd, buffer, chunk) != 1)
            return -1;
        length -= (uint32_t)chunk;
    }
    return 0;
}

static int send_reply(int net, uint64_t handle, uint32_t error)
{
    struct nbd_reply reply;
    reply.magic = htonl(NBD_REPLY_MAGIC);
    reply.error = htonl(error);
    reply.handle = handle;
    return write_full(net, &reply, sizeof(reply));
}

static int serve_client(int net, int image, uint64_t image_size, int newstyle)
{
    static const unsigned char password[8] = "NBDMAGIC";
    static const unsigned char zeroes[124] = { 0 };
    uint64_t magic = swap64(newstyle ? UINT64_C(0x49484156454f5054) :
                                       UINT64_C(0x0000420281861253));
    uint64_t size_be = swap64(image_size);
    uint32_t flags = htonl(NBD_FLAG_HAS_FLAGS | NBD_FLAG_READ_ONLY);

    if (write_full(net, password, sizeof(password)) ||
        write_full(net, &magic, sizeof(magic)))
        return -1;
    if (newstyle) {
        uint16_t global_flags = htons(NBD_FLAG_FIXED_NEWSTYLE);
        uint32_t client_flags, option, name_length;
        if (write_full(net, &global_flags, sizeof(global_flags)) ||
            read_full(net, &client_flags, sizeof(client_flags)) != 1 ||
            read_full(net, &magic, sizeof(magic)) != 1 ||
            read_full(net, &option, sizeof(option)) != 1 ||
            read_full(net, &name_length, sizeof(name_length)) != 1 ||
            swap64(magic) != UINT64_C(0x49484156454f5054) ||
            ntohl(option) != NBD_OPT_EXPORT_NAME ||
            ntohl(name_length) > 1024 ||
            discard_full(net, ntohl(name_length)))
            return -1;
        flags = htons(NBD_FLAG_HAS_FLAGS | NBD_FLAG_READ_ONLY);
        if (write_full(net, &size_be, sizeof(size_be)) ||
            write_full(net, &flags, sizeof(uint16_t)) ||
            write_full(net, zeroes, sizeof(zeroes)))
            return -1;
    } else if (write_full(net, &size_be, sizeof(size_be)) ||
               write_full(net, &flags, sizeof(flags)) ||
               write_full(net, zeroes, sizeof(zeroes)))
        return -1;

    for (;;) {
        struct nbd_request request;
        uint32_t command, length;
        uint64_t offset;
        int status = read_full(net, &request, sizeof(request));
        if (status != 1)
            return status;
        if (ntohl(request.magic) != NBD_REQUEST_MAGIC)
            return -1;

        command = ntohl(request.type) & 0xffffU;
        length = ntohl(request.len);
        offset = swap64(request.from);
        fprintf(stderr, "NBD req: cmd=%u off=%llu len=%u\n", command,
                (unsigned long long)offset, length);
        fflush(stderr);
        if (command == NBD_CMD_DISC)
            return 0;
        if (command == NBD_CMD_WRITE) {
            if (discard_full(net, length))
                return -1;
            if (send_reply(net, request.handle, EPERM))
                return -1;
            continue;
        }
        if (command != NBD_CMD_READ || length > MAX_REQUEST ||
            offset > image_size || length > image_size - offset) {
            if (send_reply(net, request.handle, EINVAL))
                return -1;
            continue;
        }

        if (send_reply(net, request.handle, 0))
            return -1;
        while (length != 0) {
            unsigned char buffer[4096];
            size_t chunk = length > sizeof(buffer) ? sizeof(buffer) : length;
            ssize_t got = pread(image, buffer, chunk, (off_t)offset);
            if (got != (ssize_t)chunk)
                return -1;
            if (write_full(net, buffer, chunk))
                return -1;
            offset += chunk;
            length -= (uint32_t)chunk;
        }
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in address;
    int image, listener, one = 1;
    unsigned long port;
    uint64_t image_size;

    signal(SIGPIPE, SIG_IGN);
    int newstyle = 0;
    int offset = 1;
    if (argc > 1 && strcmp(argv[1], "--newstyle") == 0) {
        newstyle = 1;
        offset++;
    }
    if (argc - offset != 3) {
        fprintf(stderr, "Uso: %s [--newstyle] PORTA ARQUIVO TAMANHO_BYTES\n", argv[0]);
        return 2;
    }
    port = strtoul(argv[offset], NULL, 10);
    if (port == 0 || port > 65535) {
        fprintf(stderr, "porta invalida\n");
        return 2;
    }
    image_size = strtoull(argv[offset + 2], NULL, 10);
    if (image_size == 0) {
        fprintf(stderr, "tamanho invalido\n");
        return 2;
    }
    image = open(argv[offset + 1], O_RDONLY);
    if (image < 0) {
        perror("arquivo");
        return 1;
    }
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0 || setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) ||
        (memset(&address, 0, sizeof(address)), address.sin_family = AF_INET,
         address.sin_addr.s_addr = htonl(INADDR_ANY), address.sin_port = htons((uint16_t)port),
         bind(listener, (struct sockaddr *)&address, sizeof(address))) || listen(listener, 1)) {
        perror("socket/bind/listen");
        return 1;
    }
    fprintf(stderr, "NBD %s read-only: porta %lu, arquivo %s, %llu bytes\n",
            newstyle ? "newstyle" : "oldstyle", port, argv[offset + 1],
            (unsigned long long)image_size);
    for (;;) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR)
                continue;
            perror("accept");
            return 1;
        }
        (void)serve_client(client, image, image_size, newstyle);
        close(client);
    }
}

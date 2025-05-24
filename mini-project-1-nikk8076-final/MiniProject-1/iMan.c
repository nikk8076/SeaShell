#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pcre.h>
#include "iMan.h"

#define BUFFER_SIZE 4096
#define HOST "man.he.net"
#define PORT "80"

void remove_html_tags(const char *input, char *output, size_t output_size) {
    pcre *re;
    pcre_extra *extra;
    const char *error;
    int erroffset;
    const char *pattern = "<[^>]*>";
    int rc;
    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (!re) {
        fprintf(stderr, "PCRE compilation failed: %s\n", error);
        strncpy(output, input, output_size - 1);
        output[output_size - 1] = '\0';
        return;
    }
    extra = pcre_study(re, 0, &error);
    if (error) {
        fprintf(stderr, "PCRE study failed: %s\n", error);
        pcre_free(re);
        strncpy(output, input, output_size - 1);
        output[output_size - 1] = '\0';
        return;
    }
    const char *subject = input;
    char *output_ptr = output;
    int ovector[30];
    int subject_length = strlen(subject);
    size_t remaining_size = output_size - 1;
    while ((rc = pcre_exec(re, extra, subject, subject_length, 0, 0, ovector, sizeof(ovector) / sizeof(ovector[0]))) >= 0) {
        if (rc == 0) rc = sizeof(ovector) / sizeof(ovector[0]) - 1;
        int start = ovector[0];
        int end = ovector[1];
        int copy_length = start < remaining_size ? start : remaining_size;
        memcpy(output_ptr, subject, copy_length);
        output_ptr += copy_length;
        remaining_size -= copy_length;
        subject += end;
        subject_length -= end;
        if (remaining_size <= 0) {
            break;
        }
    }
    if (remaining_size > 0) {
        strncpy(output_ptr, subject, remaining_size);
        output_ptr[remaining_size] = '\0';
    }
    pcre_free(re);
    if (extra) pcre_free(extra);
}

void iMan(const char *command_name) {
    int sockfd;
    struct addrinfo hints, *res, *p;
    char buffer[BUFFER_SIZE];
    char clean_buffer[BUFFER_SIZE];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status;
    if ((status = getaddrinfo(HOST, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return;
    }
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) {
            break;
        }
        perror("connect");
        close(sockfd);
    }
    if (p == NULL) {
        fprintf(stderr, "Failed to connect\n");
        return;
    }
    freeaddrinfo(res);
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request),"GET /?topic=%s&section=all HTTP/1.1\r\n""Host: %s\r\n""Connection: close\r\n\r\n",command_name, HOST);
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        close(sockfd);
        return;
    }    
    int bytes_received;
    int total_bytes = 0;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        remove_html_tags(buffer, clean_buffer, sizeof(clean_buffer));
        printf("%s", clean_buffer);
        total_bytes += bytes_received;
    }
    if (bytes_received == -1) {
        perror("recv");
    }    
    close(sockfd);
}

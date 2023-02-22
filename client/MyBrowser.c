#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include <poll.h>
#include <sys/wait.h>
#define __USE_XOPEN
#include <time.h>

char *browser = "google-chrome";
char *adobe = "acroread";
char *img_def = "eog";
char *gedit = "gedit";

#define POLL_TIMEOUT 3000

void get_put_http_response(char *, int, char *, char *);

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;

    char url[1000];
    char host[100];
    int port;
    char file[100];
    char filename[100];
    char method[100];
    char rem[100];

    while (1)
    {
        printf("MyOwnBrowser> ");
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            perror("unable to create socket\n");
            exit(1);
        }

        fgets(url, 1000, stdin);

        if (strncmp(url, "QUIT", 4) == 0)
        {
            printf("Exiting...\n\n");
            break;
        }
        port = -1;
        if (strncmp(url, "GET", 3) == 0)
        {
            sscanf(url, "%s http://%[^/]/%[^:\n]:%d\n", method, host, file, &port);
        }
        else if (strncmp(url, "PUT", 3) == 0)
        {
            sscanf(url, "%s http://%[^/]/%[^ :]%[^\n]", method, host, file, rem);
            // printf("\n%s\n", rem);
            if (rem[0] == ':')
            {
                sscanf(rem, ":%d %s", &port, filename);
            }
            else
                sscanf(rem, "%s", filename);

            strcat(file, "/");
            strcat(file, basename(filename));
        }
        else
        {
            printf("Invalid Command\n: Usage:\n1. GET {url}\n2. PUT {url}\n3. QUIT\n");
            continue;
        }
        if (port == -1)
            port = 80;

        memset(&serv_addr, 0, sizeof(serv_addr));

        // connecting to the server
        char *host_name = (char *)malloc(strlen(host) + 1);
        strcpy(host_name, host);
        serv_addr.sin_family = AF_INET;
        struct hostent *host_ = gethostbyname(host);
        memcpy(&serv_addr.sin_addr.s_addr, host_->h_addr, host_->h_length);
        serv_addr.sin_port = htons(port);

        // printf("server:%s %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

        if ((connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
        {
            perror("Unable to connect to server\n");
            exit(0);
        }

        time_t now;
        struct tm *now_tm;
        char date[100];
        now = time(NULL);
        now_tm = gmtime(&now);
        strftime(date, sizeof(date), "%a, %d %b %Y %T GMT", now_tm);

        if (strcmp(method, "GET") == 0)
        {
            char *extension = (char *)malloc(20);
            char *accept = (char *)malloc(20);
            char *buffer = (char *)malloc(1000);

            extension = strrchr(file, '.');
            if (extension == NULL)
            {
                // strcat(file, "index.html");
                strcpy(accept, "text/*");
            }
            else if (strcmp(extension, ".html") == 0)
            {
                strcpy(accept, "text/html");
            }
            else if (strcmp(extension, ".pdf") == 0)
            {
                strcpy(accept, "application/pdf");
            }
            else if (strcmp(extension, ".jpg") == 0)
            {
                strcpy(accept, "image/jpeg");
            }
            else
            {
                strcpy(accept, "text/*");
            }

            sprintf(buffer, "GET /%s HTTP/1.1\r\n", file);
            sprintf(buffer + strlen(buffer), "Host: %s\r\n", host_name);
            sprintf(buffer + strlen(buffer), "Connection: close\r\n");
            sprintf(buffer + strlen(buffer), "Date: %s\r\n", date);
            sprintf(buffer + strlen(buffer), "Accept: %s\r\n", accept);
            sprintf(buffer + strlen(buffer), "Accept-Language: en-US,en;q=0.5\r\n");
            now -= 2 * 24 * 60 * 60;
            now_tm = gmtime(&now);
            strftime(date, sizeof(date), "%a, %d %b %Y %T GMT", now_tm);
            sprintf(buffer + strlen(buffer), "If-Modified-Since: %s\r\n\r\n", date);

            send(sockfd, buffer, strlen(buffer) + 1, 0);

            // printf("HTTP REQUEST:\n%s\n", buffer);

            struct pollfd fds[1];
            fds[0].fd = sockfd;
            fds[0].events = POLLIN;
            fds[0].revents = 0;
            int ret = poll(fds, 1, POLL_TIMEOUT);

            if (ret == 0)
            {
                printf("TIMELIMIT EXCEEDED\n");
                close(sockfd);
            }
            else if (ret == -1)
            {
                perror("poll\n");
                close(sockfd);
                exit(1);
            }
            else if (fds[0].revents && POLLIN)
            {
                char buff[100];
                char *response = NULL;
                int size = 0;
                int total_len = 0;
                while ((size = recv(sockfd, buff, 100, 0)) > 0)
                {
                    total_len += size;
                    if (response == NULL)
                    {
                        response = (char *)malloc(size);
                        memcpy(response, buff, size);
                    }
                    else
                    {
                        response = (char *)realloc(response, total_len);
                        memcpy(response + total_len - size, buff, size);
                    }
                }
                close(sockfd);
                get_put_http_response(response, total_len, file, "GET");
            }
        }
        else if (strcmp(method, "PUT") == 0)
        {
            char *extension = (char *)malloc(100);
            char *content_type = (char *)malloc(100);

            extension = strrchr(file, '.');
            if (extension == NULL)
            {
                strcpy(content_type, "text/*");
            }
            else if (strcmp(extension, ".html") == 0)
            {
                strcpy(content_type, "text/html");
            }
            else if (strcmp(extension, ".pdf") == 0)
            {
                strcpy(content_type, "application/pdf");
            }
            else if (strcmp(extension, ".jpg") == 0)
            {
                strcpy(content_type, "image/jpeg");
            }
            else
            {
                strcpy(content_type, "text/*");
            }

            FILE *file_put = fopen(filename, "rb");
            if (!file_put)
            {
                printf("Error opening file\n");
            }
            else
            {
                fseek(file_put, 0, SEEK_END);
                long content_size = ftell(file_put);
                rewind(file_put);

                char *content = (char *)malloc(content_size);
                fread(content, content_size, 1, file_put);
                fclose(file_put);
                char header[1000];
                sprintf(header, "PUT /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nDate: %s\r\nContent-Language: en-US\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", file, host_name, date, content_type, content_size);

                char *request = (char *)malloc(strlen(header) + content_size + 2);
                strcpy(request, header);
                memcpy(request + strlen(header), content, content_size);

                send(sockfd, request, strlen(header) + content_size, 0);

                // printf("HTTP REQUEST:\n%s\n", header);

                struct pollfd fds[1];
                fds[0].fd = sockfd;
                fds[0].events = POLLIN;
                fds[0].revents = 0;
                int ret = poll(fds, 1, POLL_TIMEOUT);

                if (ret == 0)
                {
                    printf("TIMELIMIT EXCEEDED\n");
                    close(sockfd);
                }
                else if (ret == -1)
                {
                    perror("poll\n");
                    close(sockfd);
                    exit(1);
                }
                else if (fds[0].revents && POLLIN)
                {
                    char buff[100];
                    char *response = NULL;
                    int size = 0;
                    int total_len = 0;
                    printf("1\n");

                    while ((size = recv(sockfd, buff, 100, 0)) > 0)
                    {
                        total_len += size;
                        if (response == NULL)
                        {
                            response = (char *)malloc(size);
                            memcpy(response, buff, size);
                        }
                        else
                        {
                            response = (char *)realloc(response, total_len);
                            memcpy(response + total_len - size, buff, size);
                        }
                    }
                    close(sockfd);
                    get_put_http_response(response, total_len, file, "PUT");
                }
            }
        }
    }
}

void get_put_http_response(char *response, int total_len, char *filename, char *method)
{
    char *content_lang = (char *)malloc(100);
    char *content_type = (char *)malloc(100);
    long Content_length;
    int status_code;
    char *protocol;
    char *status_msg = (char *)malloc(100);
    char *cache_control = (char *)malloc(20);
    char *expires = (char *)malloc(30);
    char *last_modified = (char *)malloc(30);

    char *line_start = response;
    char *line_end;
    int content_size = 0;
    char *content = NULL;

    printf("HTTP RESPONSE:\n");
    while (1)
    {
        line_end = strstr(line_start, "\r\n");

        if (line_end == NULL)
        {
            break;
        }

        int line_length = line_end - line_start;
        char line[line_length + 1];
        memcpy(line, line_start, line_length);
        line[line_length] = '\0';
        printf("%s\n", line);

        if (strlen(line) == 0)
        {
            // printf("inside\n");
            break;
        }
        else if (strncmp(line, "HTTP/1.1", 8) == 0)
        {
            sscanf(line, "HTTP/1.1 %d %[^\r\n]", &status_code, status_msg);
        }
        else if (strncmp(line, "Content-Type:", 13) == 0)
        {
            strcpy(content_type, line + 14);
        }
        else if (strncmp(line, "Content-Length:", 15) == 0)
        {
            sscanf(line + 15, "%ld", &Content_length);
        }
        else if (strncmp(line, "Content-language:", 17) == 0)
        {
            strcpy(content_lang, line + 17);
        }
        else if (strncmp(line, "Cache-control:", 14) == 0)
        {
            sscanf(line, "Cache-control: %s", cache_control);
        }
        else if (strncmp(line, "Expires:", 8) == 0)
        {
            sscanf(line, "Expires: %[^\n]", expires);
        }
        else if (strncmp(line, "Last-modified:", 14) == 0)
        {
            sscanf(line, "Last-modified: %[^\n]", last_modified);
        }
        line_start = line_end + 2;
    }

    printf("STATUS: %d %s\n", status_code, status_msg);

    if (expires != NULL)
    {
        // printf("%s\n", expires);
        struct tm tm;
        strptime(expires, "%d-%m-%y:%H-%M-%S", &tm);
        tm.tm_isdst = 0;
        time_t time1 = mktime(&tm);
        time_t time2;
        time(&time2);

        // printf("%ld   %ld\n", time1, time2);
        double diff = difftime(time1, time2);
        if (diff < 0)
        {
            printf("content expires\n");
            return;
        }
    }

    char *header_end = strstr(response, "\r\n\r\n");
    if (header_end)
    {
        content_size = total_len - (header_end - response) - 4;
        content = (char *)malloc(content_size);
        memcpy(content, header_end + 4, content_size);
    }
    else
    {
        printf("empty content body\n");
        return;
    }

    char *output_file = (char *)malloc(100);

    if (strcmp(cache_control, "no-store") == 0)
    {
        mkdir("__temp__", 0700);
        if (strcmp(content_type, "text/html") == 0)
        {
            strcpy(output_file, "__temp__/___temp__.html");
        }
        else if (strcmp(content_type, "application/pdf") == 0)
        {
            strcpy(output_file, "__temp__/___temp__.pdf");
        }
        else if (strcmp(content_type, "image/jpeg") == 0)
        {
            strcpy(output_file, "__temp__/___temp__.jpg");
        }
        else
        {
            char *extension = (char *)malloc(20);
            extension = strrchr(filename, '.');
            if (extension != NULL)
            {
                strcpy(output_file, "__temp__");
                strcat(output_file, extension);
            }
            else
                strcpy(output_file, "__temp__/___temp__.txt");
        }
    }
    else
    {
        if (status_code != 200 || strcmp(method, "PUT") == 0)
        {
            if (strcmp(content_type, "text/html") == 0)
            {
                strcpy(output_file, "___error_msg.html");
            }
            else if (strcmp(content_type, "application/pdf") == 0)
            {
                strcpy(output_file, "___error_msg.pdf");
            }
            else if (strcmp(content_type, "image.jpeg") == 0)
            {
                strcpy(output_file, "___error_msg.jpg");
            }
            else
            {
                strcpy(output_file, "___error_msg.txt");
            }
        }
        else
        {
            strcpy(output_file, filename);
        }

        char *extension = (char *)malloc(50);

        extension = strrchr(output_file, '.');

        if (extension == NULL)
        {
            strcat(output_file, "index.html");
        }
        // printf("%s\n", output_file);
        char *temp = (char *)malloc(strlen(output_file) + 1);
        strcpy(temp, output_file);
        mkdir(dirname(temp), 0700);
    }

    FILE *file = fopen(output_file, "wb");

    if (!file)
    {
        printf("Error opening file\n");
    }
    else
    {
        fwrite(content, 1, content_size, file);
        fclose(file);
        if (fork() == 0)
        {
            if (strcmp(content_type, "text/html") == 0)
            {
                char *args[] = {browser, output_file, NULL};
                execvp(browser, args);
            }
            else if (strcmp(content_type, "application/pdf") == 0)
            {
                char *args[] = {adobe, output_file, NULL};
                execvp(adobe, args);
            }
            else if (strcmp(content_type, "image/jpeg") == 0)
            {
                char *args[] = {img_def, output_file, NULL};
                execvp(img_def, args);
            }
            else if (strcmp(content_type, "text/*") == 0)
            {
                char *args[] = {gedit, output_file, NULL};
                execvp(gedit, args);
            }
            else
            {
                printf("Unknown content_type sent by the server\n");
            }
        }
        int status;
        wait(&status);
        if (strcmp(cache_control, "no-store") == 0)
        {
            remove(output_file);
            printf("file removed :: cache_control= no-store\n");
            // delete the output file
        }
    }
}

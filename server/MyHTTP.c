#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#define __USE_XOPEN
#include <time.h>

#define PORT 8080

/*
    there are some file which client can not access like, status_handler, access_log.txt and can increase if the
    server functionalities increase if client tries to access those then 403 error will be given
    GET:
    200: file found
    400: syntax error or bad request
    403: forbidden (if client try to access the status_handler folder of server or access log)
    404: file not found
    PUT:
    200: directory exist
    400: directory doesn't exits (bad request)
    403: same if client wants to access status_handler folder of server or the access log
    404: no need

    ** this server doesn't give permission to create folder so if put request is there in a directory that doesn't
    exist it will give bad request error(400)
*/

typedef struct response
{
    char *response;
    int total_len;

} response;

response process_http_request(char *, int);

int main()
{
    int sockfd, newsockfd;
    int client;
    struct sockaddr_in cli_addr, serv_addr;
    int i;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("cannot create socket\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("unable to bind local address\n");
        exit(0);
    }
    listen(sockfd, 10);

    while (1)
    {
        client = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&client);

        if (newsockfd < 0)
        {
            perror("Accept error");
            exit(0);
        }
        int size = 0;
        int total_len = 0;
        char buff[1000];

        char *request = NULL;

        int len_ = 0;
        while (1)
        {
            size = recv(newsockfd, buff, sizeof(buff), 0);
            total_len += size;
            if (request == NULL)
            {
                request = (char *)malloc(size);
                memcpy(request, buff, size);
            }
            else
            {
                request = (char *)realloc(request, total_len);
                memcpy(request + total_len - size, buff, size);
            }
            if (strstr(request, "Content-Length:") != NULL)
            {
                char *content_length_header = strstr(request, "Content-Length:");
                sscanf(content_length_header, "Content-Length: %d", &len_);
            }
            char *header_end = strstr(request, "\r\n\r\n");
            if (header_end != NULL)
            {
                if ((total_len - (header_end - request) - 4) < len_)
                {
                    continue;
                }
                else
                    break;
            }
        }

        printf("%s", request);

        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        char date_time[20];
        strftime(date_time, sizeof(date_time), "%d-%m-%y : %H-%M-%S", t);
        char url[100];

        FILE *fp = fopen("AccessLog.txt", "a");

        if (fp == NULL)
        {
            printf("Could not open file AccessLog.txt\n");
            exit(1);
        }

        if (strncmp(request, "PUT", 3) == 0)
        {
            sscanf(request, "PUT %s HTTP/1.1", url);
            fprintf(fp, "%s : %s : %d : PUT : %s\n", date_time, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), url);
        }
        else if (strncmp(request, "GET", 3) == 0)
        {
            sscanf(request, "GET %s HTTP/1.1", url);
            fprintf(fp, "%s : %s : %d : GET : %s\n", date_time, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), url);
        }
        fclose(fp);
        struct response response = process_http_request(request, total_len);
        // printf("1\n");
        // sleep(5);
        send(newsockfd, response.response, response.total_len, 0);
        // printf("%s\n",response.response);

        close(newsockfd);
    }

    return 0;
}

response process_http_request(char *request, int total_len)
{
    char *response = NULL;
    int size_total = 0;
    char *line_start = request;
    char *line_end;

    time_t current_time;
    time(&current_time);
    current_time += 60 * 60 * 24 * 3;
    char expires_header[100];
    strftime(expires_header, sizeof(expires_header), "Expires: %d-%m-%y:%H-%M-%S", gmtime(&current_time));

    if (strncmp(request, "PUT", 3) == 0)
    {
        char *host = (char *)malloc(100);
        char *connection = (char *)malloc(100);
        char *date = (char *)malloc(100);
        char *content_lang = (char *)malloc(100);
        char *content_type = (char *)malloc(100);
        long Content_length;
        char *content_body = NULL;
        char *filename = (char *)malloc(100);
        char *protocol = (char *)malloc(100);

        printf("\nHTTP REQUEST RECEIVED:\n");
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
            else if (strncmp(line, "PUT", 3) == 0)
            {
                sscanf(line, "PUT /%s %s", filename, protocol);
            }
            else if (strncmp(line, "Host:", 5) == 0)
            {
                sscanf(line, "Host: %s", host);
            }
            else if (strncmp(line, "Connection:", 11) == 0)
            {
                sscanf(line, "Connection: %s", connection);
            }
            else if (strncmp(line, "Date:", 5) == 0)
            {
                sscanf(line, "Date: %[^\n]", date);
            }
            else if (strncmp(line, "Content-Language:", 17) == 0)
            {
                sscanf(line, "Content-Language: %s", content_lang);
            }
            else if (strncmp(line, "Content-Type:", 13) == 0)
            {
                sscanf(line, "Content-Type: %s", content_type);
            }
            else if (strncmp(line, "Content-Length:", 13) == 0)
            {
                sscanf(line, "Content-Length: %ld", &Content_length);
            }
            line_start = line_end + 2;
        }
        char *header_end = strstr(request, "\r\n\r\n");
        long content_size = 0;

        int flag = 0;

        if (header_end)
        {
            content_size = total_len - (header_end - request) - 4;
            // printf("%ld\n", content_size);
            content_body = (char *)malloc(content_size);
            memcpy(content_body, header_end + 4, content_size);
        }
        else
        {
            // 400 bad request // no content available
            flag = 1;
        }

        if (strncmp(filename, "status_handler/", 15) == 0 || strcmp(filename, "AccessLog.txt") == 0)
        {
            // 403 forbidden
            FILE *file = fopen("status_handler/forbidden.html", "rb");

            fseek(file, 0, SEEK_END);
            long content_size = ftell(file);
            rewind(file);

            char *content = (char *)malloc(content_size);
            fread(content, content_size, 1, file);
            fclose(file);

            char header[2000];
            sprintf(header, "HTTP/1.1 403 Forbidden\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n", expires_header, content_size);

            response = (char *)malloc(strlen(header) + content_size);
            strcpy(response, header);
            memcpy(response + strlen(header), content, content_size);
            size_total = strlen(header) + content_size;
        }
        else
        {
            FILE *file = fopen(filename, "wb");
            if (flag == 1 || !file)
            {
                // 400 bad request // directory doesn't exist
                FILE *file = fopen("status_handler/bad_request.html", "rb");

                fseek(file, 0, SEEK_END);
                long content_size = ftell(file);
                rewind(file);

                char *content = (char *)malloc(content_size);
                fread(content, content_size, 1, file);
                fclose(file);

                char header[2000];
                sprintf(header, "HTTP/1.1 400 Bad Request\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n", expires_header, content_size);

                response = (char *)malloc(strlen(header) + content_size);
                strcpy(response, header);
                memcpy(response + strlen(header), content, content_size);
                size_total = strlen(header) + content_size;
            }
            else
            {
                // 200 ok
                fwrite(content_body, 1, content_size, file);
                fclose(file);
                FILE *file = fopen("status_handler/success.html", "rb");

                fseek(file, 0, SEEK_END);
                long content_size = ftell(file);
                rewind(file);

                char *content = (char *)malloc(content_size);
                fread(content, content_size, 1, file);
                fclose(file);

                char header[2000];
                sprintf(header, "HTTP/1.1 200 Ok\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n", expires_header, content_size);

                response = (char *)malloc(strlen(header) + content_size);
                strcpy(response, header);
                memcpy(response + strlen(header), content, content_size);
                size_total = strlen(header) + content_size;
            }
        }
    }
    else if (strncmp(request, "GET", 3) == 0)
    {

        char *host = (char *)malloc(100);
        char *connection = (char *)malloc(100);
        char *date = (char *)malloc(100);
        char *accept = (char *)malloc(100);
        char *language = (char *)malloc(100);
        char *modified_date = (char *)malloc(100);
        char *filename = (char *)malloc(100);
        char *protocol = (char *)malloc(100);
        int flag = 1;

        printf("\nHTTP REQUEST RECEIVED:\n");
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
            else if (strncmp(line, "GET", 3) == 0)
            {
                // printf("1\n");
                sscanf(line, "GET /%s %s", filename, protocol);
            }
            else if (strncmp(line, "Host:", 5) == 0)
            {
                sscanf(line, "Host: %s", host);
            }
            else if (strncmp(line, "Connection:", 11) == 0)
            {
                sscanf(line, "Connection: %s", connection);
            }
            else if (strncmp(line, "Date:", 5) == 0)
            {
                sscanf(line, "Date: %s", date);
            }
            else if (strncmp(line, "Accept:", 7) == 0)
            {
                sscanf(line, "Accept: %s", accept);
            }
            else if (strncmp(line, "Accept-Language:", 16) == 0)
            {
                sscanf(line, "Accept-Language: %s", language);
            }
            else if (strncmp(line, "If-Modified-Since:", 18) == 0)
            {
                sscanf(line, "If-Modified-Since: %[^\n]", modified_date);
                flag = 0;
            }
            line_start = line_end + 2;
        }
        // printf("1\n");
        char *extension = (char *)malloc(50);

        extension = strrchr(filename, '.');

        if (extension == NULL)
        {
            strcat(filename, "index.html");
            strcpy(accept, "text/html");
        }
        if (strcmp(protocol, "HTTP/1.1") != 0)
        {
            // 400 bad request
            FILE *file = fopen("status_handler/bad_request.html", "rb");

            fseek(file, 0, SEEK_END);
            long content_size = ftell(file);
            rewind(file);

            char *content = (char *)malloc(content_size);
            fread(content, content_size, 1, file);
            fclose(file);

            char header[2000];
            sprintf(header, "HTTP/1.1 400 Bad Request\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n", expires_header, content_size);

            response = (char *)malloc(strlen(header) + content_size);
            strcpy(response, header);
            memcpy(response + strlen(header), content, content_size);
            size_total = strlen(header) + content_size;
        }
        else if (strncmp(filename, "status_handler/", 15) == 0 || strcmp(filename, "AccessLog.txt") == 0)
        {
            // 403 forbidden
            FILE *file = fopen("status_handler/forbidden.html", "rb");

            fseek(file, 0, SEEK_END);
            long content_size = ftell(file);
            rewind(file);

            char *content = (char *)malloc(content_size);
            fread(content, content_size, 1, file);
            fclose(file);

            char header[2000];
            sprintf(header, "HTTP/1.1 403 Forbidden\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n", expires_header, content_size);

            response = (char *)malloc(strlen(header) + content_size);
            strcpy(response, header);
            memcpy(response + strlen(header), content, content_size);
            size_total = strlen(header) + content_size;
        }
        else
        {
            FILE *file = fopen(filename, "rb");
            if (!file)
            {
                // 404 not found
                FILE *file = fopen("status_handler/not_found.html", "rb");

                fseek(file, 0, SEEK_END);
                long content_size = ftell(file);
                rewind(file);

                char *content = (char *)malloc(content_size);
                fread(content, content_size, 1, file);
                fclose(file);

                char header[2000];
                sprintf(header, "HTTP/1.1 404 Not Found\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n", expires_header, content_size);

                response = (char *)malloc(strlen(header) + content_size);
                strcpy(response, header);
                memcpy(response + strlen(header), content, content_size);
                size_total = strlen(header) + content_size;
            }
            else
            {
                // 200 ok

                // last-modified details:
                struct stat file_info;
                time_t file_stat;
                char last_mod[50];
                if (stat(filename, &file_info) == 0)
                {
                    strftime(last_mod, sizeof(last_mod), "%a, %d %b %Y %T GMT", gmtime(&file_info.st_mtime));
                    file_stat = file_info.st_mtime;
                    // printf("1\n");
                }

                // printf("%ld  %ld   %ld\n", file_stat, time1, file_info.st_mtime);

                if (flag == 0)
                {
                    struct tm tm;
                    // printf("%s\n", modified_date);
                    strptime(modified_date, "%a, %d %b %Y %T GMT", &tm);
                    tm.tm_isdst = 0;
                    time_t time1 = mktime(&tm);
                    if (difftime(file_stat, time1) < 0)
                    {
                        // printf("1\n");
                        fclose(file);
                        FILE *file = fopen("status_handler/bad_request.html", "rb");

                        fseek(file, 0, SEEK_END);
                        long content_size = ftell(file);
                        rewind(file);

                        char *content = (char *)malloc(content_size);
                        fread(content, content_size, 1, file);
                        fclose(file);

                        char header[2000];
                        sprintf(header, "HTTP/1.1 400 Bad Request\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\nLast-modified: %s\r\n\r\n", expires_header, content_size, last_mod);

                        response = (char *)malloc(strlen(header) + content_size);
                        strcpy(response, header);
                        memcpy(response + strlen(header), content, content_size);
                        size_total = strlen(header) + content_size;
                    }
                    else
                    {
                        flag = 1;
                    }
                }
                if (flag == 1)
                {

                    fseek(file, 0, SEEK_END);
                    long content_size = ftell(file);
                    rewind(file);

                    char *content = (char *)malloc(content_size);
                    fread(content, content_size, 1, file);
                    fclose(file);
                    extension = strrchr(filename, '.');
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
                    char header[2000];
                    sprintf(header, "HTTP/1.1 200 Ok\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: %s\r\nLast-modified: %s\r\n\r\n", expires_header, content_size, accept, last_mod);

                    response = (char *)malloc(strlen(header) + content_size);
                    strcpy(response, header);
                    memcpy(response + strlen(header), content, content_size);
                    size_total = strlen(header) + content_size;
                }
            }
        }
    }
    else
    {
        // 400 bad request
        FILE *file = fopen("status_handler/bad_request.html", "rb");

        fseek(file, 0, SEEK_END);
        long content_size = ftell(file);
        rewind(file);

        char *content = (char *)malloc(content_size);
        fread(content, content_size, 1, file);
        fclose(file);

        char header[1000];
        sprintf(header, "HTTP/1.1 400 Bad Request\r\n%s\r\nCache-control: no-store\r\nContent-language: en-US\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n", expires_header, content_size);

        response = (char *)malloc(strlen(header) + content_size);
        strcpy(response, header);
        memcpy(response + strlen(header), content, content_size);
        size_total = strlen(header) + content_size;
    }

    struct response ret;
    ret.response = response;
    ret.total_len = size_total;
    return ret;
}

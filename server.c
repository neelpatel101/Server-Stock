// Hamza Khalique 23076851
// Neel Patel 16752035

// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"

struct csvInfo
{
    char date[20];
    char price[20];
};

char **sh_split_line(char *line)
{
    int bufsize = SH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;

    token = strtok(line, SH_TOK_DELIM);
    while (token != NULL) 
    {
        tokens[position] = token;
        position++;

        token = strtok(NULL, SH_TOK_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

char* getField(char* line, int num)
{
    char* tok;

    for (tok = strtok(line, ","); tok && *tok; tok = strtok(NULL, ",\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

int main(int argc, char* argv[]) 
{
    int i, j;
    int size;
    char appleLine[1024];
    char twitterLine[1024];
    float appleProfits;
    float twitterProfits;
    FILE* applePtr;
    FILE* twitterPtr;
    int csvSize = 100000;
    struct csvInfo appleInfo[csvSize];
    struct csvInfo twitterInfo[csvSize];

    char** args;
    char* csv1 = argv[1];
    char* csv2 = argv[2];
    int PORT = atoi(argv[3]);
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
       
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(PORT); 

    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr*)&address,  
                                 sizeof(address)) < 0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    printf("server started\n"); 

    applePtr = fopen(csv1, "r");

    i = 0;
    j = 0;
    float min = 1000000.0;
    float maxProfit = 0;

    while (fgets(appleLine, 1024, applePtr))
    {
        char* line = strdup(appleLine);
        char prices[20] = {0};
        float temp = atof(getField(line, 5));

        if (i == 0)
        {
            i++;
            continue;
        }
        
        sprintf(prices, "%.2f", temp);

        if (min > temp)
            min = temp;
            
        else if (maxProfit < (temp - min))
            maxProfit = temp - min;
            
        strcpy(appleInfo[j].date, getField(line, 1));
        strcpy(appleInfo[j].price, prices);

        i++;
        j++;
    }

    appleProfits = maxProfit;
    fclose(applePtr);
    
    twitterPtr = fopen(csv2, "r");

    i = 0;
    j = 0;
    min = 1000000.0;
    maxProfit = 0;

    while (fgets(twitterLine, 1024, twitterPtr))
    {
        char* line = strdup(twitterLine);
        char prices[20] = {0};
        float temp = atof(getField(line, 5));

        if (i == 0)
        {
            i++;
            continue;
        }
        
        sprintf(prices, "%.2f", temp);

        if (min > temp)
            min = temp;
            
        else if (maxProfit < (temp - min))
            maxProfit = temp - min;
            
        strcpy(twitterInfo[j].date, getField(line, 1));
        strcpy(twitterInfo[j].price, prices);

        i++;
        j++;
    }

    twitterProfits = maxProfit;
    fclose(twitterPtr);

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address,  
                    (socklen_t*)&addrlen)) < 0) 
        { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        }

        while (1)
        {
            char buffer[256] = {0};
            char message[255] = {0};

            valread = read(new_socket, buffer, 256); 
            
            if (strcmp(buffer, "quit") == 0)
            {
                close(new_socket);
                break;
            }

            printf("%s\n", buffer);

            size = (int)(buffer[0]);

            for (i = 0; i < size; ++i)
            {
                message[i] = buffer[i + 1];
            }
            
            args = sh_split_line(message);

            if (strcmp(args[0], "Prices") == 0)
            {
                if (strcmp(args[1], "AAPL") == 0)
                {
                    for (i = 0; i < csvSize; ++i)
                    {
                        if (strcmp(args[2], appleInfo[i].date) == 0)
                        {
                            send(new_socket, appleInfo[i].price, strlen(appleInfo[i].price), 0);
                            break;
                        }

                        else if (appleInfo[i].date[0] == '\0')
                        {
                            char* failure = "Unknown";

                            send(new_socket, failure, strlen(failure), 0);
                            break;
                        }
                    }
                }

                else if (strcmp(args[1], "TWTR") == 0)
                {
                    for (i = 0; i < csvSize; ++i)
                    {
                        if (strcmp(args[2], twitterInfo[i].date) == 0)
                        {
                            send(new_socket, twitterInfo[i].price, strlen(twitterInfo[i].price), 0);
                            break;
                        }

                        else if (twitterInfo[i].date[0] == '\0')
                        {
                            char* failure = "Unknown";

                            send(new_socket, failure, strlen(failure), 0);
                            break;
                        }
                    }
                }
            }

            else if (strcmp(args[0], "MaxProfit") == 0)
            {
                if (strcmp(args[1], "AAPL") == 0)
                {
                    char appleMaxProfit[100] = {0};

                    sprintf(appleMaxProfit, "Maximum Profit for AAPL: %.2f\n", appleProfits);
                    send(new_socket, appleMaxProfit, strlen(appleMaxProfit), 0);
                }

                else if (strcmp(args[1], "TWTR") == 0)
                {
                    char twitterMaxProfit[100] = {0};

                    sprintf(twitterMaxProfit, "Maximum Profit for TWTR: %.2f\n", twitterProfits);
                    send(new_socket, twitterMaxProfit, strlen(twitterMaxProfit), 0);
                }
            }
        }
    }
    return 0;
} 
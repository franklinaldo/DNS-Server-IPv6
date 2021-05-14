#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>


#define DATA_SIZE 2
#define DATA_LENGTH 2
#define RESPONSE 2
#define FIRST_NAME_INDEX 12
#define DOT 1


/* ******************** Functions ******************** */

int convertHexToDec(unsigned char* byte){
    return (int)byte[0]*256 + (int)byte[1];
}

// read data
void read_raw_data(char* filename, unsigned char** message) {
    int fdes;
    unsigned char buffer[DATA_LENGTH];

    /* open the file for unformatted input */
    fdes = open(filename,O_RDONLY);
    if( fdes==-1 ) {
        fprintf(stderr,"Unable to open %s\n", filename);
    }

    read(fdes, buffer, DATA_LENGTH);

    int message_size = convertHexToDec(buffer);
    // printf("Data size: %d\n", convertHexToDec(buffer));
    printf("Data size: %d\n", message_size);

    *message =  malloc(message_size);
    read( fdes, *message, message_size);

    printf("\n");
    
    /* close the file */
    close(fdes);
}


// Finding domain name
void find_domain_name(unsigned char** message, char** domain_name, int* index) {
    
    int is_index_name_length = 1;
    int next_index_name_length;
    int total_length = 0;
    int q_name_index = 0;

    unsigned char zero_hex =0x0;

    while ((*message)[*index] != zero_hex) {
            // printf("MESSAGE[%d] = %x\n", *index, (*message)[*index]);
            // Current index arrives at the next label length

            // Flag the current index as the length of the next sub name
            if (next_index_name_length == *index) {
                is_index_name_length = 1;

                // Adding dot to the separated name
                if (*index!=FIRST_NAME_INDEX) {
                    total_length+= DOT;
                    (*domain_name)[q_name_index] = '.';
                    q_name_index++;
                }
            }

            // current index is telling about the length of label
            if (is_index_name_length) {
                int label_length = (int)(*message)[*index];
                next_index_name_length = (*index)+ label_length + 1;
                total_length += label_length;
                // question_name = (char *) realloc(question_name, total_length);
                is_index_name_length = 0;
                (*index)++;

            } else { // append character in current index
                (*domain_name)[q_name_index] = (char)(*message)[*index];
                q_name_index++;
                (*index)++;
            }
    }
}

char* timestamp(char** ts) {
    time_t rawtime;
    struct tm *info;

    time( &rawtime );

    info = localtime( &rawtime );

    strftime(*ts,80,"%FT%T%z", info);
    
    return *ts;
}

void stringify_ip_address(char *cleaned_ip_string, unsigned char* message, int* index) {

    unsigned char buf[sizeof(struct in6_addr)];
    char *ip_addr_string = malloc(INET6_ADDRSTRLEN);
    int s;
    char temp[4];
    // cleaned_ip_string[0]
    // unsigned char buf[sizeof(struct in6_addr)];
    // char *ip_addr_string = malloc(INET6_ADDRSTRLEN);
    // int s;
    // printf("MASOK\n");
    // Getting the ipd address hex
    
    for (int i=0; i<16; i++) {
        // printf("message[%d]: %.2x\n", *index+17+i, message[*index+17+i]);
        sprintf(temp, "%.2x", message[*index+17+i]);
        // printf("MASOK x%d\n", i);
        strcat(ip_addr_string, temp);
        // ip_address[ip_index] = temp;
        // ip_index++;
        if ((i%2) && (i!=15)) {
            strcat(ip_addr_string, ":");
            // printf("MASOK sini juga x%d\n", i);
        }
        // printf("ip_addr_string: %s\n", ip_addr_string);
    }

    s = inet_pton(AF_INET6, ip_addr_string, buf);
    
    if (s <= 0) {
        if (s == 0)
            fprintf(stderr, "Not in presentation format");
        else
            perror("inet_pton");
        exit(EXIT_FAILURE);
    }
    inet_ntop(AF_INET6, buf, cleaned_ip_string, INET6_ADDRSTRLEN);

    // printf("%s\n", cleaned_ip_string);

}
int main(int argc, char* argv[]) {

    /* ***************************** Logging ************************************** */
    // from https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm

    // Preparation for logging
    // char* request_log_string = malloc(80);
    FILE *f;
    f = fopen("dns_svr.log", "w"); // a+ (create + append) option will allow appending which is useful in a log file
    if (f == NULL) { /* Something is wrong   */}
    // Attributes for server
    int sockfd, newsockfd, n, re, s;
	// char buffer[256];
	struct addrinfo hints, *res;
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;

	// if (argc < 2) {
	// 	fprintf(stderr, "ERROR, no port provided\n");
	// 	exit(EXIT_FAILURE);
	// }

	// Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, "8053", &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(sockfd, 5) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}


	// Read characters from the connection, then process
	while (1) {

        // Accept a connection - blocks until a connection is ready to be accepted
        // Get back a new file descriptor to communicate on
        client_addr_size = sizeof client_addr;
        newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (newsockfd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        unsigned char buffer[DATA_SIZE];
        unsigned char* dns_message;
        unsigned char* whole_dns_message;

        // Finding the size of dns message
        // memset(buffer, 0, DATA_SIZE);
        n = read(newsockfd, buffer, DATA_SIZE); // n is number of characters read
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(EXIT_FAILURE);
		}

        int message_size = convertHexToDec(buffer);
        printf("Data size: %d\n", message_size);

        
        // Import message into dns_message
        dns_message = malloc(message_size*sizeof(unsigned char));
        memset(dns_message, 0, message_size);
        n = read(newsockfd, dns_message, message_size);
        
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        printf("mes size: %d \n", (int)sizeof(dns_message));
        
        unsigned char response = dns_message[2]>>7;
        printf("Response: %d\n", response);
        
        printf("\n");

        // unsigned char* whole_dns_message = malloc(sizeof(buffer)+sizeof(dns_message));
        whole_dns_message = malloc(DATA_SIZE+message_size);


        printf("REQUEST\n");
        // for (int i =0; i<52; i++) {
        //     printf("[%d]: %x ",i, dns_message[i]);
        // }
        printf("\n\n");

        // creating a variable that contains the whole dns message
        // Inserting the data length hex
        for (int i =0; i<(DATA_SIZE); i++) {
            whole_dns_message[i] = buffer[i];
        }
        // Inserting the dns message hex
        for (int i =0; i<(message_size); i++) {
            whole_dns_message[i+DATA_SIZE] = dns_message[i];
        }
        // Printing the whole dns message
        for (int i =0; i<(DATA_SIZE+message_size); i++) {
            printf("[%d]: %x ",i, whole_dns_message[i]);
        }

        printf("\nwhole dns size: %lu\n",sizeof(whole_dns_message));

        int index = FIRST_NAME_INDEX; //12
        char* domain_name = malloc(100);
    
        find_domain_name(&dns_message, &domain_name, &index);
        printf("Domain name: %s\n", domain_name);
        printf("index: %d\n", index);

		// Disconnect
		if (n == 0) {
			break;
		}

        /******************* Connect with the upstream server *********************/
        int sockfd_client, n_client, s_client;
        struct addrinfo hints_client, *servinfo_client, *rp_client;
        unsigned char* buffer_client;

        if (argc < 3) {
            printf("usage %s hostname port\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        // Create address
        memset(&hints_client, 0, sizeof hints_client);
        hints_client.ai_family = AF_INET;
        hints_client.ai_socktype = SOCK_STREAM;

        // Get addrinfo of server
        s_client = getaddrinfo(argv[1], argv[2], &hints_client, &servinfo_client);
        if (s_client != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s_client));
            exit(EXIT_FAILURE);
        }
        printf("arg1, arg2: %s %s\n",argv[1], argv[2]);
        // Connect to first valid result
        // Why are there multiple results? see man page (search 'several reasons')
        // How to search? enter /, then text to search for, press n/N to navigate
        for (rp_client = servinfo_client; rp_client != NULL; rp_client = rp_client->ai_next) {
            sockfd_client = socket(rp_client->ai_family, rp_client->ai_socktype, rp_client->ai_protocol);
            if (sockfd_client == -1)
                continue;

            if (connect(sockfd_client, rp_client->ai_addr, rp_client->ai_addrlen) != -1)
                break; // success

            close(sockfd_client);
        }
        if (rp_client == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            exit(EXIT_FAILURE);
        }

        // Do processing
        printf("Passing client's req to upstream server\n");
        for (int i =0; i<(DATA_SIZE+message_size); i++) {
            printf("[%d]: %x ",i, whole_dns_message[i]);
        }
        // Send message to upstream server
        n_client = write(sockfd_client, whole_dns_message, DATA_SIZE+message_size);
        if (n_client < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        printf("masi aman\n");

        unsigned char buffer_data_size[DATA_SIZE];
        unsigned char* dns_message_res;
        unsigned char* whole_dns_message_res;

        // Finding the size of dns message
        n_client = recv(sockfd_client,buffer_data_size,DATA_SIZE,0);
        printf("bufff disessesese: %02x %02x\n", buffer_data_size[0],buffer_data_size[1]);

        // n_client = rec(sockfd_client, buffer_data_size, DATA_SIZE); // n is number of characters read
		if (n_client < 0) {
			perror("ERROR reading from socket");
			exit(EXIT_FAILURE);
		}

        printf("NNNNNN: %d\n", n);
        printf("bufff disessesese: %x\n", buffer_data_size[1]);

        int message_size_res = convertHexToDec(buffer_data_size);
        printf("Data size RES: %d\n", message_size_res);

        // Import message into dns_message
        dns_message_res = malloc(message_size_res);
        n_client = read(sockfd_client, dns_message_res, message_size_res);
        if (n_client < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        printf("mes size: %d \n", (int)sizeof(dns_message_res));
        
        response = dns_message_res[2]>>7;
        printf("Response: %d\n", response);
        
        printf("\n");

        // unsigned char* whole_dns_message = malloc(sizeof(buffer)+sizeof(dns_message));
        whole_dns_message_res = malloc(DATA_SIZE+message_size_res);


        printf("RESPONSE\n");

        // creating a variable that contains the whole dns message // DUPLICATE
        for (int i =0; i<(DATA_SIZE); i++) {
            whole_dns_message_res[i] = buffer_data_size[i];
            // printf("[%d]: %x ",i, whole_dns_message[i]);
        }
        for (int i =0; i<(message_size_res); i++) {
            whole_dns_message_res[i+DATA_SIZE] = dns_message_res[i];
            // printf("[%d]: %x ",i, whole_dns_message[i]);
        }
        for (int i =0; i<(DATA_SIZE+message_size_res); i++) {
            printf("[%d]: %x ",i, whole_dns_message_res[i]);
        }

        close(sockfd_client);
        freeaddrinfo(servinfo_client);

    /****************************** end of establishing connection with upstream server ************************************/

        n = write(newsockfd, whole_dns_message_res, DATA_SIZE+message_size_res);
        if (n < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
	}

    fflush(stdout);
	freeaddrinfo(res);
	close(sockfd);
	close(newsockfd);
    fclose(f);

    return(0);
}

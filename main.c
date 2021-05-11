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


#define DATA_LENGTH 2
#define RESPONSE 2
#define FIRST_NAME_INDEX 12
#define DOT 1


/* ******************** Functions ******************** */

int convertHexToDec(char* byte){
    return (int)byte[0]*256 + (int)byte[1];
}

// DNS message size
int dns_message_size(char* size_in_hex) {
    return convertHexToDec(size_in_hex);
}

// read data
void read_raw_data(char* filename, unsigned char** message) {
    int fdes;
    char buffer[DATA_LENGTH];

    /* open the file for unformatted input */
    fdes = open(filename,O_RDONLY);
    if( fdes==-1 ) {
        fprintf(stderr,"Unable to open %s\n", filename);
    }

    read(fdes, buffer, DATA_LENGTH);
    
    int message_size = dns_message_size(buffer);
    // printf("Data size: %d\n", convertHexToDec(buffer));
    printf("Data size: %d\n", message_size);

    *message =  malloc(message_size);
    read( fdes, *message, message_size);
    
    // Printing the data
    // for( int x=0; x<message_size; x++) {
    //     putchar(*message[x]);
    // };

    // printf("Data size2: %d\n", read_mes);

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
    int sockfd, newsockfd, n, re, i, s;
	char buffer[256];
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

	// Accept a connection - blocks until a connection is ready to be accepted
	// Get back a new file descriptor to communicate on
	client_addr_size = sizeof client_addr;
	newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
	if (newsockfd < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

    /******************* Connect with the upstream server *********************/
    int sockfd_client, n_client, s_client;
	struct addrinfo hints_client, *servinfo_client, *rp_client;
	char buffer_client[256];

    if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Create address
	memset(&hints_client, 0, sizeof hints_client);
	hints_client.ai_family = AF_INET;
	hints_client.ai_socktype = SOCK_STREAM;

	// Get addrinfo of server. From man page:
	// The getaddrinfo() function combines the functionality provided by the
	// gethostbyname(3) and getservbyname(3) functions into a single interface
	s_client = getaddrinfo(argv[1], argv[2], &hints_client, &servinfo_client);
	if (s_client != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s_client));
		exit(EXIT_FAILURE);
	}

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

    /****************************** end of establishing connection with upstream server ************************************/


	// Read characters from the connection, then process
	while (1) {
		memset(buffer, 0, 256);
		n = read(newsockfd, buffer, 255); // n is number of characters read
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(EXIT_FAILURE);
		}

		// Disconnect
		if (n == 0) {
			break;
		}

		// Write message back (Note: memset is setting the trailing \0)
		// A rather ugly solution for the buffer
		memmove(buffer + 30, buffer, n);
		memmove(buffer, "Here is the message in upper: ", 30);
		printf("%s\n", buffer);
		n = write(newsockfd, buffer, n + 30);
		if (n < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	freeaddrinfo(res);
	close(sockfd);
	close(newsockfd);





    /* ***************************** Phase 1 ************************************** */


    // // reading data
    // unsigned char* message;
    // read_raw_data(argv[2], &message);
    // // Shifting to the first bit of the response
    // unsigned char response = message[2]>>7;
    // printf("Response: %d\n", response);
    
    // int index = FIRST_NAME_INDEX; //12
    
    // // Domain name
    // char* domain_name = malloc(100);
    // find_domain_name(&message, &domain_name, &index);
    // printf("Domain name: %s\n", domain_name);
    // printf("index: %d\n", index);

    // /* ***************************** Question Class ************************************** */
    // // char* question_class = malloc(sizeof(char)*4);
    // // question_class = message[index];

    // int is_IPv6 = 0;
    // if (message[index+1]==0x0 && message[index+2]==0x1c) {
    //     is_IPv6 = 1;
    //     printf("DNS type: IPv6\n");
    // }

    // /* ***************************** logging request ************************************** */
    // if (!response){
    //     char* time_buffer = timestamp(&request_log_string);
    //     fprintf(f, "%s %s %s\n", time_buffer, "requested", domain_name);
    //     printf("%s %s %s\n", time_buffer, "requested", domain_name);
    // }

    // // printf("From outside msg: %x\n", message[41]);
    // /* ***************************** response ************************************** */

    // if (response) {
    //     char* cleaned_ip_string = malloc(INET6_ADDRSTRLEN);
    //     char* time_buffer = timestamp(&request_log_string);
    //     stringify_ip_address(cleaned_ip_string, message, &index);
    //     printf("IP Address: %s\n", cleaned_ip_string);
    //     // 2021-04-24T05:12:32+0000 1.comp30023 is at 2001:388:6074::7547:1
    //     fprintf(f, "%s %s %s %s\n", time_buffer, domain_name, "is at", cleaned_ip_string);
    //     printf("%s %s %s %s\n", time_buffer, domain_name, "is at", cleaned_ip_string);
    // }
    // /* *****************************  ************************************** */


    /* ***************************** end of phase 1  ************************************** */
    fclose(f);

    return(0);
}

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
#include <assert.h>

#include "parse_data.h"
#include "logging.h"

#define DATA_SIZE 2
#define DATA_LENGTH 2
#define RESPONSE 2
#define FIRST_NAME_INDEX 12
#define DOT 1

int main(int argc, char* argv[]) {

    /* ***************************** Logging ************************************** */
    

    // Preparation for logging
    // Attributes for server
    int sockfd, newsockfd, n, re, s;
	// char buffer[256];
	struct addrinfo hints, *res;
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;

	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(EXIT_FAILURE);
	}

	// Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept

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

    create_log_file();

	// Read characters from the connection, then process
	while (1) {
        printf("\n---------------------BEGINING OF WHILE LOOP---------------------\n");
        // Accept a connection - blocks until a connection is ready to be accepted
        // Get back a new file descriptor to communicate on
        client_addr_size = sizeof client_addr;
        newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (newsockfd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        
        // Finding the size of dns message
        unsigned char dns_message_size_hex[DATA_SIZE];
        n = read(newsockfd, dns_message_size_hex, DATA_SIZE); // n is number of characters read
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(EXIT_FAILURE);
		}

        int message_size = convertHexToDec(dns_message_size_hex);
        printf("Data size: %d\n", message_size);

        // Import message into dns_message
        int n2;
        printf("strlen dns msg: %d\n", message_size);
        // unsigned char* dns_message = malloc(sizeof(unsigned char)*message_size);
        unsigned char dns_message[sizeof(unsigned char)*message_size];

        int receive_bytes =0;
        // Getting dns message one by one to avoid failure on delay
        while (receive_bytes != message_size){
            unsigned char temp[1];
            n2 = read(newsockfd, temp, 1);
            if (n < 0) {
                perror("ERROR reading from socket");
                exit(EXIT_FAILURE);
            }
            dns_message[receive_bytes] = temp[0];
            receive_bytes += n2;
            // printf("mes size: %d \n", (int)sizeof(dns_message));
        }
        
        printf("\n");

        // Finding domain name for logging
        int index = FIRST_NAME_INDEX; //12
        
        char* domain_name = find_domain_name(dns_message, &index);
        // printf("FOUND: %s\n", found);
        // // printf("last thingin dom name: %c, [%lu]\n", domain_name[strlen(domain_name)-1], strlen(domain_name));
            
        // printf("\nDomain name: %s, %lu\n", found, strlen(found));
        // for (int i =0; i< (strlen(found)); i++) {
        //     printf("[%d]: %c ",i, found[i]);
        // }
        // printf("\n");
        
        // printf("\nDomain name: %s, %lu\n", domain_name, strlen(domain_name));
        // for (int i =0; i< strlen(domain_name); i++) {
        //     printf("[%d]: %c ",i, domain_name[i]);
        // }
        printf("\n");

        printf("index: %d\n", index);

        // unsigned char* whole_dns_message = malloc(sizeof(buffer)+sizeof(dns_message));

        printf("REQUEST\n");

        // creating a variable that contains the whole dns message
        // Inserting the data length hex
        unsigned char* whole_dns_message = malloc(sizeof(unsigned char)*(n+message_size));
        merge_sub_dns_messages(whole_dns_message, dns_message_size_hex, &n, dns_message, &message_size);
        
        printf("\n--------------------------\n");

        for (int i =0; i<(DATA_SIZE+message_size); i++) {
            // printf("[%d]: %c ",i, whole_dns_message[i]);
        }

        write_log_request(domain_name);

        // Check whether using IPv6 or other connections
        int is_IPv6 = 0;
    
        if (dns_message[index+1]==0x0 && dns_message[index+2]==0x1c) {
            is_IPv6 = 1;
        }
        
        // Handling Not AAAA request
        if (!is_IPv6) {
            write_log_unimplemented_request();
            
            // Modifying the request for responding the client
            whole_dns_message[4] = 0x80;
            whole_dns_message[5] = 4;
            n = write(newsockfd, whole_dns_message, DATA_SIZE+message_size);
            if (n < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }

        } else {
            printf("DNS type: IPv6\n");
            /******************* Connect with the upstream server *********************/
            int sockfd_client, n_client, s_client;
            struct addrinfo hints_client, *servinfo_client, *rp_client;

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
            for (rp_client = servinfo_client; rp_client != NULL; rp_client = rp_client->ai_next) {
                sockfd_client = socket(rp_client->ai_family, rp_client->ai_socktype, rp_client->ai_protocol);
                if (sockfd_client == -1)
                    continue;

                if (connect(sockfd_client, rp_client->ai_addr, rp_client->ai_addrlen) != -1)
                    break; // success

                close(sockfd_client);
            }
            if (rp_client == NULL) {
                fprintf(stderr, "client: failed to connect to upstream server\n");
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

            // unsigned char* whole_dns_message_res;

/**********************************************************************************************/
/****************************************** RESPONSE ******************************************/
/**********************************************************************************************/

            // Finding the size of dns message
            unsigned char buffer_data_size[DATA_SIZE];
            n_client = read(sockfd_client,buffer_data_size,DATA_SIZE);
            // printf("bufff disessesese: %02x %02x\n", buffer_data_size[0],buffer_data_size[1]);

            // n_client = rec(sockfd_client, buffer_data_size, DATA_SIZE); // n is number of characters read
            if (n_client < 0) {
                perror("ERROR reading from socket");
                exit(EXIT_FAILURE);
            }

            int message_size_res = convertHexToDec(buffer_data_size);
            printf("\nData size RES: %d\n", message_size_res);

            // Import message into dns_message
            unsigned char* dns_message_res = malloc(sizeof(unsigned char)*message_size_res);
            n_client = read(sockfd_client, dns_message_res, message_size_res);
            if (n_client < 0) {
                perror("ERROR reading from socket");
                exit(EXIT_FAILURE);
            }

            for (int i=0;i<message_size; i++) {
                // printf("[%d]: %.2x ", i, dns_message_res[i]);
            }
            printf("\n");


///////////////////////
            int res_index = FIRST_NAME_INDEX; //12
            char* res_domain_name = find_domain_name(dns_message_res, &res_index);
            // char* res_domain_name = malloc(1*sizeof(char));

            // find_domain_name(dns_message_res, res_domain_name, &res_index);
            // res_domain_name[strlen(res_domain_name)] = '\0';
            // printf("last thingin dom name: %c, [%lu]\n", res_domain_name[strlen(res_domain_name)-1], strlen(res_domain_name));
            
            printf("\nDomain name: %s, %lu\n", res_domain_name, strlen(res_domain_name));
            for (int i =0; i< (strlen(res_domain_name)); i++) {
                printf("[%d]: %c ",i, res_domain_name[i]);
            }
            printf("\n");

            printf("res index: %d\n", res_index);

///////////////////////
            printf("total message size: %d\n", n_client);
            // unsigned char* whole_dns_message = malloc(sizeof(buffer)+sizeof(dns_message));

            printf("\n\nRESPONSE\n");

            unsigned char whole_dns_message_res[sizeof(unsigned char)*(DATA_LENGTH+message_size_res)];
            printf("whol dns size: %lu \n", sizeof(whole_dns_message_res));

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

            int is_IPv6 = 0;
    
            if (dns_message_res[index+1+6]==0x0 && dns_message_res[index+2+6]==0x1c) {
                // printf("\nRES connection hex: %x %x\n",dns_message_res[index+1+6], dns_message_res[index+2+6]);
                is_IPv6 = 1;
            }
            
            printf("\nRES connection hex: %x %x\n idx: %d, %d\n",dns_message_res[index+1], dns_message_res[index+2], index+1, index+2);
            printf("\nRES connection hex: %x %x\n idx: %d, %d\n",dns_message_res[index+1+6], dns_message_res[index+2+6], index+1+6, index+2+6);
            
            // Only log the AAAA response
            if (is_IPv6) {
                char* cleaned_ip_string = malloc(INET6_ADDRSTRLEN);
                stringify_ip_address(cleaned_ip_string, dns_message_res, &res_index);
                printf("IP Address: %s\n", cleaned_ip_string);
                write_log_response(res_domain_name, cleaned_ip_string);
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
        // free(domain_name);
	}

    fflush(stdout);
	freeaddrinfo(res);
	close(sockfd);
	close(newsockfd);

    return(0);
}

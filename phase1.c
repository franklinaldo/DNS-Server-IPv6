#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


#define DATA_LENGTH 2
#define RESPONSE 2
#define FIRST_NAME_INDEX 12
#define DOT 1



long long convertIntToBin(int n) {
    long long bin = 0;
    int rem, i = 1, step = 1;
    while (n != 0) {
        rem = n % 2;
        printf("Step %d: %d/2, Remainder = %d, Quotient = %d\n", step++, n, rem, n / 2);
        n /= 2;
        bin += rem * i;
        i *= 10;
    }
    return bin;
}

int convertHexToDec(char* byte){
    return (int)byte[0]*256 + (int)byte[1];
}

int convertUHexToDec(unsigned char* byte){
    return (int)byte[0]*256 + (int)byte[1];
}

// Functions
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
    size_t read_mes = read( fdes, *message, message_size);
    
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

int main(int argc, char* argv[]) {

    // Shifting to the first bit of the response
    unsigned char* message;
    
    read_raw_data(argv[1], &message);

    unsigned char response = message[2]>>7;
    printf("Response: %d\n", response);


    int index = FIRST_NAME_INDEX; //12
    char* domain_name = malloc(100);
    
    find_domain_name(&message, &domain_name, &index);

    printf("Domain name: %s\n", domain_name);
    printf("index: %d\n", index);

    /* ***************************** Question Class ************************************** */
    // char* question_class = malloc(sizeof(char)*4);
    // question_class = message[index];

    int is_IPv6 = 0;
    
    if (message[index+1]==0x0 && message[index+2]==0x1c) {
        is_IPv6 = 1;
        printf("DNS type: IPv6\n");
    }

    /* ***************************** Log Request ************************************** */
    // from https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
    
    char* request_log_string = malloc(80);
    char* time_buffer = timestamp(&request_log_string);

    FILE *f;
    f = fopen("dns_svr.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
    if (f == NULL) { /* Something is wrong   */}

    if (!response){
    fprintf(f, "%s %s %s\n", time_buffer, "requested", domain_name);
    printf("%s %s %s\n", time_buffer, "requested", domain_name);
    }

    /* ***************************** Response ************************************** */
    unsigned char* ip_address = malloc(INET6_ADDRSTRLEN);
    unsigned char buf[sizeof(struct in6_addr)];
    char *ip_addr_string = malloc(INET6_ADDRSTRLEN);
    int ip_index = 0;
    int domain, s;
    char str[INET6_ADDRSTRLEN];
    char temp[4];

    for (int i=0; i<16; i++) {
        ip_address[i] = message[index+17+i];
        printf("ipadd: %x\n", ip_address[i]);
    }
    
    for (int i=0; i<16; i++) {
        ip_address[i] = message[index+17+i];
        sprintf(temp, "%.2x", message[index+17+i]);
        strcat(ip_addr_string, temp);
        // ip_address[ip_index] = temp;
        // ip_index++;
        if ((i%2) && (i!=15)) {
            strcat(ip_addr_string, ":");
        }
    }

    printf("ip str: %s\n", ip_addr_string);
    s = inet_pton(AF_INET6, ip_addr_string, buf);
    // s = inet_pton(domain, ip_addr_string, buf);
           if (s <= 0) {
               if (s == 0)
                   fprintf(stderr, "Not in presentation format");
               else
                   perror("inet_pton");
               exit(EXIT_FAILURE);
           }
           inet_ntop(AF_INET6, buf, str, INET6_ADDRSTRLEN);

           printf("%s\n", str);

    /**************************************************************************************/

    // unsigned char buf[sizeof(struct in6_addr)];
    // int domain, s;
    // char str[INET6_ADDRSTRLEN];

    // if (argc != 3) {
    //     fprintf(stderr, "Usage: %s {i4|i6|<num>} string\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // // domain = (strcmp(argv[1], "i4") == 0) ? AF_INET :
    // //         (strcmp(argv[1], "i6") == 0) ? AF_INET6 : atoi(argv[1]);
    // // domain = atoi()
    // s = inet_pton(domain, argv[2], buf);
    // if (s <= 0) {
    //     if (s == 0)
    //         fprintf(stderr, "Not in presentation format");
    //     else
    //         perror("inet_pton");
    //     exit(EXIT_FAILURE);
    // }

    // if (inet_ntop(domain, buf, str, INET6_ADDRSTRLEN) == NULL) {
    //     perror("inet_ntop");
    //     exit(EXIT_FAILURE);
    // }

    // printf("%s\n", str);

    // exit(EXIT_SUCCESS);

/**************************************************************************************/

    // if (response) {

        
    //     // Jump to IPv6 Address, +17 from the latest index
    //     // for (int i=0; i<16; i++) {
    //     //     // message[index+17]
    //     //     // char hex_ip[2] = message[index+17+i];
    //     //     // printf("response[%d+17+%d]: %x\n", index, i, convertHexToDec((char*)message[index+17+i]));
    //     //     printf("belom seg faultttt\n");

    //     //     // switch (i%2)
    //     //     // {
    //     //     // case 0:
    //     //     //                         // printf("inner INDEX %d\n", convertUHexToDec(message[index+17+i]));
    //     //     //     if (message[index+17+i]!=0x0) {
    //     //     //         printf("Case 0\n");
    //     //     //         // printf("belom seg faultttt\n");
    //     //     //                         // printf("inner INDEX %x\n", message[index+17+i]);
    //     //     //         printf("response[%d+17+%d]: %x\n", index, i, message[index+17+i]);
    //     //     //         fprintf(f,"%x", message[index+17+i]);
    //     //     //         sprintf(temp, "%x", message[index+17+i]);
    //     //     //         ip_address[ip_index] = temp;
    //     //     //         ip_index++;


    //     //     //         // fprintf('%x', message[index+17+i]);
    //     //     //         // printf("belom seg faultttt\n");
    //     //     //     }
    //     //     //     break;
    //     //     // case 1:
    //     //     //     if (message[index+17+i]!=0x0) {
    //     //     //         printf("Case 1\n");
    //     //     //         if (message[index+17+i-1] != 0x0) {
    //     //     //             fprintf(f,"%.2x", message[index+17+i]);
    //     //     //             printf("response[%d+17+%d]: %.2x\n", index, i, message[index+17+i]);
    //     //     //             sprintf(temp, "%x", message[index+17+i]);
    //     //     //             printf("temp: %s\n", temp);
    //     //     //             ip_address[ip_index] = temp;
    //     //     //             ip_index++;
    //     //     //         } else {
    //     //     //             fprintf(f,"%x", message[index+17+i]);
    //     //     //             printf("response[%d+17+%d]: %x\n", index, i, message[index+17+i]);
    //     //     //             sprintf(temp, "%x", message[index+17+i]);
    //     //     //             ip_address[ip_index] = temp;
    //     //     //             ip_index++;
    //     //     //         }
    //     //     //         // fprintf(f,"%x", message[index+17+i]);
    //     //     //         // printf("response[%d+17+%d]: %x\n", index, i, message[index+17+i]);

    //     //     //         // fprintf('%x', message[index+17+i]);
                
    //     //     //     if (i!=15) {
    //     //     //         fprintf(f,":");
    //     //     //         ip_address[ip_index] = ":";
    //     //     //         ip_index++;
    //     //     //     }
    //     //     //     }
    //     //     //     break;
    //     //     // default:
    //     //     //     break;
    //     //     // }
    //     //     // fprintf(f, "%s %s %s\n", time_buffer, "requested", domain_name);

    //     // }
    //     // printf("IP_ADD: %s\n", ip_address);


    //     // printf("message[%d+17]: %x", index, message[index+17]);
    //     // <timestamp> <domain_name> is at <IP address>
    // }

    

    return(0);

}
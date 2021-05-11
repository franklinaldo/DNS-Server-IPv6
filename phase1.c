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

    // Shifting to the first bit of the response
    unsigned char* message;
    
    read_raw_data(argv[2], &message);

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

    /* ***************************** Logging ************************************** */
    // from https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
    
    char* request_log_string = malloc(80);

    FILE *f;
    f = fopen("dns_svr.log", "w"); // a+ (create + append) option will allow appending which is useful in a log file
    if (f == NULL) { /* Something is wrong   */}

    /* ***************************** request ************************************** */
    if (!response){
        char* time_buffer = timestamp(&request_log_string);
        fprintf(f, "%s %s %s\n", time_buffer, "requested", domain_name);
        printf("%s %s %s\n", time_buffer, "requested", domain_name);
    }

    // printf("From outside msg: %x\n", message[41]);
    /* ***************************** response ************************************** */

    if (response) {
        char* cleaned_ip_string = malloc(INET6_ADDRSTRLEN);
        char* time_buffer = timestamp(&request_log_string);
        stringify_ip_address(cleaned_ip_string, message, &index);
        printf("IP Address: %s\n", cleaned_ip_string);
        // 2021-04-24T05:12:32+0000 1.comp30023 is at 2001:388:6074::7547:1
        fprintf(f, "%s %s %s %s\n", time_buffer, domain_name, "is at", cleaned_ip_string);
        printf("%s %s %s %s\n", time_buffer, domain_name, "is at", cleaned_ip_string);
    }
    /* *****************************  ************************************** */


    return(0);
}

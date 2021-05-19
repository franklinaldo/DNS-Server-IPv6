#define DATA_SIZE 2
#define DATA_SIZE 2
#define DATA_LENGTH 2
#define RESPONSE 2
#define FIRST_NAME_INDEX 12
#define DOT 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>


int convertHexToDec(unsigned char* byte){
    return (int)byte[0]*256 + (int)byte[1];
}

void stringify_ip_address(char *cleaned_ip_string, unsigned char* message, int* index) {

    unsigned char buf[sizeof(struct in6_addr)];
    // char *ip_addr_string = malloc(INET6_ADDRSTRLEN);
    char ip_addr_string[INET6_ADDRSTRLEN] = "";
    // ip_addr_string = malloc(46);
    // assert(ip_addr_string);
    // printf("before ipaddstr: %s\n", ip_addr_string);
    int s;
    char temp[4];
    
    for (int i=0; i<16; i++) {
        // printf("message[%d]: %.2x\n", *index+17+i, message[*index+17+i]);
        sprintf(temp, "%.2x", message[*index+17+i]);
        // printf("MASOK x%d\n", i);
        strcat(ip_addr_string, temp);
        // printf("temp: %s\n",temp);
        // ip_address[ip_index] = temp;
        // ip_index++;
        if ((i%2) && (i!=15)) {
            strcat(ip_addr_string, ":");
            // printf("MASOK sini juga x%d\n", i);
        }
        // printf("ip_addr_string: %s\n\n", ip_addr_string);
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

    printf("%s\n", cleaned_ip_string);

}

void merge_sub_dns_messages(unsigned char* buffer, unsigned char* first_sub_message, int* first_sub_message_size, unsigned char* second_sub_message, int* second_sub_message_size){

    for (int i =0; i<(*first_sub_message_size); i++) {
        buffer[i] = first_sub_message[i];
    }
    // Inserting the dns message hex
    for (int i =0; i<(*second_sub_message_size); i++) {
        buffer[i+(*first_sub_message_size)] = second_sub_message[i];
    }

}

// Finding domain name
char* find_domain_name(unsigned char* message, int* index) {
    
    int is_index_name_length = 1;
    int next_index_name_length = 0;
    int total_length = 0;
    int q_name_index = 0;

    char* domain_name = malloc(sizeof(char));
    unsigned char zero_hex =0x0;

    // While the bytes doesnt contain the 0x00 (stop sign for domain name)
    while ((message)[*index] != zero_hex) {
        if (next_index_name_length == *index) {
            is_index_name_length = 1;

            // Adding dot to the separated sub name
            if (*index!=FIRST_NAME_INDEX) {
                total_length+= DOT;
                (domain_name)[q_name_index] = '.';
                q_name_index++;
            }
        }

        // current index is telling about the length of label
        if (is_index_name_length) {
            int label_length = (int)(message)[*index];
            // printf("label len : %d\n", label_length);
            next_index_name_length = (*index)+ label_length + DOT;
            total_length += label_length;
            // printf("total len : %d\n", total_length);
            printf("Realloc size: %lu\n", (sizeof(char))*total_length);
            domain_name = realloc(domain_name, (sizeof(char))*total_length+2);
            is_index_name_length = 0;
            (*index)++;

        } else { // append character in current index
            (domain_name)[q_name_index] = (char)(message)[*index];
            q_name_index++;
            (*index)++;
        }

        // Adding termination
        if (strlen(domain_name)!=total_length) {
            domain_name[total_length] = '\0';
        }

    }
    printf("strlen dom name : %lu\n", strlen(domain_name));
    printf("dom name print : %s\n", domain_name);
    return domain_name;
}

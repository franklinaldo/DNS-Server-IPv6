#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


#define SIZE 2
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

typedef unsigned char Byte;
typedef struct request Request;

struct request{
    int byteLength;
};

int convertHexToDec(char* byte){
    return (int)byte[0]*256 + (int)byte[1];
}


int main(int argc, char* argv[]) {

    /* open the file for unformatted input */
    int fdes,x;
    char buffer[SIZE];
    size_t read_data_size;
    size_t read_message;

    /* open the file for unformatted input */
    fdes = open(argv[1],O_RDONLY);
    if( fdes==-1 ) {
        fprintf(stderr,"Unable to open %s\n",argv[1]);
        return(1);
    }

    /* read raw data */
    read_data_size = read( fdes, buffer, SIZE );

    // Printing the data
    for( int x=0; x<read_data_size; x++ ) {
        putchar( buffer[x] );
    };

    printf("\n");

    printf("Data size: %d\n", convertHexToDec(buffer));

    unsigned char message[convertHexToDec(buffer)];
    read_message = read( fdes, message, convertHexToDec(buffer) );
    
    /* close the file */
    close(fdes);

    /* output the buffer */
    /* not null character terminated! */

    // for( int x=0; x<read_message; x++ ) {
    //     putchar( message[x] );
    // };
    // printf("\n");

    unsigned char response = message[2]>>7;
    printf("Response: %d\n", response);

//    char *str;

    // str = (char *) malloc(15);
    // strcpy(str, "tutorialspoint");
    // printf("String = %s,  Address = %u\n", str, str);

    // /* Reallocating memory */
    // str = (char *) realloc(str, 25);
    // strcat(str, ".com");
    // printf("String = %s,  Address = %u\n", str, str);

    // free(str);

    int index = FIRST_NAME_INDEX; //12
    int is_index_name_length = 1;
    int next_index_name_length;
    int total_length = 0;
    int q_name_index = 0;
    // char* question_name = malloc(sizeof(char));
    char domain_name[100];

    unsigned char zero_hex =0x0;

    /* ***************************** Question Name ************************************** */

    // Going thorugh the label
    while (message[index] != zero_hex) {

        // Current index arrives at the next label length
        if (next_index_name_length == index) {
            is_index_name_length = 1;

            // Adding dot to the separated name
            if (index!=FIRST_NAME_INDEX) {
                total_length+= DOT;
                domain_name[q_name_index] = '.';
                q_name_index++;
            }
        }

        // current index is telling about the length of label
        if (is_index_name_length) {
            int label_length = (int)message[index];
            next_index_name_length = index+ label_length + 1;
            total_length += label_length;
            // question_name = (char *) realloc(question_name, total_length);
            is_index_name_length = 0;
            index++;
        } else {
            // printf("Message[%d]: %c\n", index, message[index]);
            domain_name[q_name_index] = message[index];
            q_name_index++;
            index++;
        }
    
    // printf("total len: %d\n", total_length);
    }

    printf("Domain name: %s\n", domain_name);
    printf("index: %d\n", index);

    /* ***************************** Question Class ************************************** */
    // char* question_class = malloc(sizeof(char)*4);
    // question_class = message[index];
    if (message[index+1]==0x0 && message[index+2]==0x1c) {
        printf("DNS type: IPv6\n");
    }
    // printf("%x \n", message[index+1]);
    /* ***************************** Request ************************************** */

    // from https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
    time_t rawtime;
    struct tm *info;
    char time_buffer[80];

    time( &rawtime );

    info = localtime( &rawtime );

    strftime(time_buffer,80,"%FT%T%z", info);

    FILE *f;
    f = fopen("dns_svr.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
    if (f == NULL) { /* Something is wrong   */}

    if (!response){
    fprintf(f, "%s %s %s\n", time_buffer, "requested", domain_name);
    printf("%s %s %s\n", time_buffer, "requested", domain_name);
    }
    /* ***************************** Response ************************************** */
    
    if (response) {
        // Jump to IPv6 Address, +17 from the latest index
        for (int i=0; i<16; i++) {
            // message[index+17]
            printf("message[%d+17]: %x\n", index, message[index+17+i]);
        }
        // printf("message[%d+17]: %x", index, message[index+17]);
        // <timestamp> <domain_name> is at <IP address>
    }

    return(0);

}
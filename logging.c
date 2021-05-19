#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// from https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
char* timestamp(char** ts) {
    time_t rawtime;
    struct tm *info;

    time(&rawtime);
    info = localtime(&rawtime);
    strftime(*ts,80,"%FT%T%z", info);
    
    return *ts;
}

void create_log_file() {
    // from https://www.tutorialspoint.com/c_standard_library/c_function_fopen.htm
    FILE *f;
    f = fopen("dns_svr.log", "w");
    fclose(f);
}

void write_log_request(char* domain_name) {
    // For timestamp
    char* request_log_string = malloc(80);
    char* time_buffer = timestamp(&request_log_string);
    
    // Logging request
    FILE *f;
    f = fopen("dns_svr.log", "a+");
    fprintf(f, "%s %s %s\n", time_buffer, "requested", domain_name);
    printf( "%s %s %s %s\n", "Logged:", time_buffer, "requested",domain_name);
    fclose(f);
}

void write_log_unimplemented_request() {
    // For timestamp
    char* request_log_string = malloc(80);
    char* time_buffer = timestamp(&request_log_string);

    time_buffer = timestamp(&request_log_string);
        FILE *f;
        f = fopen("dns_svr.log", "a+");
        fprintf(f, "%s %s\n", time_buffer, "unimplemented request");
        fclose(f);
    
    free(request_log_string);
}

void write_log_response(char* res_domain_name, char* ip_address) {
    
    char* response_log_string = malloc(80);
    char* res_time_buffer = timestamp(&response_log_string);

    FILE *f;
    f = fopen("dns_svr.log", "a+"); 
    fprintf(f, "%s %s %s %s\n", res_time_buffer, res_domain_name, "is at", ip_address);
    printf("%s %s %s %s %s\n", "Logged:", res_time_buffer, res_domain_name, "is at", ip_address);
    fclose(f);

    free(response_log_string);
}

char* timestamp(char** ts);
void create_log_file();
void write_log_request(char* domain_name);
void write_log_unimplemented_request();
void write_log_response(char* res_domain_name, char* ip_address);

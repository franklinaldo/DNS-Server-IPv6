void find_domain_name(unsigned char* message, char* domain_name, int* index);

void stringify_ip_address(char *cleaned_ip_string, unsigned char* message, int* index);

void merge_sub_dns_messages(unsigned char* buffer, unsigned char* first_sub_message, int* first_sub_message_size, unsigned char* second_sub_message, int* second_sub_message_size);


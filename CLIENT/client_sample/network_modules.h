#pragma once

void print_login_fail_reason(char reason);
void process_packet(char* ptr);
void process_data(char* net_buf, size_t io_byte);
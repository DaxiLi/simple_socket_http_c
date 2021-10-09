//
// Created by aja on 2021/10/7.
//

#ifndef PROJ_LOG_H
#define PROJ_LOG_H


int time_prefix = 1;
void log_use_time_prefix(int toggle);
void log_info(char *format_string, ...);
void log_error(char *format_string, ...);
void log_success(char *format_string, ...);
void log_warning(char *format_string, ...);

#endif //PROJ_LOG_H

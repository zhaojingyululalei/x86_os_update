#ifndef __PATH_H
#define __PATH_H
#include "types.h"

#define PATH_SEPARATOR '/'

// 路径解析状态结构体
typedef struct {
    const char* original_path;  // 原始路径
    char* normalized_path;      // 规范化后的路径
    size_t current_pos;         // 当前位置
    bool is_first;              // 是否是第一次调用
} PathParser;


bool is_separator(char c) ;
bool is_rootdir(const char* path);
char* path_normalize(const char* path) ;
bool path_is_absolute(const char* path);
char* path_join(const char* base, const char* path);
char* path_dirname(const char* path);
char* path_basename(const char* path);
const char* path_extension(const char* path);
PathParser* path_parser_init(const char* path);
bool is_parse_finish(PathParser* parse);
char* path_parser_next(PathParser* parser);
void path_parser_free(PathParser* parser);
char* path_to_absolute(const char* cwd, const char* relative_path);
char *strdup(const char *s);
#endif
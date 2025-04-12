#include "fs/path.h"
#include "string.h"
#include "mem/kmalloc.h"
static char *strdup(const char *s)
{
    size_t len = strlen(s);
    len +=1;
    char* new_s = (char*)kmalloc(len);
    strncpy(new_s,s,len);
    return new_s;
}
// 判断字符是否是路径分隔符
bool is_separator(char c) {
    return c == PATH_SEPARATOR;
}
bool is_rootdir(const char* path)
{
    if(strcmp(path,"/")==0)
    {
        return true;
    }
    return false;
}
/**
 * @brief 规范化路径 (移除多余的斜杠、处理.和..等)
 * @param path 原始路径
 * @return 规范化后的路径字符串，需要调用者释放
 */
char* path_normalize(const char* path) {
    if (path == NULL) return NULL;
    
    size_t len = strlen(path);
    if (len == 0) return strdup(".");
    
    // 检查路径是否已经是简单形式（没有需要规范化的内容）
    bool needs_normalization = false;
    bool has_dot = false;
    bool has_double_dot = false;
    bool has_double_slash = false;
    
    for (size_t i = 0; i < len; i++) {
        if (path[i] == '.') {
            if (i + 1 < len && path[i+1] == '.') {
                has_double_dot = true;
            } else {
                has_dot = true;
            }
        } else if (i > 0 && is_separator(path[i]) && is_separator(path[i-1])) {
            has_double_slash = true;
        }
    }
    
    // 如果路径不需要任何规范化，直接返回副本
    if (!has_dot && !has_double_dot && !has_double_slash) {
        return strdup(path);
    }
    
    // 需要规范化的情况
    char* result = kmalloc(len + 2);
    if (result == NULL) return NULL;
    
    size_t pos = 0;
    bool is_absolute = is_separator(path[0]);
    
    // 处理绝对路径
    if (is_absolute) {
        result[pos++] = PATH_SEPARATOR;
    }
    
    // 临时缓冲区用于存放路径组件
    char* component = kmalloc(len + 1);
    size_t comp_len = 0;
    
    for (size_t i = is_absolute ? 1 : 0; i <= len; i++) {
        // 遇到分隔符或字符串结尾时处理当前组件
        if (i == len || is_separator(path[i])) {
            if (comp_len == 0) continue; // 跳过空组件
            
            component[comp_len] = '\0';
            
            // 处理 "." 组件
            if (strcmp(component, ".") == 0) {
                comp_len = 0;
                continue;
            }
            
            // 处理 ".." 组件
            if (strcmp(component, "..") == 0) {
                if (pos > 0) {
                    // 回退到上一级目录
                    while (pos > 0 && !is_separator(result[pos - 1])) {
                        pos--;
                    }
                    if (pos > 0) {
                        pos--; // 移除前一个分隔符
                    }
                }
                comp_len = 0;
                continue;
            }
            
            // 添加路径分隔符（如果不是第一个组件）
            if (pos > 0 && !(pos == 1 && is_absolute)) {
                result[pos++] = PATH_SEPARATOR;
            }
            
            // 添加组件到结果
            memcpy(result + pos, component, comp_len);
            pos += comp_len;
            comp_len = 0;
        } else {
            // 收集组件字符
            component[comp_len++] = path[i];
        }
    }
    
    kfree(component);
    
    // 确保以空字符结尾
    result[pos] = '\0';
    
    // 处理空路径的情况
    if (pos == 0) {
        kfree(result);
        return is_absolute ? strdup("/") : strdup(".");
    }
    
    return result;
}

// 检查路径是否是绝对路径
bool path_is_absolute(const char* path) {
    return path != NULL && is_separator(path[0]);
}

// 组合两个路径
char* path_join(const char* base, const char* path) {
    if (base == NULL || *base == '\0') return path_normalize(path);
    if (path == NULL || *path == '\0') return path_normalize(base);
    
    // 如果path是绝对路径，直接返回path
    if (path_is_absolute(path)) {
        return path_normalize(path);
    }
    
    size_t base_len = strlen(base);
    size_t path_len = strlen(path);
    
    char* result = kmalloc(base_len + path_len + 2);
    if (result == NULL) return NULL;
    
    strcpy(result, base);
    
    // 确保base以分隔符结尾
    if (base_len > 0 && !is_separator(result[base_len - 1])) {
        result[base_len] = PATH_SEPARATOR;
        base_len++;
        result[base_len] = '\0';
    }
    
    strcat(result, path);
    
    char* normalized = path_normalize(result);
    kfree(result);
    
    return normalized;
}

// 获取路径的父目录
char* path_dirname(const char* path) {
    if (path == NULL || *path == '\0') return strdup(".");
    
    char* normalized = path_normalize(path);
    if (normalized == NULL) return NULL;
    
    size_t len = strlen(normalized);
    
    // 根目录的情况
    if (len == 1 && is_separator(normalized[0])) {
        return normalized; // 已经是"/"，直接返回
    }
    
    // 找到最后一个分隔符
    size_t last_sep = len;
    while (last_sep > 0 && !is_separator(normalized[last_sep - 1])) {
        last_sep--;
    }
    
    // 没有找到分隔符
    if (last_sep == 0) {
        kfree(normalized);
        return strdup(".");
    }
    
    // 只有根目录的情况
    if (last_sep == 1 && is_separator(normalized[0])) {
        normalized[1] = '\0';
        return normalized;
    }
    
    // 截取到最后一个分隔符之前
    normalized[last_sep - 1] = '\0';
    
    // 如果结果为空，返回当前目录
    if (normalized[0] == '\0') {
        kfree(normalized);
        return strdup(".");
    }
    
    return normalized;
}

// 获取路径的最后一部分 (文件名或目录名)
char* path_basename(const char* path) {
    if (path == NULL || *path == '\0') return strdup(".");
    
    char* normalized = path_normalize(path);
    if (normalized == NULL) return NULL;
    
    size_t len = strlen(normalized);
    
    // 根目录的情况
    if (len == 1 && is_separator(normalized[0])) {
        kfree(normalized);
        return strdup("/");
    }
    
    // 找到最后一个分隔符
    size_t last_sep = len;
    while (last_sep > 0 && !is_separator(normalized[last_sep - 1])) {
        last_sep--;
    }
    
    char* basename = strdup(normalized + last_sep);
    kfree(normalized);
    
    // 如果没有basename，返回当前目录
    if (basename[0] == '\0') {
        kfree(basename);
        return strdup(".");
    }
    
    return basename;
}

// 获取文件扩展名
const char* path_extension(const char* path) {
    if (path == NULL) return NULL;
    
    const char* basename = path;
    const char* last_sep = strchr(path, PATH_SEPARATOR);
    if (last_sep != NULL) {
        basename = last_sep + 1;
    }
    
    const char* last_dot = strchr(basename, '.');
    if (last_dot == NULL || last_dot == basename) {
        return NULL; // 没有扩展名或隐藏文件(.开头)
    }
    
    return last_dot + 1;
}
// 初始化路径解析器
PathParser* path_parser_init(const char* path) {
    if (path == NULL) return NULL;
    
    PathParser* parser = kmalloc(sizeof(PathParser));
    if (parser == NULL) return NULL;
    
    parser->original_path = path;
    parser->normalized_path = path_normalize(path);
    parser->current_pos = 0;
    parser->is_first = true;
    
    if (parser->normalized_path == NULL) {
        kfree(parser);
        return NULL;
    }
    
    return parser;
}

// 获取下一层路径组件
char* path_parser_next(PathParser* parser) {
    if (parser == NULL || parser->normalized_path == NULL) {
        return NULL;
    }
    
    size_t len = strlen(parser->normalized_path);
    
    // 处理根目录的特殊情况
    if (parser->is_first && is_separator(parser->normalized_path[0])) {
        parser->is_first = false;
        
        // 如果路径就是根目录
        if (len == 1) {
            return strdup("/");
        }
        
        // 返回根目录
        parser->current_pos = 1;
        return strdup("/");
    }
    
    // 已经到达路径末尾
    if (parser->current_pos >= len) {
        return NULL;
    }
    
    // 找到下一个分隔符
    size_t start = parser->current_pos;
    size_t end = start;
    while (end < len && !is_separator(parser->normalized_path[end])) {
        end++;
    }
    
    // 提取当前组件
    size_t component_len = end - start;
    if (component_len == 0) {
        return NULL;
    }
    
    char* component = kmalloc(component_len + 1);
    if (component == NULL) {
        return NULL;
    }
    
    strncpy(component, parser->normalized_path + start, component_len);
    component[component_len] = '\0';
    
    // 更新位置
    parser->current_pos = end + 1;
    
    return component;
}
bool is_parse_finish(PathParser* parse)
{
    int len = strlen(parse->normalized_path);
    if(parse->current_pos < len)
    {
        return false;
    }
    return true;

}
// 释放路径解析器
void path_parser_free(PathParser* parser) {
    if (parser == NULL) return;
    
    kfree(parser->normalized_path);
    kfree(parser);
}

/**
 * 
 * // 测试路径规范化
    char* test1 = path_normalize("/foo/../bar/./baz//qux");
    printf("Normalized: %s\n", test1); // 输出: /bar/baz/qux
    free(test1);
    
    // 测试路径组合
    char* test2 = path_join("/foo/bar", "../baz");
    printf("Joined: %s\n", test2); // 输出: /foo/baz
    free(test2);
    
    // 测试获取父目录
    char* test3 = path_dirname("/foo/bar/baz.txt");
    printf("Dirname: %s\n", test3); // 输出: /foo/bar
    free(test3);
    
    // 测试获取basename
    char* test4 = path_basename("/foo/bar/baz.txt");
    printf("Basename: %s\n", test4); // 输出: baz.txt
    free(test4);
    
    // 测试获取扩展名
    const char* test5 = path_extension("/foo/bar.baz.txt");
    printf("Extension: %s\n", test5); // 输出: txt
 */
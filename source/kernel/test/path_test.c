#include "fs/path.h"
#include "printk.h"

void path_test(void) {
    // 测试路径规范化
    char* test1 = path_normalize("/foo/../bar/./baz//qux");
    dbg_info("Normalized: %s\r\n", test1); // 预期输出: /bar/baz/qux
    kfree(test1);

    // 测试路径组合
    char* test2 = path_join("/foo/bar", "../baz");
    dbg_info("Joined: %s\r\n", test2); // 预期输出: /foo/baz
    kfree(test2);

    // 测试获取父目录
    char* test3 = path_dirname("/foo/bar/baz.txt");
    dbg_info("Dirname: %s\r\n", test3); // 预期输出: /foo/bar
    kfree(test3);

    // 测试获取文件名
    char* test4 = path_basename("/foo/bar/baz.txt");
    dbg_info("Basename: %s\r\n", test4); // 预期输出: baz.txt
    kfree(test4);

    // 测试获取扩展名
    const char* test5 = path_extension("/foo/bar.baz.txt");
    dbg_info("Extension: %s\r\n", test5); // 预期输出: txt

    // 测试相对路径转绝对路径
    char* test6 = path_to_absolute("/home/user", "docs/file.txt");
    dbg_info("Absolute Path: %s\r\n", test6); // 预期输出: /home/user/docs/file.txt
    kfree(test6);

    // 测试路径解析器
    PathParser* parser = path_parser_init("/foo/bar/baz.txt");
    if (parser) {
        char* component;
        while ((component = path_parser_next(parser)) != NULL) {
            dbg_info("Path Component: %s\r\n", component);
            kfree(component);
        }
        path_parser_free(parser);
    }
}
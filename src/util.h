#ifndef DAILYPLUGIN_UTIL_H
#define DAILYPLUGIN_UTIL_H

#include <stddef.h>

/* 简易动态字符串缓冲 */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} SB;

void sb_init(SB *sb);
void sb_append(SB *sb, const char *s, size_t n);
void sb_append_cstr(SB *sb, const char *s);
void sb_free(SB *sb);
char *sb_cstr(SB *sb); /* 返回可写 cstr，data 归调用者所有 */

/* 文件 */
char *read_file(const char *path, size_t *out_len);
int write_file(const char *path, const void *data, size_t n);
int file_exists(const char *path);
int dir_exists(const char *path);

/* 目录创建（递归） */
int mkdir_p(const char *path);

/* 执行外部命令，返回退出码 */
int run_command(const char *cmd);

/* 模板文件路径定位 */
char *resolve_templates_dir(void);

/* 简单字符串工具 */
int ends_with(const char *s, const char *suffix);
char *path_join(const char *a, const char *b);

#endif /* DAILYPLUGIN_UTIL_H */

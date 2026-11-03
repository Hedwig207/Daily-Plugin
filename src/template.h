#ifndef DAILYPLUGIN_TEMPLATE_H
#define DAILYPLUGIN_TEMPLATE_H

#include <stddef.h>
#include "util.h"

/* 模板变量键值对 */
typedef struct {
    char *key;
    char *value;
} TplPair;

typedef struct {
    TplPair *items;
    size_t count;
    size_t cap;
} TplVars;

void tpl_init(TplVars *v);
void tpl_set(TplVars *v, const char *key, const char *value);
void tpl_free(TplVars *v);
const char *tpl_get(const TplVars *v, const char *key);

/* 将文本中的 {{key}} 全部替换为变量值，返回新分配的字符串 */
char *tpl_render(const char *text, size_t len, const TplVars *v);

/* 读取文件 -> 渲染 -> 写入输出文件。成功返回 0。 */
int tpl_render_file(const char *src, const char *dst, const TplVars *v);

#endif /* DAILYPLUGIN_TEMPLATE_H */

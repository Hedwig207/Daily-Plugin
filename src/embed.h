#ifndef DAILYPLUGIN_EMBED_H
#define DAILYPLUGIN_EMBED_H

#include <stddef.h>

/* 内嵌模板表条目（由构建脚本生成 templates_embed.c） */
typedef struct {
    const char *rel;  /* 相对路径，如 "forge/build.gradle" */
    const char *data; /* 文件内容（编译期常量） */
    unsigned int len; /* 内容长度 */
} DpTplFile;

/* 以 {NULL, NULL, 0} 结尾 */
extern const DpTplFile DP_TPL_FILES[];

/* 内嵌模板的签名（文件集合 + 内容长度），用于判断缓存是否过期 */
extern const char *DP_TPL_SIG;

/* 将内嵌模板释放到 %LOCALAPPDATA%\dp\templates 并返回该目录（malloc，需 free）；
 * 已是最新则直接返回。失败返回 NULL。 */
char *extract_embedded_templates(void);

#endif /* DAILYPLUGIN_EMBED_H */
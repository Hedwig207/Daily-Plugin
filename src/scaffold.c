#include "scaffold.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#define MAIN_TEMPLATE_REL "src/main/java/com/example/mod/ExampleMod.java"

/* 递归遍历模板目录：
 *  - 目录：递归
 *  - 文件：若 rel == MAIN_TEMPLATE_REL 则跳过（主类已单独生成），
 *          否则将渲染后的内容写入 out_dir/rel */
static int recurse(const char *src_dir, const char *dst_dir,
                   const TplVars *vars) {
    DIR *d = opendir(src_dir);
    if (!d) return -1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;

        char *src = path_join(src_dir, e->d_name);
        char *dst = path_join(dst_dir, e->d_name);
        int is_dir = dir_exists(src);

        if (is_dir) {
            mkdir_p(dst);
            recurse(src, dst, vars);
        } else {
            /* 主类模板由调用方单独生成到真实包路径 */
            if (strcmp(e->d_name, "ExampleMod.java") != 0) {
                printf("  [make] %s\n", dst);
                if (tpl_render_file(src, dst, vars) != 0) {
                    fprintf(stderr, "Error: failed to write %s\n", dst);
                    free(src); free(dst);
                    closedir(d);
                    return -1;
                }
            }
        }
        free(src);
        free(dst);
    }
    closedir(d);
    return 0;
}

int scaffold_mod(const char *templates_dir,
                 const char *out_dir,
                 const TplVars *variables,
                 const char *package_path,
                 const char *mod_class) {
    /* 1. 生成主类文件到真实包路径（package_path 为点分隔，转成目录斜杠） */
    size_t rel_len = strlen(package_path) + strlen(mod_class) + 8;
    char *main_rel = (char*)malloc(rel_len);
    if (!main_rel) return -1;
    {
        char *w = main_rel;
        for (const char *p = package_path; *p; p++) *w++ = (*p == '.') ? '/' : *p;
        *w++ = '/';
        for (const char *p = mod_class; *p; p++) *w++ = *p;
        strcpy(w, ".java");
    }
    char *main_src = path_join(templates_dir, "src/main/java/com/example/mod/ExampleMod.java");
    char *main_dst_root = path_join(out_dir, "src/main/java");
    char *main_dst = path_join(main_dst_root, main_rel);
    printf("  [make] main class %s\n", main_rel);
    if (tpl_render_file(main_src, main_dst, variables) != 0) {
        fprintf(stderr, "Error: failed to generate main class\n");
        free(main_rel); free(main_src); free(main_dst_root); free(main_dst);
        return -1;
    }
    free(main_src); free(main_dst_root); free(main_dst);

    /* 2. 递归复制其余模板文件（主类除外） */
    if (recurse(templates_dir, out_dir, variables) != 0) {
        free(main_rel);
        return -1;
    }
    free(main_rel);
    return 0;
}

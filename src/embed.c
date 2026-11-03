#include "embed.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 模板释放目录：%LOCALAPPDATA%\dp\templates */
static char *templates_cache_dir(void) {
    const char *base = getenv("LOCALAPPDATA");
    if (!base) base = getenv("APPDATA");
    if (!base) base = ".";
    char *dp = path_join(base, "dp");
    char *dir = path_join(dp, "templates");
    free(dp);
    return dir;
}

/* 为文件路径创建其父目录 */
static int ensure_parent_dir(const char *file_path) {
    char *p = strdup(file_path);
    if (!p) return -1;
    for (char *c = p; *c; c++) if (*c == '\\') *c = '/';
    char *slash = strrchr(p, '/');
    int rc = 0;
    if (slash) {
        *slash = '\0';
        if (*p) rc = mkdir_p(p);
    }
    free(p);
    return rc;
}

char *extract_embedded_templates(void) {
    char *dir = templates_cache_dir();
    if (!dir) return NULL;

    char *marker = path_join(dir, ".sig");
    char *old = read_file(marker, NULL);
    if (old && DP_TPL_SIG && strcmp(old, DP_TPL_SIG) == 0) {
        free(old);
        free(marker);
        return dir;
    }
    free(old);

    if (mkdir_p(dir) != 0) {
        fprintf(stderr, "Error: cannot create template cache: %s\n", dir);
        free(marker);
        free(dir);
        return NULL;
    }

    for (int i = 0; DP_TPL_FILES[i].rel; i++) {
        char *dst = path_join(dir, DP_TPL_FILES[i].rel);
        if (ensure_parent_dir(dst) != 0 ||
            write_file(dst, DP_TPL_FILES[i].data, DP_TPL_FILES[i].len) != 0) {
            fprintf(stderr, "Error: failed to extract template: %s\n", dst);
            free(dst);
            free(marker);
            free(dir);
            return NULL;
        }
        free(dst);
    }

    if (DP_TPL_SIG) write_file(marker, DP_TPL_SIG, strlen(DP_TPL_SIG));
    free(marker);
    return dir;
}
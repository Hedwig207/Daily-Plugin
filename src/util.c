#include "util.h"
#include "embed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include <windows.h>
#define RPATH_SEP '\\'
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#define RPATH_SEP '/'
#endif

void sb_init(SB *sb) {
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static void sb_reserve(SB *sb, size_t extra) {
    if (sb->len + extra + 1 > sb->cap) {
        size_t ncap = sb->cap ? sb->cap : 32;
        while (ncap < sb->len + extra + 1) ncap *= 2;
        char *nd = (char*)realloc(sb->data, ncap);
        if (!nd) {
            fprintf(stderr, "内存分配失败\n");
            exit(1);
        }
        sb->data = nd;
        sb->cap = ncap;
    }
}

void sb_append(SB *sb, const char *s, size_t n) {
    if (!s || n == 0) return;
    sb_reserve(sb, n);
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sb_append_cstr(SB *sb, const char *s) {
    if (s) sb_append(sb, s, strlen(s));
}

char *sb_cstr(SB *sb) {
    if (!sb->data) {
        sb->data = (char*)malloc(1);
        sb->data[0] = '\0';
        sb->cap = 1;
    }
    sb->data[sb->len] = '\0';
    return sb->data;
}

void sb_free(SB *sb) {
    free(sb->data);
    sb_init(sb);
}

char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len) *out_len = rd;
    return buf;
}

int write_file(const char *path, const void *data, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t w = fwrite(data, 1, n, f);
    fclose(f);
    return (w == n) ? 0 : -1;
}

static int path_kind(const char *p, int *is_dir) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(p);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    *is_dir = (attr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
    return 1;
#else
    struct stat st;
    if (stat(p, &st) != 0) return 0;
    *is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
    return 1;
#endif
}

int file_exists(const char *path) {
    int d = 0;
    int ok = path_kind(path, &d);
    return ok && !d;
}

int dir_exists(const char *path) {
    int d = 0;
    return path_kind(path, &d) && d;
}

int mkdir_p(const char *path) {
    if (dir_exists(path)) return 0;
    /* 向上递归 */
    char *tmp = strdup(path);
    size_t len = strlen(tmp);
    /* 去掉末尾分隔符 */
    while (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) tmp[--len] = '\0';
    for (size_t i = 1; i < len; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            tmp[i] = '\0';
            if (!dir_exists(tmp)) {
#ifdef _WIN32
                _mkdir(tmp);
#else
                mkdir(tmp, 0755);
#endif
            }
            tmp[i] = RPATH_SEP;
        }
    }
    int rc = 0;
    if (!dir_exists(tmp)) {
#ifdef _WIN32
        rc = _mkdir(tmp);
#else
        rc = mkdir(tmp, 0755);
#endif
        if (rc == 0) rc = 0;
    }
    free(tmp);
    return 0;
}

int run_command(const char *cmd) {
    int rc = system(cmd);
#ifdef _WIN32
    if (rc == -1) return -1;
    /* 该 CRT 的 system() 把退出码放在低字节 */
    return rc & 0xFF;
#else
    if (rc == -1) return -1;
    return WEXITSTATUS(rc);
#endif
}

char *path_join(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    int need_sep = 0;
    if (la > 0 && a[la - 1] != '/' && a[la - 1] != '\\') need_sep = 1;
    char *out = (char*)malloc(la + lb + 2);
    if (!out) return NULL;
    memcpy(out, a, la);
    if (need_sep) out[la++] = RPATH_SEP;
    memcpy(out + la, b, lb);
    out[la + lb] = '\0';
    return out;
}

int ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t ls = strlen(s), lx = strlen(suffix);
    if (lx > ls) return 0;
    return strcmp(s + ls - lx, suffix) == 0;
}

char *resolve_templates_dir(void) {
    const char *env = getenv("DAILY_PLUGIN_TEMPLATES");
    if (env && dir_exists(env)) return strdup(env);

    const char *paths[] = { "./templates", NULL };
    for (int i = 0; paths[i]; i++) {
        if (dir_exists(paths[i])) return strdup(paths[i]);
    }

#ifdef _WIN32
    static char exe[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
    if (n > 0) {
        char *slash = strrchr(exe, '\\');
        if (slash) *slash = '\0';
        char *tpl = path_join(exe, "templates");
        if (dir_exists(tpl)) return tpl;
        free(tpl);
    }
#endif

    /* 回退：释放 exe 内嵌的模板到本地缓存（dp.exe 可单文件使用） */
    return extract_embedded_templates();
}
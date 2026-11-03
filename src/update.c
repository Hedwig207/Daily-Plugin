#include "update.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define POWERSHELL "powershell -NoProfile -ExecutionPolicy Bypass -Command"
#else
#define CURL "curl"
#endif

/* 默认仓库地址（可被环境变量 DP_UPDATE_REPO 覆盖） */
#define DAILYPLUGIN_DEFAULT_REPO "Hedwig207/Daily-Plugin"

static const char *VERSION = "0.3.0";

const char *dp_version(void) { return VERSION; }

char *update_repo(void) {
    const char *env = getenv("DP_UPDATE_REPO");
    return strdup(env && *env ? env : DAILYPLUGIN_DEFAULT_REPO);
}

int version_cmp(const char *a, const char *b) {
    if (!a || !b) return 0;
    const char *pa = a, *pb = b;
    /* 跳过前导 v/V */
    if (*pa == 'v' || *pa == 'V') pa++;
    if (*pb == 'v' || *pb == 'V') pb++;
    while (*pa || *pb) {
        /* 提取数字段 */
        long na = 0, nb = 0;
        while (*pa >= '0' && *pa <= '9') { na = na * 10 + (*pa - '0'); pa++; }
        while (*pb >= '0' && *pb <= '9') { nb = nb * 10 + (*pb - '0'); pb++; }
        if (na != nb) return na > nb ? 1 : -1;
        /* 跳过非数字分隔符 */
        while (*pa && !(*pa >= '0' && *pa <= '9')) pa++;
        while (*pb && !(*pb >= '0' && *pb <= '9')) pb++;
    }
    return 0;
}

/* 将路径分隔符统一为 / 并执行命令 */
static int run(const char *cmd) { return run_command(cmd); }

static int http_get_json(const char *url, SB *out) {
    SB full; sb_init(&full);
    /* 写到一个临时文件再读取，避免命令行转义复杂 */
    char tmpfile[256];
#ifdef _WIN32
    snprintf(tmpfile, sizeof(tmpfile), "%s_dp_release_tmp.json", getenv("TEMP") ? getenv("TEMP") : ".");
#else
    snprintf(tmpfile, sizeof(tmpfile), "/tmp/dp_release_tmp.json");
#endif
    size_t n = 1024 + strlen(url) * 2;
    char *cmd = (char*)malloc(n);
    char *u = strdup(url);
    for (char *c = u; *c; c++) if (*c == '\\') *c = '/';
    char *t = strdup(tmpfile);
    for (char *c = t; *c; c++) if (*c == '\\') *c = '/';
#ifdef _WIN32
    snprintf(cmd, n, "%s \"Invoke-WebRequest -Uri '%s' -OutFile '%s'\"",
             POWERSHELL, u, t);
#else
    snprintf(cmd, n, "%s -L -s -o '%s' '%s'", CURL, t, u);
#endif
    free(u); free(t);
    int rc = run(cmd);
    free(cmd);
    if (rc != 0) { sb_free(&full); return -1; }

    char *data = read_file(tmpfile, NULL);
    /* 清理临时文件 */
#ifdef _WIN32
    {
        size_t m = strlen(tmpfile) + 64;
        char *d = strdup(tmpfile);
        for (char *c = d; *c; c++) if (*c == '\\') *c = '/';
        char *rm = (char*)malloc(m);
        snprintf(rm, m, "%s \"Remove-Item -Force '%s' -ErrorAction SilentlyContinue\"", POWERSHELL, d);
        run(rm);
        free(rm); free(d);
    }
#else
    remove(tmpfile);
#endif
    if (!data) { sb_free(&full); return -1; }
    sb_append_cstr(&full, data);
    free(data);
    sb_append(&full, "\0", 1);
    sb_free(out);
    *out = full;
    return 0;
}

/* 在 JSON 中提取 "key": value 的字符串原文字段，返回新分配字符串 */
static char *json_str(const char *json, const char *key) {
    if (!json) return NULL;
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return NULL;
    p += strlen(needle);
    while (*p && (*p == ':' || *p == ' ' || *p == '\t')) p++;
    if (*p != '"') return NULL;
    p++;
    SB out; sb_init(&out);
    for (; *p && *p != '"'; p++) {
        if (*p == '\\' && p[1]) { p++; sb_append(&out, p, 1); }
        else sb_append(&out, p, 1);
    }
    char *res = sb_cstr(&out);
    /* 去除末尾可能残留的 \0 */
    sb_free(&out);
    return res;
}

int check_latest(const char *repo, char **out_tag) {
    char url[1024];
    snprintf(url, sizeof(url), "https://api.github.com/repos/%s/releases/latest", repo);
    SB json; sb_init(&json);
    if (http_get_json(url, &json) != 0) {
        sb_free(&json);
        return -1;
    }
    char *tag = json_str(json.data, "tag_name");
    if (!tag) { sb_free(&json); return -2; }
    *out_tag = tag;
    sb_free(&json);
    return 0;
}

int download_latest_dp(const char *repo, const char *out_file) {
    /* 直接构造 CDN 释放资产 URL 更可靠，但需知道资产名；这里查询 API 资产列表 */
    char url[1024];
    snprintf(url, sizeof(url), "https://api.github.com/repos/%s/releases/latest", repo);
    SB json; sb_init(&json);
    if (http_get_json(url, &json) != 0) { sb_free(&json); return -1; }

    char *dl = NULL;
    /* 在 json 中找 "browser_download_url" 匹配 dp-windows */
    const char *p = json.data;
    while ((p = strstr(p, "browser_download_url")) != NULL) {
        p += strlen("browser_download_url");
        while (*p && (*p == ':' || *p == ' ' || *p == '\t')) p++;
        if (*p != '"') { p++; continue; }
        p++;
        SB u; sb_init(&u);
        for (; *p && *p != '"'; p++) {
            if (*p == '\\' && p[1]) { p++; sb_append(&u, p, 1); }
            else sb_append(&u, p, 1);
        }
        char *asset = sb_cstr(&u);
        sb_free(&u);
        if (strstr(asset, "dp-windows") || strstr(asset, "dp.exe")) {
            dl = asset;
            break;
        }
        free(asset);
    }
    if (!dl) { sb_free(&json); return -2; }

    int rc = 0;
    size_t n = 1024 + strlen(dl) + strlen(out_file) * 2;
    char *cmd = (char*)malloc(n);
    char *suba = strdup(dl), *o = strdup(out_file);
    for (char *c = suba; *c; c++) if (*c == '\\') *c = '/';
    for (char *c = o; *c; c++) if (*c == '\\') *c = '/';
#ifdef _WIN32
    snprintf(cmd, n, "%s \"Invoke-WebRequest -Uri '%s' -OutFile '%s'\"", POWERSHELL, suba, o);
#else
    snprintf(cmd, n, "%s -L -o '%s' '%s'", CURL, o, suba);
#endif
    free(suba); free(o);
    rc = run(cmd);
    free(cmd);
    free(dl);
    sb_free(&json);
    return rc == 0 ? 0 : -1;
}

int self_update(const char *current_dir) {
    char *repo = update_repo();
    if (!repo) return -1;
    char *latest = NULL;
    int rc = check_latest(repo, &latest);
    if (rc != 0) {
        fprintf(stderr, "Update check failed (network or repo error).\n");
        free(repo);
        return -1;
    }
    int cmp = version_cmp(latest, VERSION);
    printf("Current version : %s\n", VERSION);
    printf("Latest on GitHub: %s\n", latest);
    if (cmp <= 0) {
        printf("You are up to date.\n");
        free(latest); free(repo);
        return 0;
    }
    printf("A new version (%s) is available.\n", latest);
    printf("  1. download it  (dp update -download)\n");
    printf("  2. skip\n");
    printf("Choose [1/2]: ");
    fflush(stdout);
    char line[64];
    if (!fgets(line, sizeof(line), stdin) || (line[0] != '1' && line[0] != '\r' && line[0] != '\n')) {
        printf("Skipped.\n");
        free(latest); free(repo);
        return 1;
    }
    /* 下载到当前目录 */
    char out_path[1024];
    snprintf(out_path, sizeof(out_path), "%s/dp_latest.exe", current_dir);
    printf("Downloading %s ...\n", out_path);
    if (download_latest_dp(repo, out_path) != 0) {
        fprintf(stderr, "Download failed.\n");
        free(latest); free(repo);
        return -1;
    }
    printf("Downloaded. Replace your current dp.exe with dp_latest.exe, then re-run.\n");
    free(latest); free(repo);
    return 1;
}
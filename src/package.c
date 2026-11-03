#include "package.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define POWERSHELL "powershell -NoProfile -ExecutionPolicy Bypass -Command"
#else
#define ZIP "zip"
#define UNZIP "unzip"
#endif

int pack_dir(const char *dir, const char *plug_file) {
    /* 确保目录以 sep 结尾，使 * 通配可命中内容 */
    size_t cmd_len = 1024 + strlen(dir) * 2 + strlen(plug_file) * 2;
    char *cmd = (char*)malloc(cmd_len);
    if (!cmd) return -1;
    int rc;

#ifdef _WIN32
    /* Compress-Archive 只接受 .zip 扩展名，先打包为临时 zip 再改名 */
    char *d = strdup(dir);
    char *p = strdup(plug_file);
    for (char *c = d; *c; c++) if (*c == '\\') *c = '/';
    for (char *c = p; *c; c++) if (*c == '\\') *c = '/';
    char *tmp = (char*)malloc(strlen(p) + 8);
    sprintf(tmp, "%s.zip", p);
    snprintf(cmd, cmd_len,
             "%s \"$s='%s/*'; $tmp='%s'; $t='%s'; "
             "if(Test-Path $t){Remove-Item -Force $t}; "
             "Compress-Archive -Path $s -DestinationPath $tmp -Force; "
             "Move-Item -Force $tmp $t\"",
             POWERSHELL, d, tmp, p);
    free(d); free(p); free(tmp);
    printf("  packing: %s -> %s\n", dir, plug_file);
    rc = run_command(cmd);
#else
    snprintf(cmd, cmd_len, "%s -r %s .", ZIP, plug_file);
    rc = run_command(cmd);
#endif
    free(cmd);
    return rc == 0 ? 0 : -1;
}

int unpack_plug(const char *plug_file, const char *out_dir) {
    size_t cmd_len = 1024 + strlen(plug_file) * 2 + strlen(out_dir) * 2;
    char *cmd = (char*)malloc(cmd_len);
    if (!cmd) return -1;
    int rc;

#ifdef _WIN32
    /* Expand-Archive 只接受 .zip，先把 .plug 复制为临时 zip 再解压 */
    char *p = strdup(plug_file);
    char *o = strdup(out_dir);
    for (char *c = p; *c; c++) if (*c == '\\') *c = '/';
    for (char *c = o; *c; c++) if (*c == '\\') *c = '/';
    char *tmp = (char*)malloc(strlen(p) + 8);
    sprintf(tmp, "%s.zip", p);
    snprintf(cmd, cmd_len,
             "%s \"$tmp='%s'; Copy-Item -Force '%s' $tmp; $o='%s'; "
             "if(Test-Path $o){Remove-Item -Recurse -Force $o}; "
             "Expand-Archive -Path $tmp -DestinationPath $o -Force; "
             "Remove-Item -Force $tmp\"",
             POWERSHELL, tmp, p, o);
    free(p); free(o); free(tmp);
    printf("  unpacking: %s -> %s\n", plug_file, out_dir);
    rc = run_command(cmd);
#else
    snprintf(cmd, cmd_len, "mkdir -p %s && %s %s -d %s", out_dir, UNZIP, plug_file, out_dir);
    rc = run_command(cmd);
#endif
    free(cmd);
    return rc == 0 ? 0 : -1;
}

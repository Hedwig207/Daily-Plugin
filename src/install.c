#include "install.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define POWERSHELL "powershell -NoProfile -ExecutionPolicy Bypass -Command"
#include <windows.h>
#define DEFAULT_FORGE_MDK \
    "https://maven.minecraftforge.net/net/minecraftforge/forge/1.20.1-47.2.0/forge-1.20.1-47.2.0-mdk.zip"
#else
#define CURL "curl"
#define UNZIP "unzip"
#define DEFAULT_FORGE_MDK \
    "https://maven.minecraftforge.net/net/minecraftforge/forge/1.20.1-47.2.0/forge-1.20.1-47.2.0-mdk.zip"
#define JDK17_URL \
    "https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.10%2B7/OpenJDK17U-jdk_x64_linux_hotspot_17.0.10_7.tar.gz"
#include <dirent.h>
#endif

#ifdef _WIN32
#define JDK17_URL \
    "https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.10%2B7/OpenJDK17U-jdk_x64_windows_hotspot_17.0.10_7.zip"
#define GRADLE_URL "https://services.gradle.org/distributions/gradle-8.7-bin.zip"
#else
#define GRADLE_URL "https://services.gradle.org/distributions/gradle-8.7-bin.zip"
#endif

int loader_supported(const char *loader) {
    static const char *known[] = {"forge", "fabric", "neoforge", "quilt", "liteloader", NULL};
    for (int i = 0; known[i]; i++)
        if (strcmp(known[i], loader) == 0) return 1;
    return 0;
}

char *install_home(void) {
    const char *base = getenv("LOCALAPPDATA");
    if (!base) base = getenv("APPDATA");
    char *home = base ? path_join(base, "dp/libs") : strdup("./.dp/libs");
    return home;
}

char *loader_dir(const char *loader) {
    char *home = install_home();
    char *dir = path_join(home, loader);
    free(home);
    return dir;
}

int is_installed(const char *loader) {
    char *dir = loader_dir(loader);
    char *marker = path_join(dir, "installed");
    int ok = file_exists(marker);
    free(marker);
    free(dir);
    return ok;
}

static int download(const char *url, const char *out_file) {
    size_t n = 1024 + strlen(url) + strlen(out_file) * 2;
    char *cmd = (char*)malloc(n);
    char *o = strdup(out_file);
    for (char *c = o; *c; c++) if (*c == '\\') *c = '/';
#ifdef _WIN32
    snprintf(cmd, n, "%s \"Invoke-WebRequest -Uri '%s' -OutFile '%s'\"", POWERSHELL, url, o);
#else
    snprintf(cmd, n, "%s -L -o '%s' '%s'", CURL, o, url);
#endif
    free(o);
    int rc = run_command(cmd);
    free(cmd);
    return rc == 0 ? 0 : -1;
}

/* 检查是否可找到可用的 JDK（构建模组需要） */
static int has_jdk(void) {
#ifdef _WIN32
    return run_command("where java >nul 2>&1") == 0;
#else
    return run_command("command -v java >/dev/null 2>&1") == 0;
#endif
}

/* 检查是否有可用的 Gradle（PATH 探测，避免 gradle 报“不是命令”却返回 0） */
static int has_gradle(void) {
#ifdef _WIN32
    return run_command("where gradle >nul 2>&1") == 0;
#else
    return run_command("command -v gradle >/dev/null 2>&1") == 0;
#endif
}

static char *dp_base(void) {
    const char *base = getenv("LOCALAPPDATA");
    if (!base) base = getenv("APPDATA");
    return base ? path_join(base, "dp") : strdup("./.dp");
}

static char *tools_home(void) {
    char *b = dp_base();
    char *t = path_join(b, "tools");
    free(b);
    return t;
}

/* 解压归档到 dest（目录已存在） */
static int extract_archive(const char *archive, const char *dest) {
    char *z = strdup(archive), *d = strdup(dest);
    for (char *c = z; *c; c++) if (*c == '\\') *c = '/';
    for (char *c = d; *c; c++) if (*c == '\\') *c = '/';
    size_t n = strlen(z) + strlen(d) + 2048;
    char *cmd = (char*)malloc(n);
    int rc;
#ifdef _WIN32
    snprintf(cmd, n, "%s \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"",
             POWERSHELL, z, d);
#else
    if (ends_with(archive, ".zip"))
        snprintf(cmd, n, "unzip -o '%s' -d '%s'", z, d);
    else
        snprintf(cmd, n, "mkdir -p '%s' && tar -xzf '%s' -C '%s'", d, z, d);
#endif
    free(z); free(d);
    rc = run_command(cmd);
    free(cmd);
    return rc == 0 ? 0 : -1;
}

/* 返回 dir 下第一个子目录（已 malloc），无则 NULL */
static char *first_subdir(const char *dir) {
#ifdef _WIN32
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    char *res = NULL;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
            res = path_join(dir, fd.cFileName);
            break;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return res;
#else
    DIR *d = opendir(dir);
    if (!d) return NULL;
    struct dirent *e;
    char *res = NULL;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        char *p = path_join(dir, e->d_name);
        int isdir = dir_exists(p);
        if (isdir) { res = p; break; }
        free(p);
    }
    closedir(d);
    return res;
#endif
}

/* 下载并解压一个工具到 tools/<name>，返回其顶级目录（已 malloc）；失败返回 NULL。带 .home 缓存标记 */
static char *install_tool(const char *name, const char *url) {
    char *th = tools_home();
    char *dir = path_join(th, name);
    mkdir_p(dir);

    char *home_marker = path_join(dir, ".home");
    if (file_exists(home_marker)) {
        char *saved = read_file(home_marker, NULL);
        free(th); free(dir);
        if (saved) return saved;
    }

    printf("Downloading %s (this may take a while)...\n", name);
    char *archive = (char*)malloc(strlen(dir) + 16);
#ifdef _WIN32
    sprintf(archive, "%s.zip", dir);
#else
    sprintf(archive, "%s.pkg", dir);
#endif
    if (download(url, archive) != 0) {
        fprintf(stderr, "Error: failed to download %s. Check your network.\n", name);
        free(archive); free(home_marker); free(th); free(dir);
        return NULL;
    }
    if (extract_archive(archive, dir) != 0) {
        fprintf(stderr, "Error: failed to extract %s.\n", name);
        free(archive); free(home_marker); free(th); free(dir);
        return NULL;
    }
    remove(archive);
    free(archive);

    char *home = first_subdir(dir);
    if (!home) {
        fprintf(stderr, "Error: %s extracted but no subdirectory found.\n", name);
        free(home_marker); free(th); free(dir);
        return NULL;
    }
    write_file(home_marker, home, strlen(home));
    free(home_marker);
    free(th); free(dir);
    return home;
}

/* 确保有可用 JDK：优先系统，缺失则自动下载。返回需写入 JAVA_HOME 的路径（已 malloc）或 NULL（用系统） */
char *ensure_jdk(void) {
    if (has_jdk()) return NULL;
    return install_tool("jdk17", JDK17_URL);
}

/* 确保有可用 Gradle：优先系统，缺失则自动下载。返回 gradle 根目录（已 malloc）或 NULL（用系统） */
char *ensure_gradle(void) {
    if (has_gradle()) return NULL;
    return install_tool("gradle", GRADLE_URL);
}

static int install_forge(void) {
    char *dir = loader_dir("forge");
    mkdir_p(dir);
    const char *url = getenv("DP_FORGE_MDK_URL");
    if (!url || !*url) url = DEFAULT_FORGE_MDK;
    char *zip = path_join(dir, "mdk.zip");
    printf("Downloading Forge MDK...\n");
    if (download(url, zip) != 0) {
        fprintf(stderr,
                "Error: failed to download Forge MDK.\n"
                "  Check your network, or set DP_FORGE_MDK_URL.\n");
        free(zip); free(dir);
        return 1;
    }
    char *cmd = (char*)malloc(strlen(zip) + strlen(dir) + 1024);
    char *z = strdup(zip), *d = strdup(dir);
    for (char *c = z; *c; c++) if (*c == '\\') *c = '/';
    for (char *c = d; *c; c++) if (*c == '\\') *c = '/';
#ifdef _WIN32
    snprintf(cmd, strlen(zip) + strlen(dir) + 1024,
             "%s \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"", POWERSHELL, z, d);
#else
    snprintf(cmd, strlen(zip) + strlen(dir) + 1024, "%s '%s' -d '%s'", UNZIP, zip, dir);
#endif
    free(z); free(d);
    int rc = run_command(cmd);
    free(cmd);
    char *marker = path_join(dir, "installed");
    write_file(marker, "forge\n", 6);
    free(marker);
    free(zip);
    free(dir);
    if (rc != 0) {
        fprintf(stderr, "Warning: extraction failed; library may be incomplete.\n");
        return 1;
    }
    printf("Forge mod library installed.\n");
    return 0;
}

static int install_fabric(void) {
    if (!has_jdk()) {
        fprintf(stderr,
                "Error: no JDK found. Fabric mods need Java 17+ (run 'java -version').\n");
        return 1;
    }
    char *dir = loader_dir("fabric");
    mkdir_p(dir);
    char *marker = path_join(dir, "installed");
    write_file(marker, "fabric\n", 7);
    free(marker);
    free(dir);
    printf("Fabric mod library prepared (build deps fetched by Gradle Loom).\n");
    return 0;
}

static int install_neoforge(void) {
    if (!has_jdk()) {
        fprintf(stderr,
                "Error: no JDK found. NeoForge mods need Java 17+ (run 'java -version').\n");
        return 1;
    }
    char *dir = loader_dir("neoforge");
    mkdir_p(dir);
    char *marker = path_join(dir, "installed");
    write_file(marker, "neoforge\n", 9);
    free(marker);
    free(dir);
    printf("NeoForge mod library prepared (build deps fetched by Gradle).\n");
    return 0;
}

static int install_quilt(void) {
    if (!has_jdk()) {
        fprintf(stderr,
                "Error: no JDK found. Quilt mods need Java 17+ (run 'java -version').\n");
        return 1;
    }
    char *dir = loader_dir("quilt");
    mkdir_p(dir);
    char *marker = path_join(dir, "installed");
    write_file(marker, "quilt\n", 6);
    free(marker);
    free(dir);
    printf("Quilt mod library prepared (build deps fetched by Gradle Quilt Loom).\n");
    return 0;
}

int install_loader(const char *loader) {
    if (!loader_supported(loader)) {
        fprintf(stderr, "Error: unknown loader '%s'. Supported: %s.\n", loader, KNOWN_LOADERS);
        return 1;
    }
    if (is_installed(loader)) {
        printf("%s mod library is already installed.\n", loader);
        return 0;
    }
    if (strcmp(loader, "forge") == 0) return install_forge();
    if (strcmp(loader, "fabric") == 0) return install_fabric();
    if (strcmp(loader, "neoforge") == 0) return install_neoforge();
    if (strcmp(loader, "quilt") == 0) return install_quilt();
    fprintf(stderr,
            "Note: loader '%s' is recognized but not implemented yet.\n", loader);
    return 1;
}
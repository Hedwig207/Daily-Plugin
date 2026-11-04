#include "addon.h"
#include "install.h"
#include "package.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define ADDON_SEP '\\'
#else
#include <unistd.h>
#define ADDON_SEP '/'
#endif

int addon_parse_manifest(const char *text, DpAddon *a) {
    memset(a, 0, sizeof(*a));
    const char *p = text;
    while (p && *p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        char line[512];
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, p, len);
        line[len] = '\0';
        for (char *c = line; *c; c++) if (*c == '\r') *c = '\0';
        char *eq = strchr(line, '=');
        if (eq && line[0] != '#') {
            *eq = '\0';
            char *key = line;
            char *val = eq + 1;
            while (*key == ' ' || *key == '\t') key++;
            char *ke = key + strlen(key);
            while (ke > key && (ke[-1] == ' ' || ke[-1] == '\t')) *--ke = '\0';
            while (*val == ' ' || *val == '\t') val++;
            if (strcmp(key, "id") == 0) snprintf(a->id, sizeof(a->id), "%s", val);
            else if (strcmp(key, "name") == 0) snprintf(a->name, sizeof(a->name), "%s", val);
            else if (strcmp(key, "version") == 0) snprintf(a->version, sizeof(a->version), "%s", val);
            else if (strcmp(key, "type") == 0) snprintf(a->type, sizeof(a->type), "%s", val);
            else if (strcmp(key, "loader") == 0) snprintf(a->loader, sizeof(a->loader), "%s", val);
            else if (strcmp(key, "description") == 0) snprintf(a->description, sizeof(a->description), "%s", val);
        }
        if (!nl) break;
        p = nl + 1;
    }
    return (a->id[0]) ? 0 : -1;
}

char *addon_home(void) {
    const char *base = getenv("LOCALAPPDATA");
    if (!base) base = getenv("APPDATA");
    char *home;
    if (base) home = path_join(path_join(base, "dp"), "addons");
    else home = strdup(".dp/addons");
    return home;
}

static char *addon_dir_for(const char *id) {
    char *home = addon_home();
    char *dir = path_join(home, id);
    free(home);
    return dir;
}

int addon_pack_dir(const char *dir, const char *out) {
    return pack_dir(dir, out);
}

/* 复制整棵树（跳过 .gradle），源和目标都加尾部斜杠以保证内容是合并而非建子目录 */
static int copy_tree_into(const char *src, const char *dst) {
    size_t n = strlen(src) + strlen(dst) + 1100;
    char *cmd = (char*)malloc(n);
    char *s = strdup(src), *d = strdup(dst);
    for (char *c = s; *c; c++) if (*c == '\\') *c = '/';
    for (char *c = d; *c; c++) if (*c == '\\') *c = '/';
#ifdef _WIN32
    snprintf(cmd, n, "powershell -NoProfile -ExecutionPolicy Bypass -Command "
             "Copy-Item -Recurse -Force '%s/*' '%s'", s, d);
#else
    snprintf(cmd, n, "cp -r '%s/. '", s);
    /* 简化 POSIX 不支持 *；改用 mkdir+copytree 由上层保证 */
    snprintf(cmd, n, "mkdir -p '%s' && cp -r '%s'/. '%s'", d, s, d);
#endif
    free(s); free(d);
    int rc = run_command(cmd);
    free(cmd);
    return rc == 0 ? 0 : -1;
}

/* 递归删除目录 */
static int remove_tree(const char *dir) {
    size_t n = strlen(dir) + 256;
    char *cmd = (char*)malloc(n);
    char *d = strdup(dir);
    for (char *c = d; *c; c++) if (*c == '\\') *c = '/';
#ifdef _WIN32
    snprintf(cmd, n, "powershell -NoProfile -ExecutionPolicy Bypass -Command "
             "Remove-Item -Recurse -Force '%s'", d);
#else
    snprintf(cmd, n, "rm -rf '%s'", d);
#endif
    free(d);
    int rc = run_command(cmd);
    free(cmd);
    return rc;
}

int addon_install(const char *dplug) {
    /* 1. 解压到临时目录 */
    static int seq = 0;
    seq++;
    char tmpdir[512];
#ifdef _WIN32
    snprintf(tmpdir, sizeof(tmpdir), "%s_dp_addon_%d", dplug, seq);
#else
    snprintf(tmpdir, sizeof(tmpdir), "%s_dp_addon_%d", dplug, seq);
#endif
    remove_tree(tmpdir);
    if (unpack_plug(dplug, tmpdir) != 0) {
        fprintf(stderr, "Error: failed to extract addon: %s\n", dplug);
        return -1;
    }

    /* 2. 读 manifest */
    char *manifest_path = path_join(tmpdir, ADDON_MANIFEST_NAME);
    char *text = read_file(manifest_path, NULL);
    if (!text) {
        fprintf(stderr, "Error: no " ADDON_MANIFEST_NAME " in addon package.\n");
        remove_tree(tmpdir);
        free(manifest_path);
        return -1;
    }
    DpAddon a;
    if (addon_parse_manifest(text, &a) != 0) {
        fprintf(stderr, "Error: invalid addon manifest.\n");
        free(text); remove_tree(tmpdir); free(manifest_path);
        return -1;
    }
    free(text);

    /* 3. 拷贝整个附加包到 addon_home/<id> */
    char *dest = addon_dir_for(a.id);
    /* 保留当前实现，先删除旧的再整体拷贝（templates 会一起进来） */
    remove_tree(dest);
    mkdir_p(dest);
    if (copy_tree_into(tmpdir, dest) != 0) {
        fprintf(stderr, "Error: failed to copy addon into %s\n", dest);
        free(dest); remove_tree(tmpdir); free(manifest_path);
        return -1;
    }
    printf("Addon '%s' v%s installed.\n", a.id, a.version[0] ? a.version : "?");
    printf("  location: %s\n", dest);

    /* 4. 若为 loader/tool 附加包，把其 templates 下的每个加载器模板
     *    分别合并到全局模板缓存。这样单附加包可携带整套模板库。 */
    if (strcmp(a.type, "loader") == 0 || strcmp(a.type, "tool") == 0) {
        char *extra = path_join(dest, "templates");
        if (dir_exists(extra)) {
            char *tpl_root = resolve_templates_dir();
            if (tpl_root) {
#ifdef _WIN32
                char pat[1024];
                snprintf(pat, sizeof(pat), "%s\\*", extra);
                WIN32_FIND_DATAA fd;
                HANDLE h = FindFirstFileA(pat, &fd);
                if (h != INVALID_HANDLE_VALUE) {
                    int n = 0;
                    do {
                        if (fd.cFileName[0] == '.') continue;
                        char *sub = path_join(extra, fd.cFileName);
                        char *dst_tpl = path_join(tpl_root, fd.cFileName);
                        mkdir_p(dst_tpl);
                        if (copy_tree_into(sub, dst_tpl) == 0) { printf("  loaded template '%s'.\n", fd.cFileName); n++; }
                        free(sub); free(dst_tpl);
                    } while (FindNextFileA(h, &fd));
                    FindClose(h);
                    if (n == 0 && a.loader[0]) {
                        /* 兼容旧 manifest：兜底仅合并单 loader 目录本身 */
                        char *loader_tpl = path_join(tpl_root, a.loader);
                        mkdir_p(loader_tpl);
                        copy_tree_into(extra, loader_tpl);
                        printf("  loaded templates for loader '%s'.\n", a.loader);
                        free(loader_tpl);
                    }
                }
#else
                fprintf(stderr, "  note: POSIX template merge for multi-loader addon not implemented.\n");
                if (a.loader[0]) {
                    char *loader_tpl = path_join(tpl_root, a.loader);
                    mkdir_p(loader_tpl);
                    copy_tree_into(extra, loader_tpl);
                    printf("  loaded templates for loader '%s'.\n", a.loader);
                    free(loader_tpl);
                }
#endif
            }
            free(tpl_root);
        }
        free(extra);
    }

    free(manifest_path);
    free(dest);
    remove_tree(tmpdir);
    return 0;
}

int addon_uninstall(const char *id) {
    char *dest = addon_dir_for(id);
    char *node = path_join(dest, ADDON_MANIFEST_NAME);
    if (!file_exists(node)) {
        free(dest); free(node);
        return -1;
    }
    /* read manifest to know if this is a loader addon (so we can drop its templates) */
    char *text = read_file(node, NULL);
    if (text) {
        DpAddon a;
        if (addon_parse_manifest(text, &a) == 0 && strcmp(a.type, "loader") == 0 && a.loader[0]) {
            char *tpl_root = resolve_templates_dir();
            if (tpl_root) {
                char *loader_tpl = path_join(tpl_root, a.loader);
                if (dir_exists(loader_tpl)) remove_tree(loader_tpl);
                free(loader_tpl);
            }
            free(tpl_root);
        }
        free(text);
    }
    int rc = remove_tree(dest);
    free(dest); free(node);
    if (rc == 0) printf("Addon '%s' uninstalled.\n", id);
    return rc == 0 ? 0 : -1;
}

int addon_list(void) {
    char *home = addon_home();
    if (!dir_exists(home)) {
        printf("No addons installed.\n");
        free(home);
        return 0;
    }
    printf("Installed addons:\n");
#ifdef _WIN32
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s\\*", home);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) { free(home); return 0; }
    int count = 0;
    do {
        if (fd.cFileName[0] == '.') continue;
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        char *m = path_join(path_join(home, fd.cFileName), ADDON_MANIFEST_NAME);
        char *t = read_file(m, NULL);
        if (t) {
            DpAddon a;
            if (addon_parse_manifest(t, &a) == 0) {
                printf("  [%s] %s v%s", a.id, a.name[0] ? a.name : a.id, a.version);
                if (strcmp(a.type, "loader") == 0 && a.loader[0])
                    printf("  (loader: %s)", a.loader);
                if (a.description[0]) printf("  - %s", a.description);
                printf("\n");
                count++;
            }
            free(t);
        }
        free(m);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    if (!count) printf("  (none)\n");
#else
    printf("  (POSIX listing: scan %s)\n", home);
#endif
    free(home);
    return 0;
}

void list_all_loaders(void) {
    static const char *builtin[] = {"forge", "fabric", "neoforge", "quilt", "liteloader", NULL};
    printf("Known loaders:\n");
    for (int i = 0; builtin[i]; i++)
        printf("  [%s] %s\n", loader_usable(builtin[i]) ? "x" : " ", builtin[i]);
    /* addon-provided loaders */
    char *home = addon_home();
#ifdef _WIN32
    if (dir_exists(home)) {
        char pattern[1024];
        snprintf(pattern, sizeof(pattern), "%s\\*", home);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.cFileName[0] == '.') continue;
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                char *m = path_join(path_join(home, fd.cFileName), ADDON_MANIFEST_NAME);
                char *t = read_file(m, NULL);
                if (t) {
                    DpAddon a;
                    if (addon_parse_manifest(t, &a) == 0 && strcmp(a.type, "loader") == 0 && a.loader[0]) {
                        if (!loader_supported(a.loader))
                            printf("  [%s] %s  (addon)\n", loader_usable(a.loader) ? "x" : " ", a.loader);
                    }
                    free(t);
                }
                free(m);
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
#endif
    free(home);
}

int loader_usable(const char *loader) {
    if (loader_supported(loader)) return 1;
    char *home = addon_home();
    int found = 0;
#ifdef _WIN32
    if (dir_exists(home)) {
        char pattern[1024];
        snprintf(pattern, sizeof(pattern), "%s\\*", home);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.cFileName[0] == '.') continue;
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                /* 判据 A：manifest 声明的 loader */
                char *m = path_join(path_join(home, fd.cFileName), ADDON_MANIFEST_NAME);
                char *t = read_file(m, NULL);
                if (t) {
                    DpAddon a;
                    if (addon_parse_manifest(t, &a) == 0 && strcmp(a.type, "loader") == 0 &&
                        strcmp(a.loader, loader) == 0) found = 1;
                    free(t);
                }
                free(m);
                /* 判据 B：附加包 templates/ 目录下含该加载器模板 */
                if (!found) {
                    char *tp = path_join(path_join(home, fd.cFileName), "templates");
                    char *cand = path_join(tp, loader);
                    if (dir_exists(cand)) found = 1;
                    free(cand); free(tp);
                }
            } while (!found && FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
#endif
    free(home);
    return found;
}

char *loader_extra_templates(const char *loader) {
    char *home = addon_home();
    char *out = NULL;
#ifdef _WIN32
    if (dir_exists(home)) {
        char pattern[1024];
        snprintf(pattern, sizeof(pattern), "%s\\*", home);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.cFileName[0] == '.') continue;
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                char *m = path_join(path_join(home, fd.cFileName), ADDON_MANIFEST_NAME);
                char *t = read_file(m, NULL);
                if (t) {
                    DpAddon a;
                    if (addon_parse_manifest(t, &a) == 0 && strcmp(a.type, "loader") == 0 &&
                        strcmp(a.loader, loader) == 0) {
                        free(t); free(m);
                        out = path_join(home, a.id);
                        char *tp = path_join(out, "templates");
                        free(out);
                        out = tp;
                        if (!dir_exists(out)) { free(out); out = NULL; }
                        break;
                    }
                    free(t);
                }
                free(m);
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
#endif
    free(home);
    return out;
}

/* 生成一个新的 .dplug 项目骨架：
 *   <dir>/manifest.dplug      - 附加包清单
 *   <dir>/README.dplug.md     - 制作说明
 *   <dir>/templates/mymod/... - 演示模板（用户可替换成自己的）
 * 之后用户可加入多个 templates/<loader>/ 目录，再用 dp addon pack 打包。 */
int addon_new(const char *dir, const char *id, const char *name,
              const char *type, const char *description) {
    if (dir_exists(dir)) {
        fprintf(stderr, "Error: directory '%s' already exists.\n", dir);
        return -1;
    }
    mkdir_p(dir);

    const char *t = type && type[0] ? type : "loader";
    const char *nm = name && name[0] ? name : id;
    const char *ds = description && description[0] ? description : "Addon powered by DailyPlugin";
    const char *loader = (strcmp(t, "loader") == 0) ? id : "";

    char *m = path_join(dir, ADDON_MANIFEST_NAME);
    char buf[1024];
    int n = snprintf(buf, sizeof(buf),
        "id=%s\n"
        "name=%s\n"
        "version=0.1.0\n"
        "type=%s\n"
        "loader=%s\n"
        "description=%s\n",
        id, nm, t, loader, ds);
    write_file(m, buf, (size_t)n);
    free(m);

    /* 演示模板：一个最小 loader 模板占位 */
    char *tpl = path_join(path_join(dir, "templates"), "example");
    mkdir_p(tpl);
    char *f = path_join(tpl, "README.txt");
    const char *s =
        "This is a demo template.\n"
        "Replace or add your own templates/<loader>/ directories, then:\n"
        "  dp addon pack THIS_DIR --output myaddon.dplug\n";
    write_file(f, s, strlen(s));
    free(f);
    free(tpl);

    /* 制作说明 */
    char *r = path_join(dir, "README.dplug.md");
    const char *md =
        "# How to build a DailyPlugin addon (.dplug)\n"
        "\n"
        "A `.dplug` is a zip archive containing:\n"
        "\n"
        "    manifest.dplug            key=value metadata (REQUIRED)\n"
        "    templates/<loader>/...    optional mod-loader templates\n"
        "\n"
        "## manifest fields\n"
        "\n"
        "    id=<unique id>\n"
        "    name=<display name>\n"
        "    version=<x.y.z>\n"
        "    type=loader|tool|extension\n"
        "    loader=<loader id>   (only when type=loader)\n"
        "    description=<one-line summary>\n"
        "\n"
        "## workflow\n"
        "\n"
        "    dp addon new myaddon --name \"My Addon\" --type loader\n"
        "    # ... add your templates/<loader>/ files ...\n"
        "    dp addon pack myaddon --output myaddon.dplug\n"
        "    dp addon install myaddon.dplug\n"
        "    dp addon list\n"
        "\n"
        "When installed, `templates/*` are merged into the loader template cache,\n"
        "so any loader with a template becomes usable by `dp new`.\n";
    write_file(r, md, strlen(md));
    free(r);

    printf("Generated addon skeleton in '%s'\n", dir);
    printf("  manifest.dplug\n  templates/example/\n  README.dplug.md\n");
    printf("Next: add templates, then 'dp addon pack %s --output x.dplug'.\n", dir);
    return 0;
}
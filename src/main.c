#include "util.h"
#include "template.h"
#include "scaffold.h"
#include "package.h"
#include "install.h"
#include "addon.h"
#include "update.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

static const char *VERSION = "0.3.0";

static const char *TAGLINE =
    "DailyPlugin (dp) - quickly & conveniently create plugins / mods.\n";

static const char *GENERAL_HELP =
    "\nUsage: dp <command> [arguments]\n"
    "\n"
    "Initial commands (always available):\n"
    "  help [options]      Show help. Use '-mcmod' to view MC commands.\n"
    "  install <loader>    Prepare a mod-loader library (forge, fabric, ...).\n"
    "  uninstall <loader>  Remove an installed mod-loader library.\n"
    "  list                List known loaders and their install status.\n"
    "  pack <dir>          Bundle a project directory into a '.plug' archive.\n"
    "  unpack <file.plug>  Restore a '.plug' archive into source files.\n"
    "  addon <sub>         Manage DailyPlugin addon packages (.dplug).\n"
    "                      pack <dir>|install <file>|uninstall <id>|list\n"
    "  update              Check for & apply updates from GitHub Releases.\n"
    "  version             Print version.\n"
    "  help -all           Show help for every aspect.\n"
    "\n"
    "Options (only after selecting a platform):\n"
    "  -mcmod              Select the Minecraft platform.\n"
    "  --loader <name>     Mod loader: forge, fabric, neoforge, quilt. Extensible.\n"
    "  --output <path>     Destination path for pack / unpack / build output.\n"
    "\n"
    "Notes:\n"
    "  Loader commands (new / build) need 'dp install <loader>' first.\n"
    "  Use 'dp help -mcmod' to see Minecraft-specific commands.\n";

static const char *MC_HELP =
    "\nDailyPlugin - Minecraft mod commands (require 'dp install <loader>'):\n"
    "\n"
    "  new <name>          Scaffold a new mod project for the selected loader.\n"
    "  build <dir|.plug>   Restore then invoke Gradle to produce a mod jar.\n"
    "  help -mcmod         Show this MC help.\n"
    "\n"
    "Loader (--loader): forge, fabric, neoforge, quilt. Future: liteloader.\n"
    "\n"
    "MC options:\n"
    "  --package <pkg>     Java package (default com.example.<id>).\n"
    "  --id <modid>        Mod ID (default lowercased name).\n"
    "  --mc <ver>          Target Minecraft version (default 1.20.1).\n"
    "  --version <v>       Mod version (default 1.0.0).\n"
    "  --author <a>        Author (default Owlbyte).\n"
    "  --license <l>       License (default MIT).\n"
    "  --desc <d>          Mod description.\n"
    "\n"
    "Forge options:\n"
    "  --forge <ver>       Forge version (default 47.2.0).\n"
    "\n"
    "Fabric options:\n"
    "  --loom <ver>        Fabric Loom version (default 1.6.12).\n"
    "  --yarn <ver>        Fabric Yarn mappings (default 1.20.1+build.10).\n"
    "  --fabric-loader <v> Fabric Loader version (default 0.15.3).\n"
    "  --fabric-api <v>    Fabric API version (default 0.91.1+1.20.1).\n"
    "\n"
    "Quilt options:\n"
    "  --qloom <ver>       Quilt Loom version (default 1.6.12).\n"
    "  --yarn <ver>        Yarn mappings (default 1.20.1+build.10).\n"
    "  --qloader <v>       Quilt Loader version (default 0.26.2).\n"
    "  --qfapi <v>         Quilted Fabric API version (default 7.1.0+1.20.1).\n"
    "  --qsl <v>           Quilt Standard Libraries version (default 7.1.0+1.20.1).\n"
    "\n"
    "NeoForge options:\n"
    "  --neo <ver>         NeoForge version (default 47.1.106).\n"
    "  --neo-gradle <v>    NeoForge Gradle plugin version (default 7.0.80).\n";

static const char *EXAMPLES =
    "\nExamples:\n"
    "  dp install quilt\n"
    "  dp new mymod -mcmod --loader quilt --package com.owlbyte.mymod\n"
    "  dp new mymod -mcmod --loader neoforge\n"
    "  dp new mymod -mcmod --loader forge\n"
    "  dp new mymod -mcmod --loader fabric --package com.owlbyte.mymod\n"
    "  dp help -mcmod\n"
    "  dp pack mymod --output mymod.plug\n"
    "  dp unpack mymod.plug --output restored\n";

static int has_flag(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], flag) == 0) return 1;
    return 0;
}

static const char *opt_value(int argc, char **argv, const char *key) {
    for (int i = 0; i + 1 < argc; i++)
        if (strcmp(argv[i], key) == 0) return argv[i + 1];
    return NULL;
}

/* 解析 --loader，默认 forge */
static const char *pick_loader(int argc, char **argv) {
    const char *l = opt_value(argc, argv, "--loader");
    return (l && *l) ? l : "forge";
}

static int cmd_install(int argc, char **argv) {
    const char *loader = NULL;
    for (int i = 2; i < argc; i++)
        if (argv[i][0] != '-') { loader = argv[i]; break; }
    if (!loader) {
        fprintf(stderr, "Usage: dp install <loader>   (%s)\n", KNOWN_LOADERS);
        return 1;
    }
    return install_loader(loader);
}

static int cmd_uninstall(int argc, char **argv) {
    const char *loader = NULL;
    for (int i = 2; i < argc; i++)
        if (argv[i][0] != '-') { loader = argv[i]; break; }
    if (!loader) {
        fprintf(stderr, "Usage: dp uninstall <loader>\n");
        return 1;
    }
    if (!loader_supported(loader)) {
        fprintf(stderr, "Error: unknown loader '%s'.\n", loader);
        return 1;
    }
    char *dir = loader_dir(loader);
    int had = is_installed(loader);
    char *cmd = (char*)malloc(strlen(dir) + 2048);
#ifdef _WIN32
    {
        char *p = strdup(dir);
        for (char *c = p; *c; c++) if (*c == '\\') *c = '/';
        snprintf(cmd, strlen(dir) + 2048,
                 "%s \"Remove-Item -Recurse -Force '%s'\"", "powershell -NoProfile -Command", p);
        free(p);
    }
#else
    snprintf(cmd, strlen(dir) + 2048, "rm -rf '%s'", dir);
#endif
    run_command(cmd);
    free(cmd);
    free(dir);
    printf(had ? "%s mod library uninstalled.\n" : "Not installed; nothing to remove.\n",
           loader);
    return 0;
}

static int cmd_new(int argc, char **argv) {
    if (!has_flag(argc, argv, "-mcmod")) {
        fprintf(stderr, "Error: select a platform. Use 'dp new <name> -mcmod --loader <l>'.\n");
        fprintf(stderr, "  Tip: run 'dp help -mcmod'.\n");
        return 1;
    }
    const char *loader = pick_loader(argc, argv);
    if (!loader_usable(loader)) {
        fprintf(stderr, "Error: unknown loader '%s'. Supported: %s (plus addons).\n", loader, KNOWN_LOADERS);
        return 1;
    }
    if (!loader_supported(loader) && !is_installed(loader)) {
        /* 附加包加载器：只要 loader_usable 即可（模板已由其附加包合并提供） */
        printf("== Using addon-provided loader '%s' ==\n", loader);
    } else if (!is_installed(loader)) {
        fprintf(stderr,
                "Error: %s mod library is not installed.\n"
                "  Run 'dp install %s' first.\n", loader, loader);
        return 1;
    }

    const char *name = NULL;
    for (int i = 2; i < argc; i++)
        if (argv[i][0] != '-' && !name) { name = argv[i]; break; }
    if (!name) { fprintf(stderr, "Usage: dp new <name> -mcmod --loader %s\n", loader); return 1; }
    if (dir_exists(name)) { fprintf(stderr, "Error: directory '%s' already exists.\n", name); return 1; }

    const char *mod_id = opt_value(argc, argv, "--id");
    const char *pkg = opt_value(argc, argv, "--package");
    const char *mc = opt_value(argc, argv, "--mc");
    const char *forge = opt_value(argc, argv, "--forge");
    const char *loom = opt_value(argc, argv, "--loom");
    const char *yarn = opt_value(argc, argv, "--yarn");
    const char *floader = opt_value(argc, argv, "--fabric-loader");
    const char *fapi = opt_value(argc, argv, "--fabric-api");
    const char *qloom = opt_value(argc, argv, "--qloom");
    const char *qloader = opt_value(argc, argv, "--qloader");
    const char *qfapi = opt_value(argc, argv, "--qfapi");
    const char *qsl = opt_value(argc, argv, "--qsl");
    const char *neo = opt_value(argc, argv, "--neo");
    const char *neo_gradle = opt_value(argc, argv, "--neo-gradle");
    const char *version = opt_value(argc, argv, "--version");
    const char *author = opt_value(argc, argv, "--author");
    const char *license = opt_value(argc, argv, "--license");
    const char *desc = opt_value(argc, argv, "--desc");
    if (!mc) mc = "1.20.1";
    if (!forge) forge = "47.2.0";
    if (!loom) loom = "1.6.12";
    if (!yarn) yarn = "1.20.1+build.10";
    if (!floader) floader = "0.15.3";
    if (!fapi) fapi = "0.91.1+1.20.1";
    if (!qloom) qloom = "1.6.12";
    if (!qloader) qloader = "0.26.2";
    if (!qfapi) qfapi = "7.1.0+1.20.1";
    if (!qsl) qsl = "7.1.0+1.20.1";
    if (!neo) neo = "47.1.106";
    if (!neo_gradle) neo_gradle = "7.0.80";
    if (!version) version = "1.0.0";
    if (!author) author = "Owlbyte";
    if (!license) license = "MIT";
    if (!desc) desc = "";

    char id[256];
    if (mod_id) snprintf(id, sizeof(id), "%s", mod_id);
    else {
        snprintf(id, sizeof(id), "%s", name);
        for (char *c = id; *c; c++) if (*c >= 'A' && *c <= 'Z') *c = *c - 'A' + 'a';
    }
    char cls[256];
    {   /* camel-case class name */
        int up = 1, k = 0;
        for (const char *c = id; *c; c++) {
            if (*c == '-' || *c == '_') { up = 1; continue; }
            cls[k++] = up ? (char)(*c >= 'a' && *c <= 'z' ? *c - 'a' + 'A' : *c) : *c;
            up = 0;
        }
        if (k == 0) { snprintf(cls, sizeof(cls), "%s", id); }
        else cls[k] = '\0';
    }
    char package_path[512];
    if (pkg) snprintf(package_path, sizeof(package_path), "%s", pkg);
    else snprintf(package_path, sizeof(package_path), "com.example.%s", id);
    char group[256];
    {
        snprintf(group, sizeof(group), "%s", package_path);
        char *d2 = strrchr(group, '.');
        if (d2) *d2 = 0;
    }

    TplVars v;
    tpl_init(&v);
    tpl_set(&v, "mod_id", id);
    tpl_set(&v, "mod_name", name);
    tpl_set(&v, "mod_class", cls);
    tpl_set(&v, "package", package_path);
    tpl_set(&v, "group", group);
    tpl_set(&v, "mc_version", mc);
    tpl_set(&v, "version", version);
    tpl_set(&v, "author", author);
    tpl_set(&v, "license", license);
    tpl_set(&v, "description", desc);
    tpl_set(&v, "forge_version", forge);
    tpl_set(&v, "loom_version", loom);
    tpl_set(&v, "yarn_mappings", yarn);
    tpl_set(&v, "loader_version", floader);
    tpl_set(&v, "fabric_version", fapi);
    tpl_set(&v, "qloom_version", qloom);
    tpl_set(&v, "qloader_version", qloader);
    tpl_set(&v, "qfapi_version", qfapi);
    tpl_set(&v, "qsl_version", qsl);
    tpl_set(&v, "neo_version", neo);
    tpl_set(&v, "neo_gradle_version", neo_gradle);

    char *tpl_root = resolve_templates_dir();
    if (!tpl_root) {
        fprintf(stderr, "Error: templates directory not found. Set DAILY_PLUGIN_TEMPLATES.\n");
        tpl_free(&v);
        return 1;
    }
    char *tpl_dir = path_join(tpl_root, loader);

    printf("== Scaffolding %s mod: %s ==\n", loader, name);
    printf("  package: %s\n", package_path);
    printf("  main class: %s\n", cls);
    printf("  MC: %s\n", mc);

    int rc = scaffold_mod(tpl_dir, name, &v, package_path, cls);
    free(tpl_root);
    free(tpl_dir);
    tpl_free(&v);
    if (rc != 0) {
        fprintf(stderr, "Scaffolding failed.\n");
        return 1;
    }
    printf("Done. Project created in: %s\n", name);
    printf("Next: dp pack %s   (or cd %s && gradle build)\n", name, name);
    return 0;
}

static int cmd_pack(int argc, char **argv) {
    const char *target = NULL;
    for (int i = 2; i < argc; i++)
        if (argv[i][0] != '-' && !target) { target = argv[i]; break; }
    if (!target) { fprintf(stderr, "Usage: dp pack <dir>\n"); return 1; }
    const char *out_v = opt_value(argc, argv, "--output");
    char *out = out_v ? strdup(out_v) : NULL;
    if (!out) {
        out = (char*)malloc(strlen(target) + 6);
        sprintf(out, "%s.plug", target);
    }
    if (pack_dir(target, out) != 0) { fprintf(stderr, "Pack failed.\n"); free(out); return 1; }
    printf("Done: %s\n", out);
    free(out);
    return 0;
}

static int cmd_unpack(int argc, char **argv) {
    const char *target = NULL;
    for (int i = 2; i < argc; i++)
        if (argv[i][0] != '-' && !target) { target = argv[i]; break; }
    if (!target) { fprintf(stderr, "Usage: dp unpack <file.plug>\n"); return 1; }
    const char *out = opt_value(argc, argv, "--output");
    if (!out) out = "restored";
    if (unpack_plug(target, out) != 0) { fprintf(stderr, "Unpack failed.\n"); return 1; }
    printf("Done: restored to %s\n", out);
    return 0;
}

static int cmd_build(int argc, char **argv) {
    const char *loader = pick_loader(argc, argv);
    if (!loader_supported(loader)) {
        fprintf(stderr, "Error: unknown loader '%s'.\n", loader);
        return 1;
    }
    if (!has_flag(argc, argv, "-mcmod") || !is_installed(loader)) {
        fprintf(stderr,
                "Error: build needs the %s library. Run 'dp install %s' and re-add '-mcmod'.\n",
                loader, loader);
        return 1;
    }
    const char *target = NULL;
    for (int i = 2; i < argc; i++)
        if (argv[i][0] != '-' && !target) { target = argv[i]; break; }
    if (!target) { fprintf(stderr, "Usage: dp build <dir | file.plug> -mcmod --loader %s\n", loader); return 1; }

    const char *work;
    char buf[512];
    if (ends_with(target, ".plug")) {
        const char *out_v = opt_value(argc, argv, "--output");
        work = out_v ? out_v : "build_work";
        if (unpack_plug(target, work) != 0) { fprintf(stderr, "Unpack failed.\n"); return 1; }
        snprintf(buf, sizeof(buf), "%s", work);
    } else {
        work = target;
    }

    /* 自动工具链：确保 JDK / Gradle 可用（缺失则下载，带缓存） */
    printf("== Preparing toolchain (%s) ==\n", loader);
    char *jdk = ensure_jdk();
    char *gradle = ensure_gradle();
    if (jdk) printf("  use bundled JDK: %s\n", jdk);
    else     printf("  use system JDK\n");
    if (gradle) printf("  use bundled Gradle: %s\n", gradle);
    else        printf("  use system Gradle\n");

    size_t n = strlen(work) * 2 + strlen(loader) + 4096;
    char *cmdline = (char*)malloc(n);
#ifdef _WIN32
    {
        /* 通过 cmd 临时设置 JAVA_HOME 与 PATH，再调用 gradle */
        SB b;
        sb_init(&b);
        sb_append_cstr(&b, "cd /d \"");
        sb_append_cstr(&b, work);
        sb_append_cstr(&b, "\" && ");
        if (jdk) {
            sb_append_cstr(&b, "set \"JAVA_HOME=");
            sb_append_cstr(&b, jdk);
            sb_append_cstr(&b, "\" && set \"PATH=");
            sb_append_cstr(&b, jdk);
            sb_append_cstr(&b, "\\bin;%PATH%\" && ");
        }
        if (gradle) {
            sb_append_cstr(&b, "\"");
            sb_append_cstr(&b, gradle);
            sb_append_cstr(&b, "\\bin\\gradle.bat\" ");
        } else {
            sb_append_cstr(&b, "gradle ");
        }
        sb_append_cstr(&b, "build");
        free(cmdline);
        cmdline = sb_cstr(&b);
    }
#else
    snprintf(cmdline, n, "cd '%s' && ./gradlew build", work);
#endif
    free(jdk);
    free(gradle);
    printf("== Building %s mod (%s) ==\n", loader, work);
    int rc = run_command(cmdline);
    free(cmdline);
    return rc == 0 ? 0 : 1;
}

/**** 附加包 (.dplug) 命令：dp addon <sub> ****/
static int cmd_addon(int argc, char **argv) {
    const char *sub = NULL;
    for (int i = 2; i < argc; i++)
        if (argv[i][0] != '-') { sub = argv[i]; break; }
    if (!sub) {
        fprintf(stderr, "Usage: dp addon <pack|install|uninstall|list> ...\n");
        return 1;
    }
    if (strcmp(sub, "list") == 0) return addon_list();

    const char *target = NULL;
    for (int i = 3; i < argc; i++)
        if (argv[i][0] != '-') { target = argv[i]; break; }

    if (strcmp(sub, "pack") == 0) {
        if (!target) { fprintf(stderr, "Usage: dp addon pack <dir> [--output file.dplug]\n"); return 1; }
        const char *out = opt_value(argc, argv, "--output");
        char outbuf[512];
        if (!out) snprintf(outbuf, sizeof(outbuf), "%s.dplug", target), out = outbuf;
        if (addon_pack_dir(target, out) != 0) { fprintf(stderr, "Pack failed.\n"); return 1; }
        printf("Done: %s\n", out);
        return 0;
    }
    if (strcmp(sub, "install") == 0) {
        if (!target) { fprintf(stderr, "Usage: dp addon install <file.dplug>\n"); return 1; }
        return addon_install(target) == 0 ? 0 : 1;
    }
    if (strcmp(sub, "uninstall") == 0) {
        if (!target) { fprintf(stderr, "Usage: dp addon uninstall <id>\n"); return 1; }
        if (addon_uninstall(target) != 0) {
            fprintf(stderr, "Error: addon '%s' not found.\n", target);
            return 1;
        }
        return 0;
    }
    fprintf(stderr, "Unknown addon subcommand: %s\n", sub);
    return 1;
}

/**** 自动更新命令：dp update ****/
static int cmd_update(int argc, char **argv) {
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) snprintf(cwd, sizeof(cwd), ".");
    return self_update(cwd) < 0 ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        printf("%s", TAGLINE);
        if (argc >= 2 && has_flag(argc, argv, "-mcmod"))
            printf("%s%s", MC_HELP, EXAMPLES);
        else if (argc >= 2 && has_flag(argc, argv, "-all"))
            printf("%s%s", GENERAL_HELP, EXAMPLES);
        else {
            printf("%s", GENERAL_HELP);
            if (is_installed("forge") || is_installed("fabric")) printf("%s", MC_HELP);
            printf("%s", EXAMPLES);
        }
        return 0;
    }

    const char *cmd = argv[1];
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0) {
        printf("DailyPlugin %s\n", VERSION);
        return 0;
    }
    if (strcmp(cmd, "install") == 0) return cmd_install(argc, argv);
    if (strcmp(cmd, "uninstall") == 0) return cmd_uninstall(argc, argv);
    if (strcmp(cmd, "list") == 0) { list_all_loaders(); return 0; }
    if (strcmp(cmd, "addon") == 0 || strcmp(cmd, "plugin") == 0) return cmd_addon(argc, argv);
    if (strcmp(cmd, "update") == 0) return cmd_update(argc, argv);
    if (strcmp(cmd, "new") == 0) return cmd_new(argc, argv);
    if (strcmp(cmd, "pack") == 0) return cmd_pack(argc, argv);
    if (strcmp(cmd, "unpack") == 0) return cmd_unpack(argc, argv);
    if (strcmp(cmd, "build") == 0) return cmd_build(argc, argv);

    fprintf(stderr, "Unknown command: %s (run 'dp help')\n", cmd);
    return 1;
}
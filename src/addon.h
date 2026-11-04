#ifndef DAILYPLUGIN_ADDON_H
#define DAILYPLUGIN_ADDON_H

/* ===== DailyPlugin 附加包 (.dplug) =====
 * .dplug 本质是 zip 归档，内含：
 *   manifest.dplug    - 纯文本 key=value 清单
 *   templates/<loader>/...  - 可选：模板，安装后合并进模板缓存（用于扩展新加载器）
 *
 * manifest.dplug 字段：
 *   id=myaddon
 *   name=My Addon
 *   version=1.0.0
 *   type=loader|tool|extension   (loader 表示新增/扩展一个 mod 加载器)
 *   loader=dragon                (type=loader 时：该附加包对应的加载器 id)
 *   description=...
 *
 * 附加包安装到 %LOCALAPPDATA%\dp\addons\<id>\（跨平台 fallback ~/.dp/addons/<id>）
 */

typedef struct {
    char id[128];
    char name[256];
    char version[64];
    char type[32];       /* loader / tool / extension */
    char loader[128];    /* type=loader 时有效 */
    char description[512];
} DpAddon;

/* 附加包的 manifest 路径约定（用于 pack 时识别） */
#define ADDON_MANIFEST_NAME "manifest.dplug"

/* 解析 manifest 文本到 addon。成功返回 0。 */
int addon_parse_manifest(const char *text, DpAddon *a);

/* 列出已安装附加包的根目录（%LOCALAPPDATA%\dp\addons）。需 free。 */
char *addon_home(void);

/* 将某目录打包为 .dplug（zip 归档）。 */
int addon_pack_dir(const char *dir, const char *out);

/* 安装一个 .dplug：解压到 addon_home/<id>，若含 templates 则合并到模板缓存。
 * 成功返回 0；返回 1 表示已是最新或已存在（信息性）。 */
int addon_install(const char *dplug);

/* 卸载指定 id 的附加包。成功返回 0；返回 -1 表示未找到。 */
int addon_uninstall(const char *id);

/* 打印已安装附加包列表。 */
int addon_list(void);

/* 运行时枚举所有可用加载器：内置 KNOWN_LOADERS + 附加包声明的 loader。
 * 打印到 stdout。 */
void list_all_loaders(void);

/* 判断某加载器是否可用（内置或由已装附加包提供）。 */
int loader_usable(const char *loader);

/* 查找某加载器对应附加包提供的模板目录（若无则 NULL，需 free）。 */
char *loader_extra_templates(const char *loader);

/* 生成一个新的 .dplug 项目骨架（含 manifest + 示例模板），供用户扩展。
 * 成功返回 0。 */
int addon_new(const char *dir, const char *id, const char *name,
              const char *type, const char *description);

#endif /* DAILYPLUGIN_ADDON_H */
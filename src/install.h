#ifndef DAILYPLUGIN_INSTALL_H
#define DAILYPLUGIN_INSTALL_H

/* 支持的加载器（可扩展：neoforge / quilt / liteloader ...） */
#define KNOWN_LOADERS "forge, fabric, neoforge, quilt, liteloader"

/* 是否为已知加载器 */
int loader_supported(const char *loader);

/* 已安装的插件库根目录 */
char *install_home(void);

/* 某加载器的库目录（分配内存，需 free） */
char *loader_dir(const char *loader);

/* 判断某加载器的库是否已安装 */
int is_installed(const char *loader);

/* 安装某加载器的插件库。成功返回 0。 */
int install_loader(const char *loader);

/* 工具链（自动下载 JDK / Gradle，均带缓存）。
 * 返回需使用的路径（已 malloc，需 free）；返回 NULL 表示直接用系统安装的版本。
 * 仅在系统缺失对应工具时才会触发下载。 */
char *ensure_jdk(void);    /* JDK 根目录，用于设置 JAVA_HOME */
char *ensure_gradle(void); /* Gradle 根目录，用于调用 bin/gradle（或 gradlew） */

#endif /* DAILYPLUGIN_INSTALL_H */
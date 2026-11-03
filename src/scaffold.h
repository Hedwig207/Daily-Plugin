#ifndef DAILYPLUGIN_SCAFFOLD_H
#define DAILYPLUGIN_SCAFFOLD_H

#include "template.h"

/* 从给定加载器模板目录生成一个可直接用 Gradle 构建的模组工程。
 * 模板与语言无关：主类统一在 src/main/java/com/example/mod/ExampleMod.java。
 * variables 提供全部 {{key}} 值；package_path 为点分隔包名
 * （如 com.owlbyte.mymod，内部转成目录路径），mod_class 为主类名。成功返回 0。 */
int scaffold_mod(const char *templates_dir,
                 const char *out_dir,
                 const TplVars *variables,
                 const char *package_path,
                 const char *mod_class);

#endif /* DAILYPLUGIN_SCAFFOLD_H */

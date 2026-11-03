#ifndef DAILYPLUGIN_PACKAGE_H
#define DAILYPLUGIN_PACKAGE_H

/* 将已生成的模组工程目录压缩为 .plug（本质是 zip 归档） */
int pack_dir(const char *dir, const char *plug_file);

/* 将 .plug 解包到指定目录 */
int unpack_plug(const char *plug_file, const char *out_dir);

#endif /* DAILYPLUGIN_PACKAGE_H */

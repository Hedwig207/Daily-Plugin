#ifndef DAILYPLUGIN_UPDATE_H
#define DAILYPLUGIN_UPDATE_H

/* ===== DailyPlugin 自动更新 =====
 * 从 GitHub Releases 检查/下载最新版本。
 *
 * 远程仓库通过环境变量 DP_UPDATE_REPO 指定，格式 "owner/repo"，
 * 默认使用 DAILYPLUGIN_DEFAULT_REPO（见 update.c）。
 *
 * 规则：
 *  - 检查版本：调用 GitHub API 的 /releases/latest，解析 tag_name（tags 形如 v1.2.3）
 *  - 比较版本（x.y.z 数字比较）
 *  - 有新版本时下载资产中匹配 dp-windows 的 dp.exe
 */

/* 当前 dp 版本（偶数字符串，与 main.c 保持一致） */
const char *dp_version(void);

/* 检查是否可联网获取远程仓库字符串（owner/repo），失败返回 NULL（需 free）。 */
char *update_repo(void);

/* 比较两个 x.y.z 版本号：a>b 返回 1，a<b 返回 -1，相等返回 0。 */
int version_cmp(const char *a, const char *b);

/* 从 GitHub Releases 查询最新版本号（tag），成功 0 并写入 out（需 free）。 */
int check_latest(const char *repo, char **out_tag);

/* 下载最新 dp-windows 资产到 out_file。成功返回 0。 */
int download_latest_dp(const char *repo, const char *out_file);

/* 一键更新：自动更新 dp.exe（本体）。返回 0 成功/无需更新，1 有更新但未应用，-1 报错。 */
int self_update(const char *current_dir);

#endif /* DAILYPLUGIN_UPDATE_H */
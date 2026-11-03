#include "template.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tpl_init(TplVars *v) {
    v->items = NULL;
    v->count = 0;
    v->cap = 0;
}

static void tpl_reserve(TplVars *v, size_t extra) {
    if (v->count + extra > v->cap) {
        size_t ncap = v->cap ? v->cap : 16;
        while (ncap < v->count + extra) ncap *= 2;
        v->items = (TplPair*)realloc(v->items, ncap * sizeof(TplPair));
        if (!v->items) { fprintf(stderr, "内存分配失败\n"); exit(1); }
        v->cap = ncap;
    }
}

void tpl_set(TplVars *v, const char *key, const char *value) {
    for (size_t i = 0; i < v->count; i++) {
        if (strcmp(v->items[i].key, key) == 0) {
            free(v->items[i].value);
            v->items[i].value = strdup(value ? value : "");
            return;
        }
    }
    tpl_reserve(v, 1);
    v->items[v->count].key = strdup(key);
    v->items[v->count].value = strdup(value ? value : "");
    v->count++;
}

const char *tpl_get(const TplVars *v, const char *key) {
    for (size_t i = 0; i < v->count; i++) {
        if (strcmp(v->items[i].key, key) == 0) return v->items[i].value;
    }
    return NULL;
}

void tpl_free(TplVars *v) {
    for (size_t i = 0; i < v->count; i++) {
        free(v->items[i].key);
        free(v->items[i].value);
    }
    free(v->items);
    tpl_init(v);
}

char *tpl_render(const char *text, size_t len, const TplVars *v) {
    SB out;
    sb_init(&out);
    size_t i = 0;
    while (i < len) {
        if (i + 2 <= len && text[i] == '{' && text[i + 1] == '{') {
            size_t j = i + 2;
            /* 找闭合 }} */
            size_t key_start = j;
            while (j + 1 < len && !(text[j] == '}' && text[j + 1] == '}')) j++;
            if (j + 1 < len && text[j] == '}' && text[j + 1] == '}') {
                size_t key_len = j - key_start;
                char *key = (char*)malloc(key_len + 1);
                if (!key) { fprintf(stderr, "内存分配失败\n"); exit(1); }
                memcpy(key, text + key_start, key_len);
                key[key_len] = '\0';
                const char *val = tpl_get(v, key);
                if (val) sb_append_cstr(&out, val);
                free(key);
                i = j + 2;
                continue;
            }
            /* 未闭合，按原样输出 */
            sb_append(&out, text + i, 2);
            i += 2;
            continue;
        }
        /* 复制到下一个 {{ 之前 */
        size_t nxt = i + 1;
        while (nxt < len && !(text[nxt] == '{' && nxt + 1 < len && text[nxt + 1] == '{')) nxt++;
        sb_append(&out, text + i, nxt - i);
        i = nxt;
    }
    char *res = sb_cstr(&out);
    return res;
}

int tpl_render_file(const char *src, const char *dst, const TplVars *v) {
    size_t len = 0;
    char *text = read_file(src, &len);
    if (!text) return -1;
    char *rendered = tpl_render(text, len, v);
    free(text);

    /* 确保目标父目录存在再写入 */
    char *parent = strdup(dst);
    size_t plen = strlen(parent);
    while (plen > 0 && parent[plen - 1] != '/' && parent[plen - 1] != '\\') parent[--plen] = '\0';
    if (plen > 1) {
        parent[plen - 1] = '\0';
        mkdir_p(parent);
    }
    free(parent);

    int rc = write_file(dst, rendered, strlen(rendered));
    free(rendered);
    return rc;
}

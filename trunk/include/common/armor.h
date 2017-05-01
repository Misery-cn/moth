#ifndef _ARMOR_H_
#define _ARMOR_H_

#ifdef __cplusplus
extern "C" {
#endif

int armor(char *dst, const char *dst_end, const char *src, const char *end);

int armor_linebreak(char *dst, const char *dst_end, const char *src, const char *end, int line_width);

int unarmor(char *dst, const char *dst_end, const char *src, const char *end);
#ifdef __cplusplus
}
#endif

#endif

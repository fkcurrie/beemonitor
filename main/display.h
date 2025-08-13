#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

void display_init(void);
void display_show_version(const char *version);
void display_show_copyright(void);
void display_wifi_connecting(const char *ssid);
void display_update_counts(int in_count, int out_count);

#ifdef __cplusplus
}
#endif

#endif // _DISPLAY_H_

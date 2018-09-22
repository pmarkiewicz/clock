#ifndef __UDP__H__
#define __UDP__H__

void upd_init();
void sendNTPpacket();
int udp_parse();
unsigned long udp_read_time();

#endif
#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

void receive_notification(notification_t*);
const notification_t* retreive_notification(unsigned int);
const notification_t* get_all_notifications(unsigned int*);
void dismiss_notification(unsigned int);
void notification_init();
unsigned int get_total_notifications();

#endif /* NOTIFICATION_MANAGER_H */

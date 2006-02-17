#ifndef __PA_QSA_INTERFACE_H
#define __PA_QSA_INTERFACE_H

int pa_qsa_interface_init();
void pa_qsa_interface_destroy();

#include "presentity.h"

int notify_qsa_watchers(presentity_t *p);
int notify_internal_watcher(presentity_t *p, internal_pa_subscription_t *ss);
void free_internal_subscription(internal_pa_subscription_t *is);

presentity_info_t *presentity2presentity_info(presentity_t *p);

extern int accept_internal_subscriptions;

#endif

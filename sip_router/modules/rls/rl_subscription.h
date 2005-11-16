#ifndef __RL_SUBSCRIPTION_H
#define __RL_SUBSCRIPTION_H

#include "../tm/dlg.h"
#include "../../lock_ops.h"
#include "subscription_manager.h"
#include <xcap/resource_lists_parser.h>
#include <cds/vector.h>
#include <cds/ptr_vector.h>
#include <cds/sstr.h>

#include <presence/subscriber.h>

/* type for generated database ID */
typedef char db_id_t[48];

typedef enum {
	rls_auth_none, 
	rls_auth_implicit,
	rls_auth_xcap
} rls_authorization_type_t;

typedef struct {
	rls_authorization_type_t type;
	char *xcap_root; /* zero terminated (using libcurl, why not?) */
} rls_auth_params_t;

struct _virtual_subscription_t;
struct _rl_subscription_t;
typedef struct _rl_subscription_t rl_subscription_t;

typedef struct {
	str name;
	str lang;
} vs_display_name_t;

typedef struct _virtual_subscription_t {
	/* local subscription data */
	subscription_t *local_subscription;
	msg_queue_t mq;
	
	vector_t display_names;

	rl_subscription_t *subscription;
	
	/** whole document describing the state of this resource */
	str state_document;
	/** type of this state_document */
	str content_type;
	/** status of this subscription */
	subscription_status_t status;
	
	/* VS identifier */
	str uri;
	
	struct _virtual_subscription_t *next;
	struct _virtual_subscription_t *prev;

	/* generated id for database */
	db_id_t dbid;

	char uri_str[1];
} virtual_subscription_t;

/** subscription to the list of resources */
struct _rl_subscription_t {
	subscription_data_t subscription;
	
	/** sequence number of NOTIFY */
	int doc_version;

	/** the count of changed virtual subscriptions 
	 * (enough changes = send notify) */
	int changed;
	
	/* virtual subscriptions for this rls */
	ptr_vector_t vs;

	/* generated id for database */
	db_id_t dbid;
};

#define rls_get_uri(s)			&((s)->subscription.record_id)
#define rls_get_package(s)		&((s)->subscription.package)
#define rls_get_subscriber(s)	&((s)->subscription.subscriber)

extern subscription_manager_t *rls_manager;

/********* resource list subscription functions ********/

int rls_init();
int rls_destroy();
void rls_lock();
void rls_unlock();

int rls_create_subscription(struct sip_msg *m, rl_subscription_t **dst, const char *xcap_root);
int rls_refresh_subscription(struct sip_msg *m, rl_subscription_t *s);
int rls_find_subscription(str *from_tag, str *to_tag, str *call_id, rl_subscription_t **dst);
void rls_free(rl_subscription_t *s); /* removes from memory only */
void rls_remove(rl_subscription_t *s); /* finishes subscription - removes from DB, mem, ... */
int rls_generate_notify(rl_subscription_t *s, int full_info);
int rls_prepare_subscription_response(rl_subscription_t *s, struct sip_msg *m);

/* void rls_notify_all_modified(); */

/********* virtual subscription functions ********/

int vs_init();
int vs_destroy();

int vs_create(str *uri, str *package, virtual_subscription_t **dst, display_name_t *dnames, rl_subscription_t *subscription);
int vs_add_display_name(virtual_subscription_t *vs, const char *name, const char *lang);
void vs_free(virtual_subscription_t *vs);

/* database operations */
int rls_db_add(rl_subscription_t *s);
int rls_db_remove(rl_subscription_t *s);
int rls_db_update(rl_subscription_t *s);
int db_load_rls(); /* load stored subscriptions on startup */

/* helper functions */
void generate_db_id(db_id_t *id, void *data);

/** returns the count of seconds remaining to subscription expiration */
int rls_subscription_expires_in(rl_subscription_t *s);

/* allocates and initializes structure */
rl_subscription_t *rls_alloc_subscription();


#endif
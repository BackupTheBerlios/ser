/*
 * $Id: agent.c,v 1.2 2002/08/29 20:13:30 ric Exp $
 *
 * SNMP Module: SNMP Agent initialization code and MIB initialization code 
 */

#include "snmp_mod.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "../../config.h"

#define SER_SNMP_NAME "ser_snmp"
#define SUB_AGENT 1

static int init_sip_mibs();

/* XXX: We need these functions to allow for runtime discovery of where the
 * master agent is waiting for agentx connections. It's sort of a hack,
 * to get around some problems in the net-snmp library. If those problems
 * get fixed, then we should be able to get rid of these
 * (as of 5.0.4 they haven't been fixed)
 */
extern void init_agentx_config();
extern int subagent_pre_init();
extern void init_subagent();

/*********************** The subagent **************************/

/* Our application index (assigned by applTable). Needed to fill tables */
static int *serIndex=NULL;

static int agent_pid = -1;

/* 
 * - If you want the agent to try to reconnect to the master agent, set
 * agentxPingInterval <seconds>
 * in ser_snmp.conf
 * - If you want the agent to use something different than /var/agentx/master
 * for agentx connections, set
 * agentx <path or socket>	(path -> /var/agentx/master, socket-> tcp:20001)
 * in ser_snmp.conf (look at snmpcmd(1) for details on transport
 * specifications -> section AGENT SPECIFICATION)
 */
int ser_init_snmp()
{
	int port, port_len = 15; /* more than enough... > strlen("udp:<port>") */
	char port_spec[port_len];

	snmp_disable_stderrlog();

	/* 
	 * Initialize library and read config file. Note that init_agent()
	 * won't try to open a connection to the master agent. We
	 * check its return value even though (at time of writing) it's
	 * only dependent on the subagent functions, which won't be
	 * called.
	 */
	if(init_agent(SER_SNMP_NAME) != 0) {
		LOG(L_ERR, "snmp_mod: Couldn't initialize SNMP agent library\n");
		return -1;
	}
	/* 
	 * register config parameters we may use. For example:
	 * - agentxsocket
	 * - agentxPingInterval
	 */
	init_agentx_config();
	subagent_pre_init();

	/* Read the config files and finish init of snmp lib */
	init_snmp(SER_SNMP_NAME);

	/* 
	 * now comes the interesting part. First we try to be a subagent
	 * (the next call changes the behavior of the second call to
	 * subagent_pre_init()). If that fails, then we try to be a
	 * master agent. If that fails, then we die :)
	 */
	netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, 
		NETSNMP_DS_AGENT_ROLE, SUB_AGENT);
	
	if(subagent_pre_init() == 0) {
		/* ok, running as subagent, cool, cool */
		init_subagent();
		LOG(L_DBG, "snmp_mod: Running as agentx subagent\n");
	} else {
		/* eeks... try master way then ... */
		LOG(L_INFO, "snmp_mod: Couldn't initialize as subagent. "
			"Trying now as master agent\n");
		/* deactivate agentx stuff */
		netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, 
			NETSNMP_DS_AGENT_ROLE, 0);

		/* Set the port from the default port setting in snmp.conf. 
		 * If you want us to be able to run as non-root, just
		 * set a non-priviledged port in defaultPort in snmp.conf.
		 * Note that this is only to run as standalone agent. */
		port = netsnmp_ds_get_int(NETSNMP_DS_LIBRARY_ID,
					NETSNMP_DS_LIB_DEFAULT_PORT);
		snprintf(port_spec, port_len, "udp:%d", port);
		if(netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID, 
					NETSNMP_DS_AGENT_PORTS, port_spec) != SNMPERR_SUCCESS) {
			LOG(L_ERR, "snmp_mod ERROR: Couldn't initializa as master agent\n");
			return -1;
		}
		if(init_master_agent() != 0) {
			LOG(L_ERR, "snmp_mod ERROR: Couldn't initialize as master agent\n");
			/* try to give a meaningful error message */
			if((port < 1024) && (geteuid() != 0)) {
				LOG(L_ERR, "snmp_mod ERROR: not running as root and trying to "
					"use a priviledged port %d\n", port);
				LOG(L_ERR, "snmp_mod ERROR: set defaultPort in "
					"snmp.conf to a non-priviledged port or run as root\n");
			}
			return -1;
		}
		LOG(L_DBG, "snmp_mod: Running as master agent\n");
	}

	/* initialize the mib code */
	if(init_sip_mibs() == -1) {
		LOG(L_ERR, "snmpd_mod ERROR: Couldn't initialize SIP MIB\n");
		return -1;
	}

	return 0;
}

/* control mechanism for the agent */
static int keep_running;

static inline void stop_agent(int s)
{
    keep_running = 0;
}

/* main loop for subagent */
int ser_snmp_start()
{
	const char * func = "snmp_mod";
	int pid;

	LOG(L_DBG, "%s: starting snmp agent..\n", func);

	/* ok, no more registrations possible... fork() and forget */ 
	pid = fork();
	if(pid == -1) {
		LOG(L_ERR, "%s: Couldn't start agent: %s\n", func, 
			strerror(errno));
		return -1;
	} else if(pid != 0) { /* DAD */
		agent_pid = pid;
		LOG(L_DBG, "snmp_mod: all good, SNMP agent's pid %d\n", agent_pid);
		return 0;
	}

	/* the child */
	/* In case we receive a request to stop (kill -TERM or kill -INT) */
	keep_running = 1;
	signal(SIGTERM, stop_agent);
	signal(SIGINT, stop_agent);
	
	/* main loop */
	while(keep_running) {
		agent_check_and_process(1); /* 0 == don't block */
	}

	/* the end.. */
	snmp_shutdown(SER_SNMP_NAME);
	
	exit(0);
}

/* Initializes internal variables that need to be shared between ser and
 * snmp agent */
int init_snmpVars()
{
	const char *func = "snmp_mod";

	serIndex = malloc(sizeof(int));
	if(!serIndex) {
		LOG(L_ERR, "%s: Out of memory\n", func);
		return -1;
	}
	*serIndex = -1;

	return 0;
}

int ser_snmp_stop()
{
	if(agent_pid == -1) {
		LOG(L_ERR,"Agent not present!! (or maybe we lost its pid?? oops, sorry, "
				"seems like i'm getting old...)\n");
		return -1;
	}
	LOG(L_DBG,"Killing snmp agent %d\n", agent_pid);
	if(kill(agent_pid, SIGTERM) == -1) {
		LOG(L_ERR, "Couldn't kill the SNMP agent: %s\n", strerror(errno));
		return -1;
	}

	LOG(L_DBG,"Killed snmp agent successfully\n");
	agent_pid = -1;
	return 0;
}

/********************** MIB Code **************************/

#define init_branch(x) \
	if(init_##x() == -1) { \
		LOG(L_ERR, "snmp_mod: Couldn't initialize " #x "\n"); \
		return -1; \
	}

/* Initializes the tables we support. These are either filled
 * in fill_sip_mibs() or by external sources using the dynamic
 * handler */
static int init_sip_mibs()
{
	const char *func = "snmp_mod";
	/* Look at section 4.2 of SIP MIB draft for meaning of this
	 * name */
	const char* ourName = "sip_proxy_redirect_registrar";

	/* init application (network-services) MIB */
	init_branch(applTable);

	/* init sipCommonMIB */
	init_branch(sipCommonCfgTable)
	init_branch(sipCommonCfgBase)
	init_branch(sipCommonCfgTimer)
	init_branch(sipCommonCfgRetry)
	init_branch(sipCommonCfgExpires)
	init_branch(sipCommonStatsSummary)
	init_branch(sipCommonStatsMethod)
	init_branch(sipCommonStatusCode)
	init_branch(sipCommonStatsTrans)
	init_branch(sipCommonStatsRetry)
	init_branch(sipCommonStatsOther)

	/* init sipServerMIB */
	init_branch(sipServerCfg)
	init_branch(sipProxyCfg)
	init_branch(sipProxyStats)
	init_branch(sipRegCfg)
	init_branch(sipRegStats)

	/* Get the application index early on. The rest of the
	 * tables (along with the rest of the members of the
	 * applTable) are added in fill_sip_mibs() */
	*serIndex = snmp_registerAppl(ourName);
	if(*serIndex == -1) {
		LOG(L_ERR, "%s: Failed to add us to applTable\n", func);
		return -1;
	}

	return 0;
}

#define add_branch(x, y) \
	if(ser_add_##x y == -1) { \
		LOG(L_ERR, "%s: Failed to add us to " #x "\n", func); \
		*serIndex = -1; \
		return -1; \
	}

/* adds the objects we support locally */
int fill_sip_mibs()
{
	const char *func = "snmp_mod";

	/* fill the sip info. Note that most of this is dummy stuff */
	add_branch(sipCommonCfgTable, ())

	/* the end.. */
	return 0;
}

int ser_getApplIndex()
{
	if(!serIndex)
		return -1;
	else 
		return *serIndex;
}

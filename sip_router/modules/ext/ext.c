#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../../sr_module.h"
#include "../../error.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "../../globals.h"
#include "../../parser/msg_parser.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "my_exec.h"

#define MAX_BUFFER_LEN 1024


static int ext_child_init(int);
static int ext_rewriteuser(struct sip_msg*, char*, char* );
static int ext_rewriteuri(struct sip_msg*, char*, char* );
static int fixup_ext_rewrite(void** param, int param_no);


struct module_exports exports= {
	"ext",
	(char*[]){
				"ext_rewriteuser",
				"ext_rewriteuri"
			},
	(cmd_function[]){
					ext_rewriteuser,
					ext_rewriteuri
					},
	(int[]){
				1,
				1
			},
	(fixup_function[]){
				fixup_ext_rewrite,
				fixup_ext_rewrite
		},
	2,

	0,    /* Module parameter names */
	0,    /* Module parameter types */
	0,
	0,    /* Number of module paramers */

	0,   /* module initialization function */
	0,   /* response function */
	0,   /* module exit function */
	0,
	(child_init_function) ext_child_init  /* per-child init function */
};




static int ext_child_init(int child)
{
	return init_ext();
}




static int fixup_ext_rewrite(void** param, int param_no)
{
	int fd;

	if (param_no==1) {
		fd = open(*param,O_RDONLY);
		if (fd==-1) {
			LOG(L_ERR,"ERROR:fixup_ext_rewrite: [%s] -> %s\n",
				(char*)*param,strerror(errno));
			return E_UNSPEC;
		}
		close(fd);
	}
	return 0;
}




inline char *run_ext_prog(char *cmd, char *in, int in_len, int *out_len)
{
	static char buf[MAX_BUFFER_LEN];
	int len;
	int ret;

	/* launch the external program */
	if ( start_prog(cmd)!=0 ) {
		LOG(L_ERR,"ERROR:run_ext_prog: cannot launch external program\n");
		return 0;
	}

	/* feeding the program with the uri */
	if ( sendto_prog(in,in_len,1)!=in_len ) {
		LOG(L_ERR,"ERROR:run_ext_prog: cannot send input to the external "
			"program -> kill it\n");
		goto kill_it;
	}
	close_prog_input();

	/* gets its response */
	len = 0;
	do {
		ret = recvfrom_prog(buf-len,1024-len);
		if (ret==-1 && errno!=EINTR) {
			LOG(L_ERR,"ERROR:run_ext_prog: cannot read from the external "
				"program (%s) -> kill it\n",strerror(errno));
			goto kill_it;
		}
		len += ret;
	}while(ret);

	close_prog_output();
	ret = is_finished();
	DBG("DEBUG:run_ext_prog: recv <%.*s> [%d] ; status=%d\n",
		len, buf,len,is_finished());

	if (ret!=0) {
		*out_len = 0;
		return 0;
	}

	*out_len = len;
	return buf;
kill_it:
	kill_prog();
	wait_prog();
	close_prog_input();
	close_prog_output();
	*out_len = 0;
	return 0;
}




static int ext_rewriteuser(struct sip_msg *msg, char *cmd, char *foo_str )
{
	struct sip_uri parsed_uri;
	str *uri;
	str buf;
	str user;
	str new_uri;
	char *p;
	int  i;

	/* take the uri out -> from new_uri or RURI */
	if (msg->new_uri.s && msg->new_uri.len)
		uri = &(msg->new_uri);
	else if (msg->first_line.u.request.uri.s &&
	msg->first_line.u.request.uri.len )
		uri = &(msg->first_line.u.request.uri);
	else {
		LOG(L_ERR,"ERROR:ext_rewriteuser: cannot find Ruri in msg!\n");
		return -1;
	}
	/* parse it to identify username */
	if (parse_uri(uri->s,uri->len,&parsed_uri)<0 ) {
		LOG(L_ERR,"ERROR:ext_rewriteuser : cannot parse Ruri!\n");
		return -1;
	}
	/* drop it if no username found */
	if (!parsed_uri.user.s && !parsed_uri.user.len) {
		LOG(L_INFO,"INFO:ext_rewriteuser: username not present in RURI->"
			" exitting without error\n");
		goto done;
	}

	/*  run the external program */
	buf.s = run_ext_prog(cmd, parsed_uri.user.s, parsed_uri.user.len,
		&buf.len );
	if ( !buf.s || !buf.len) {
		LOG(L_ERR,"ERROR:ext_rewriteuser: empty string returned by external"
			" program -> abort rewritening!\n");
		goto error;
	}

	i = 0;
	user.s = buf.s;
	while (user.s!=buf.s+buf.len) {
		/* jump over space, tab, \n and \r */
		while ( user.s<buf.s+buf.len && ( *(user.s)==' '
		|| *(user.s)=='\t' || *(user.s)=='\n' || *(user.s)=='\r') )
			user.s++;
		/* go to the end of user */
		user.len = 0;
		while ( (p=user.s+user.len)<buf.s+buf.len && *p!=' '
		&& *p!='\t' && *p!='\n' && *p!='\r')
			user.len++;
		if (!user.len) {
			LOG(L_ERR,"ERROR:ext_rewriteuser:error parsing external prog "
			"output: <%.*s> at char[%c]\n",buf.len,buf.s,user.s[0]);
			goto error;
		}

		/* compose the new uri */
		DBG("DEBUG:ext_rewriteuser: processing user <%.*s> [%d]\n",user.len,
			user.s,user.len);
		new_uri.len = 4/*sip:*/+user.len+1/*@*/+parsed_uri.host.len+
			+(parsed_uri.port.len!=0)+parsed_uri.port.len
			+(parsed_uri.params.len!=0)+parsed_uri.params.len
			+(parsed_uri.headers.len!=0)+parsed_uri.headers.len;
		new_uri.s = (char*)pkg_malloc(new_uri.len);
		if (!new_uri.s) {
			LOG(L_ERR,"ERROR:ext_rewriteuri: no more free pkg memory\n");
			goto error;
		}
		p = new_uri.s;
		memcpy(p,"sip:",4);
		p += 4;
		memcpy(p,user.s,user.len);
		p += user.len;
		*(p++) = '@';
		memcpy(p,parsed_uri.host.s,parsed_uri.host.len);
		p += parsed_uri.host.len;
		if (parsed_uri.port.len) {
			*(p++) = ':';
			memcpy(p,parsed_uri.port.s,parsed_uri.port.len);
			p += parsed_uri.port.len;
		}
		if (parsed_uri.params.len) {
			*(p++) = ';';
			memcpy(p,parsed_uri.params.s,parsed_uri.params.len);
			p += parsed_uri.params.len;
		}
		if (parsed_uri.headers.len) {
			*(p++) = '?';
			memcpy(p,parsed_uri.headers.s,parsed_uri.headers.len);
			p += parsed_uri.headers.len;
		}

		/* now, use it! */
		DBG("DEBUG:ext_rewriteuser: setting uri <%.*s> [%d]\n",new_uri.len,
			new_uri.s,new_uri.len);
		if (i==0) {
			/* set in sip_msg the new uri */
			if (msg->new_uri.s && msg->new_uri.len)
				pkg_free(msg->new_uri.s);
			msg->new_uri.s = new_uri.s;
			msg->new_uri.len = new_uri.len;
		} else {
			LOG(L_WARN,"WARNING:ext_rewriteuser: fork not supported -> dumping"
				" uri %d <%.*s>\n",i,new_uri.len,new_uri.s);
			pkg_free(new_uri.s);
		}

		i++;
		user.s += user.len;
	}

done:
	free_uri(&parsed_uri);
	return 1;
error:
	free_uri(&parsed_uri);
	return -1;
}




static int ext_rewriteuri(struct sip_msg *msg, char *cmd, char *foo_str )
{
	str  *uri;
	str  buf;
	str  new_uri;
	char *c;
	int  i;

	/* take the uri out -> from new_uri or RURI */
	if (msg->new_uri.s && msg->new_uri.len)
		uri = &(msg->new_uri);
	else if (msg->first_line.u.request.uri.s &&
	msg->first_line.u.request.uri.len )
		uri = &(msg->first_line.u.request.uri);
	else {
		LOG(L_ERR,"ERROR:ext_rewriteuri: cannot find Ruri in msg!\n");
		return -1;
	}

	/*  run the external program */
	buf.s = run_ext_prog(cmd, uri->s, uri->len, &buf.len );
	if ( !buf.s || !buf.len) {
		LOG(L_ERR,"ERROR:ext_rewriteuri: empty string returned by external"
			" program -> abort rewritening!\n");
		return -1;
	}

	i = 0;
	new_uri.s = buf.s;
	while (new_uri.s!=buf.s+buf.len) {
		/* jump over space, tab, \n and \r */
		while ( new_uri.s<buf.s+buf.len && ( *(new_uri.s)==' '
		|| *(new_uri.s)=='\t' || *(new_uri.s)=='\n' || *(new_uri.s)=='\r') )
			new_uri.s++;
		/* go to the end of uri */
		new_uri.len = 0;
		while ( (c=new_uri.s+new_uri.len)<buf.s+buf.len && *c!=' '
		&& *c!='\t' && *c!='\n' && *c!='\r')
			new_uri.len++;
		if (!new_uri.len) {
			LOG(L_ERR,"ERROR:ext_rewriteuri:error parsing external prog output"
			": <%.*s> at char[%c]\n",buf.len,buf.s,new_uri.s[0]);
			return -1;
		}

		/* now, use it! */
		DBG("DEBUG:ext_rewriteuri: setting <%.*s> [%d]\n",new_uri.len,
			new_uri.s,new_uri.len);
		if (i==0) {
			if (msg->new_uri.s && msg->new_uri.len)
				pkg_free(msg->new_uri.s);
			msg->new_uri.s = (char*)pkg_malloc(new_uri.len);
			if (!msg->new_uri.s) {
				LOG(L_ERR,"ERROR:ext_rewriteuri: no more free pkg memory\n");
				return -1;
			}
			msg->new_uri.len = new_uri.len;
			memcpy(msg->new_uri.s,new_uri.s,new_uri.len);
		} else {
			LOG(L_WARN,"WARNING:ext_rewriteuri: fork not supported -> dumping"
				" uri %d <%.*s>\n",i,new_uri.len,new_uri.s);
		}

		i++;
		new_uri.s += new_uri.len;
	}

	return 1;
}



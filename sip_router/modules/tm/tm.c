/*$Id: tm.c,v 1.6 2001/11/26 21:21:09 bogdan Exp $
 *
 * Example ser module, it will just print its string parameter to stdout
 *
 */



#include "../../sr_module.h"
#include "../../dprint.h"
#include "sip_msg.h"
#include <stdio.h>

static int test_f(struct sip_msg*, char*,char*);

static struct module_exports nm_exports= {	"tm_module", 
												(char*[]){"tm_test"},
												(cmd_function[]){test_f},
												(int[]){1},
												(fixup_function[]){0},
												1,
												0
											};


struct module_exports* mod_register()
{
	fprintf(stderr, "nm - registering...\n");
	return &nm_exports;
}


static int print_f(struct sip_msg* msg, char* str, char* str2)
{
	/*we registered only 1 param, so we ignore str2*/
	printf("%s\n",str);
	return 1;
}

static int test_f(struct sip_msg* msg, char* s1, char* s2)
{
	struct sip_msg* tst;
	struct hdr_field* hf;

	DBG("in test_f\n");

	tst=sip_msg_cloner(msg);
	DBG("after cloner...\n");
	DBG("first_line: <%s> <%s> <%s>\n", 
			tst->first_line.u.request.method.s,
			tst->first_line.u.request.uri.s,
			tst->first_line.u.request.version.s
		);
	if (tst->h_via1)
		DBG("via1: <%s> <%s>\n", tst->h_via1->name.s, tst->h_via1->body.s);
	if (tst->h_via2)
		DBG("via2: <%s> <%s>\n", tst->h_via2->name.s, tst->h_via2->body.s);
	if (tst->callid)
		DBG("callid: <%s> <%s>\n", tst->callid->name.s, tst->callid->body.s);
	if (tst->to)
		DBG("to: <%s> <%s>\n", tst->to->name.s, tst->to->body.s);
	if (tst->cseq)
		DBG("cseq: <%s> <%s>\n", tst->cseq->name.s, tst->cseq->body.s);
	if (tst->from)
		DBG("from: <%s> <%s>\n", tst->from->name.s, tst->from->body.s);
	if (tst->contact)
		DBG("contact: <%s> <%s>\n", tst->contact->name.s,tst->contact->body.s);

	DBG("headers:\n");
	for (hf=tst->headers; hf; hf=hf->next){
		DBG("header %d: <%s> <%s>\n", hf->type, hf->name.s, hf->body.s);
	}
	if (tst->eoh!=0) return;
	DBG("parsing all...\n"); 
	if (parse_headers(tst, HDR_EOH)==-1)
		DBG("error\n");
	else{
		DBG("new headers...");
		for (hf=tst->headers; hf; hf=hf->next){
			DBG("header %d: <%s> <%s>\n", hf->type, hf->name.s, hf->body.s);
		}
	}

	free(tst);
}




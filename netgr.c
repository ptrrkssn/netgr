/*
** nergr.c - A small tool to display/check NIS netgroups
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>

#include "strmatch.h"
#include "list.h"

char *argv0 = "netgr";
char *ypdom = NULL;
char *ypmap = "netgroup";

char *match = NULL;

int debug = 0;
int verbose = 0;
int pretty = 0;
int exact = 0;
int n_match = 0;
int n_errors = 0;

int p_groups = 0;
int p_nodes = 0;


typedef struct {
    char *host;
    char *user;
    char *domain;

    List *groups;
} NODE;

typedef struct {
    char *name;
    
    List *nodes;
    List *groups;
} GROUP;


List *all_nodes = NULL;
List *all_groups = NULL;


NODE *
node_lookup(List *nodes,
	    const char *host,
	    const char *user,
	    const char *domain)
{
    unsigned int i;
    NODE *np = NULL;
    

    if (!nodes)
	return NULL;
    
    for (i = 0; i < nodes->c; i++) {
	np = nodes->v[i];
	if ((!host || strcmp(host, np->host) == 0) &&
	    (!user || strcmp(user, np->user) == 0) &&
	    (!domain || strcmp(domain, np->domain) == 0))
	    return np;
    }
    return NULL;
}


NODE *
node_add(List *nodes,
	 const char *host,
	 const char *user,
	 const char *domain)
{
    NODE *np;

    
    np = node_lookup(all_nodes, host, user, domain);
    if (!np) {
	np = malloc(sizeof(NODE));
	if (!np)
	    return NULL;
	
	if (debug > 2)
	    fprintf(stderr, "*** node_add(%s,%s,%s): New node\n",
		    host ? host : "",
		    user ? user : "",
		    domain ? domain : "");
	
	np->host = host ? strdup(host) : NULL;
	np->user = user ? strdup(user) : NULL;
	np->domain = domain ? strdup(domain) : NULL;
	np->groups = list_new(0);

	list_append(all_nodes, np);
    }

    if (nodes)
	list_add(nodes, np);
    
    return np;
}



GROUP *
group_lookup(List *groups,
	     const char *name)
{
    unsigned int i;
    GROUP *gp = NULL;


    if (!groups || !name)
	return NULL;
    
    for (i = 0; i < groups->c; i++) {
	gp = groups->v[i];
	if (strcmp(name, gp->name) == 0)
	    return gp;
    }
    return NULL;
}


GROUP *
group_add(List *groups,
	  const char *name)
{
    GROUP *gp;
    
    
    gp = group_lookup(all_groups, name);
    if (!gp) {
	gp = malloc(sizeof(GROUP));
	if (!gp)
	    return NULL;

	if (debug > 2)
	    fprintf(stderr, "*** group_add(%s): New group\n", name);
	
	gp->name = strdup(name);
	gp->nodes = list_new(0);
	
	list_append(all_groups, gp);
    }

    if (groups)
	list_add(groups, gp);
    
    return gp;
}


void
p_spaces(int n)
{
    while (n-- > 0)
	putchar(' ');
}

char *
trim(char *s)
{
    int p = strlen(s)-1;
    while (p > 0 && isspace(s[p]))
	--p;
    if (p > 0 && s[p] == ')')
	--p;
    s[p] = '\0';
    
    for (p = 0; s[p]; p++)
	if (s[p] == ',')
	    s[p] = ':';
    
    if (s[0] == '(')
	return s+1;
    
    return s;
}


int
str2ngh(char *s,
	char **h,
	char **u,
	char **d)
{
    char *lp = NULL;

    
    if (*s++ != '(')
	return 0;

    *h = strtok_r(s, ",)", &lp);
    *u = strtok_r(NULL, ",)", &lp);
    *d = strtok_r(NULL, ",)", &lp);

    if (!*h)
	*h = strdup("");
    
    if (!*u)
	*u = strdup("");
    
    if (!*d)
	*d = strdup("");
    
    return 1;
}

int
_s_match(const char *s,
	const char *m)
{
    if (exact)
	return strcmp(s, m) == 0;
    
    if (strchr(m, '*') || strchr(m, '?'))
	return strmatch(s, m);

    return strstr(s, m) != NULL;
}

int
s_match(const char *s,
	const char *m)
{
    int rc = _s_match(s, m);
    
    if (debug > 3)
	fprintf(stderr, "*** s_match(%s,%s) = %d\n", s, m, rc);

    return rc;
}


GROUP *
netgr_lookup(List *groups,
	     List *nodes,
	     char *name)
{
    char *cp;
    char *res = NULL;
    char *lp = NULL;
    int rc, len = 0;
    int level = -1;
    unsigned int i;
    GROUP *gp, *ngp;
    NODE *np;
    
    
    if (debug)
	fprintf(stderr, "*** lookup(\"%s\"):\n", name);
    
    rc = yp_match(ypdom, ypmap, name, strlen(name), &res, &len);
    if (rc != 0) {
	fprintf(stderr, "%s: %s: %s\n", argv0, name, yperr_string(rc));
	++n_errors;
	return NULL;
    }

    gp = group_add(groups, name);
    
    cp = strtok_r(res, " \t\n", &lp);
    while (cp) {
	if (*cp == '(') {
	    char *h = NULL;
	    char *u = NULL;
	    char *d = NULL;


	    if (str2ngh(cp, &h, &u, &d) != 1) {
		fprintf(stderr, "%s: %s: Invalid node\n", argv0, cp);
		goto Next;
	    }
	    
	    if (match && !(s_match(h, match) ||
			   s_match(u, match) ||
			   s_match(d, match)))
		goto Next;
	    
	    np = node_add(nodes, h, u, d);
	    
	    if (groups) {
		for (i = 0; i < groups->c; i++) {
		    ngp = groups->v[i];
		    
		    if (debug > 1)
			fprintf(stderr, "*** Adding node (%s,%s,%s) to group %s\n",
				np->host,
				np->user,
				np->domain,
				ngp->name);
		    list_add(ngp->nodes, np);
		    
		    if (debug > 1)
			fprintf(stderr, "*** Adding group %s to node (%s,%s,%s)\n",
				ngp->name,
				np->host,
				np->user,
				np->domain);
		    list_add(np->groups, ngp);
		}
	    }
	} else {
	    int j;
	    
	    for (j = 0; j < groups->c; j++) {
		ngp = groups->v[j];
		
		if (strcmp(ngp->name, cp) == 0)
		    break;
	    }
	    if (j < groups->c) {
		ngp = groups->v[j-1];
		
		if (j == level) {
		    fprintf(stderr, "%s: %s in %s: Redundant group inclusion (ignored)\n",
			    argv0, cp, ngp->name);
		} else {
		    fprintf(stderr, "%s: %s in %s: Infinite recursive loop\n",
			    argv0, cp, ngp->name);
		    if (debug) {
			fprintf(stderr, "*** Group traceback:\n");
			for (j = 0; j < groups->c; j++) {
			    ngp = groups->v[j];
			    fprintf(stderr, "\t%3d\t%s\n", j, ngp->name);
			}
		    }
		    list_delete(groups, gp);
		    return gp;
		}
	    } else {
		ngp = netgr_lookup(groups, nodes, cp);
		if (!ngp) {
		    list_delete(groups, gp);
		    return NULL;
		}
		
	    }
	}
      Next:
	cp = strtok_r(NULL, " \t\n", &lp);
    }
    
    free(res);
    res = NULL;
    
    list_delete(groups, gp);
    return gp;
}





int
yp_foreach(int s,
	   char *k, int klen,
	   char *v, int vlen,
	   char *d)
{
    int rc = 0;
    List *groups = NULL;
    List *nodes = NULL;
    
    
    if (s == YP_NOMORE)
	return 0;
    
    if (s != YP_TRUE)
	return -1;
    
    if (debug) {
	if (debug > 1)
	    fprintf(stderr, "*** yp_foreach s=%d k=%.*s v=%.*s\n", s, klen, k, vlen, v);
	else
	    fprintf(stderr, "*** yp_foreach s=%d k=%.*s\n", s, klen, k);
    }
    
    if (klen == 0 && vlen == 0)
	return 0;

    k[klen] = 0;
    
    groups = list_new(0);
    nodes = list_new(0);
    
    if (netgr_lookup(groups, nodes, k) == NULL)
	rc = -1;

    list_free(groups);
    list_free(nodes);
    
    if (rc < 0)
	return rc;

    return 0;
}


int
match_innetgr(char *group,
	      char *name)
{
    if (innetgr(group, name, NULL, NULL) == 1)
	return 1;

    if (innetgr(group, NULL, name, NULL) == 1)
	return 2;
    
    return 0;
}


int
g_compare(const void *p1,
	  const void *p2)
{
    const GROUP *g1 = *(GROUP **) p1;
    const GROUP *g2 = *(GROUP **) p2;

    return strcmp(g1->name, g2->name);
}

int
n_compare(const void *p1,
	  const void *p2)
{
    const NODE *n1 = *(NODE **) p1;
    const NODE *n2 = *(NODE **) p2;
    int rc;

    
    rc = strcmp(n1->host, n2->host);
    if (rc)
	return rc;
    
    rc = strcmp(n1->user, n2->user);
    if (rc)
	return rc;
    
    rc = strcmp(n1->domain, n2->domain);
    return rc;
}

int
main(int argc,
     char *argv[])
{
    int rc, i, j;
    int n_groups = 0;
    

    all_groups = list_new(0);
    all_nodes = list_new(0);
    
    argv0 = argv[0];
    
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
	for (j = 1; argv[i][j]; j++)
	    switch (argv[i][j]) {
	      case 'h':
		printf("Usage: %s [<options>] [<group-1>] [... <group-N>]\n", argv[0]);
		puts("Options:");
		puts("\t-h                          Display this information");
		puts("\t-v                          Increase verbosity level");
		puts("\t-g                          Print groups");
		puts("\t-n                          Print nodes");
		puts("\t-x                          Exact match only");
		puts("\t-d                          Increase debug level");
		puts("\t-m<match>                   Filter match");
		puts("\t-D<domain>                  YP domain");
		printf("\t-M<map>                     YP map (default: %s)\n", ypmap);
		exit(0);
		
	      case 'd':
		++debug;
		break;
		
	      case 'v':
		++verbose;
		break;

	      case 'g':
		p_groups = 1;
		break;

	      case 'n':
		p_nodes = 1;
		break;

	      case 'x':
		++exact;
		break;

	      case 'M':
		if (argv[i][j+1])
		    ypmap = strdup(argv[i]+j+1);
		else if (argv[i+1])
		    ypmap = strdup(argv[++i]);
		else {
		    fprintf(stderr, "%s: %s: Missing required map argument\n",
			    argv0, argv[i]);
		    exit(1);
		}
		goto NextArg;
		
	      case 'm':
		if (argv[i][j+1])
		    match = strdup(argv[i]+j+1);
		else if (argv[i+1])
		    match = strdup(argv[++i]);
		else {
		    fprintf(stderr, "%s: %s: Missing required match argument\n",
			    argv0, argv[i]);
		    exit(1);
		}
		goto NextArg;
		
	      case 'D':
		if (argv[i][j+1])
		    ypdom = strdup(argv[i]+j+1);
		else if (argv[i+1])
		    ypdom = strdup(argv[++i]);
		else {
		    fprintf(stderr, "%s: %s: Missing required domain argument\n",
			    argv0, argv[i]);
		    exit(1);
		}
		goto NextArg;
		
	      default:
		fprintf(stderr, "%s: -%c: Invalid command line option\n", argv[0], argv[i][j]);
		exit(1);
	    }
      NextArg:;
    }

    if (verbose)
	fprintf(stderr, "[netgr, version 1.0 - Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>]\n");
    
    if (ypdom == NULL) {
	rc = yp_get_default_domain(&ypdom);
	if (rc != 0) {
	    fprintf(stderr, "%s: yp_get_default_domain(): %s\n",
		    argv[0], yperr_string(rc));
	    exit(1);
	}
    }
    
    if (i >= argc) {
	struct ypall_callback icb;
	
	icb.foreach = (int (*)()) yp_foreach;
	icb.data = NULL;
	
	rc = yp_all(ypdom, ypmap, &icb);
	if (rc) {
	    fprintf(stderr, "%s: yp_all(): %s\n", argv[0], yperr_string(rc));
	    return 1;
	}

	list_sort(all_groups, g_compare);
    
	if (!p_groups && !p_nodes)
	    p_groups = 1;
    } else {
	for (; i < argc; i++) {
	    List *groups = list_new(0);
	    List *nodes = list_new(0);
	    
	    if (netgr_lookup(groups, nodes, argv[i]) == NULL)
		rc = -1;
	}
	if (!p_groups && !p_nodes)
	    p_nodes = 1;
    }

    list_sort(all_nodes, n_compare);
    
    for (j = 0; j < all_groups->c; j++) {
	GROUP *gp = all_groups->v[j];
	if (gp->nodes->c > 0) {
	    n_groups++;
	    if (p_groups)
		puts(gp->name);
	}
    }
    
    if (p_nodes) {
	for (j = 0; j < all_nodes->c; j++) {
	    NODE *np = all_nodes->v[j];
	    
	    printf("\t(%s,%s,%s)\n",
		   np->host,
		   np->user,
		   np->domain);
	}
    }

    if (verbose)
	fprintf(stderr, "[%u group%s & %u node%s]\n",
		n_groups, n_groups == 1 ? "" : "s",
		all_nodes->c, all_nodes->c == 1 ? "" : "s");
		
    if (n_errors > 0 || (match && all_nodes->c == 0))
	return 1;
    
    return 0;
}

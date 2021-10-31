/*
 * setnodename
 *
 * Version 1.1 2006-11-20
 *
 * Copyright by Schlomo Schapiro <setnodename@schlomo.schapiro.org>
 * Licensed under the GNU Public License, see the 
 * full text under http://www.gnu.org/licenses/gpl.html
 *
 * See http://www.schapiro.org/schlomo for the latest version.
 *
 * Purpose: 
 * --------
 * Make programs believe that they are running on a different host
 * by returning a different nodename (uname -n). Used mainly in a chroot
 * environment to make the software running inside appear as a different
 * system to itself and to the outside.
 *
 * For a full implementation of this concept one should be using Linux
 * vserver or OpenVZ or s.th. similar. This solution is only a quick hack
 * to help in chroot environments where the full solution is not available.
 *
 * Compilation & Installation:
 * ---------------------------
 * gcc -fPIC -rdynamic -g -c -Wall setnodename.c
 * gcc -shared -Wl,-soname,libsetnodename.so.1 -o libsetnodename.so.1 setnodename.o -lc -ldl
 * 
 * Copy the resulting libsetnodename.so.1 to one of your library directories
 * (e.g. /usr/lib)
 *
 * To test run this line:
 * # LD_PRELOAD=libsetnodename.so.1 SETNODENAME=some-other-node uname -n
 *
 * The result should be:
 * some-other-node
 *
 * Usage:
 * ------
 * To use setnodename you have to set (and export) these environment variables:
 * 
 * export LD_PRELOAD=libsetnodename.so.1 SETNODENAME=some-other-node
 * 
 * Any programs run with these variables will then retrieve some-other-node as the
 * nodename whenever they do a uname(2) call. For a system-wide effect (e.g. in the
 * chroot environment) you might want to add this line to your favourite system
 * configuration files like /etc/profile, /etc/profile.d/setnodename.sh or 
 * /etc/bash.bashrc
 * (and likewise for users of tcsh)
 *
 * Set the SETNODENAME_DEBUG variable to something to get debugging output.
 * Support:
 * --------
 *
 * Read the source (see below) and fix any bugs and send it to me :-)
 *
 * Regards,
 * Schlomo Schapiro
 *
 * VERSION HISTORY:
 * 2006-11-21	1.0	Initial Version
 * 2006-11-23	1.1	Added gethostname and debugging
 *
 */

#include <sys/utsname.h>
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void *libc_handle;
void *uname_handle;
int (*orig_uname)(struct utsname*);
char mynodename[65535] = "schlomo";
char *newnodename = NULL;
struct utsname *real_uname;
int debug = 0;

int initialize_replacement_functions() {
	char *tmpstring;
	int ret;

	libc_handle=dlopen("/lib/libc.so.6",RTLD_LAZY);
	if (!libc_handle) {
		fputs(dlerror(), stderr);
		exit(EXIT_FAILURE);
	}

	orig_uname=dlsym(libc_handle,"uname");
	if ((tmpstring=dlerror()) != NULL) {
		fprintf(stderr,"%s\n",tmpstring);
		exit(EXIT_FAILURE);
	}

	ret=orig_uname(real_uname);
	if (!ret) {
		fputs("ERROR: Could not get real uname !\n",stderr);
		exit(EXIT_FAILURE);
	}
	
	tmpstring=getenv("SETNODENAME");
	if (tmpstring == NULL) {
		fputs("ERROR: Environment variable SETNODENAME is not set !\n",stderr);
		strncpy(mynodename,real_uname->nodename,65535);
	} else {
		strncpy(mynodename,tmpstring,65535);
	}
	newnodename=mynodename;
	debug= (getenv("SETNODENAME_DEBUG")!=NULL);
	if (debug) {
		fprintf(stderr,"INFO: Setting new hostname '%s'\n",newnodename);
	}
	return(ret);
}

int gethostname(char *name, size_t len) {
	if (newnodename == NULL) {
		initialize_replacement_functions();
	}
	strncpy(name,newnodename,len);
	if (debug) {
		fprintf(stderr,"INFO: Replaced gethostname() function\n");
	}
	return(0);
}

int uname(struct utsname *buf) {
	int ret;
	if (newnodename == NULL) {
		initialize_replacement_functions();
	}
	ret=orig_uname(buf);
	strncpy(buf->nodename,newnodename,_UTSNAME_NODENAME_LENGTH);
	if (debug) {
		fprintf(stderr,"INFO: Replaced uname() function\n");
	}
	return(ret);
}

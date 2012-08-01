/*
 * sample yahoo client based on libyahoo2
 *
 * libyahoo2 is available at http://libyahoo2.sourceforge.net/
 *
 * $Revision: 373 $
 * $Date: 2010-06-02 13:55:25 -0700 (Wed, 02 Jun 2010) $
 * 
 * Copyright (C) 2002-2004, Philip S Tellis <philip.tellis AT gmx.net>
 * Copyright (C) 2009, Siddhesh Poyarekar <siddhesh.poyarekar@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#ifndef _WIN32
#include <netdb.h>
#include <sys/time.h>
#endif
#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <termios.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <openssl/ssl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Get these from http://libyahoo2.sourceforge.net/ */
#include <yahoo2.h>
#include <yahoo2_callbacks.h>
#include "yahoo_util.h"

#ifndef _WIN32
int fileno(FILE * stream);
#endif

#define MAX_PREF_LEN 255

static char *local_host = NULL;

static int do_mail_notify = 0;
static int do_yahoo_debug = 0;
static int ignore_system = 0;
static int do_typing_notify = 1;
static int accept_webcam_viewers = 1;
static int send_webcam_images = 0;
static int webcam_direction = YAHOO_WEBCAM_DOWNLOAD;
static time_t curTime = 0;
static time_t pingTimer = 0;
static time_t webcamTimer = 0;
static double webcamStart = 0;

/* id of the webcam connection (needed for uploading) */
static int webcam_id = 0;

static int poll_loop=1;

static void register_callbacks();

typedef struct {
	char yahoo_id[255];
	char password[255];
	int id;
	int fd;
	int status;
	char *msg;
} yahoo_local_account;

typedef struct {
	char yahoo_id[255];
	char name[255];
	int status;
	int away;
	char *msg;
	char group[255];
} yahoo_account;

typedef struct {
	int id;
	char *label;
} yahoo_idlabel;

typedef struct {
	int id;
	char *who;
} yahoo_authorize_data;

yahoo_idlabel yahoo_status_codes[] = {
	{YAHOO_STATUS_AVAILABLE, "Available"},
	{YAHOO_STATUS_BRB, "BRB"},
	{YAHOO_STATUS_BUSY, "Busy"},
	{YAHOO_STATUS_NOTATHOME, "Not Home"},
	{YAHOO_STATUS_NOTATDESK, "Not at Desk"},
	{YAHOO_STATUS_NOTINOFFICE, "Not in Office"},
	{YAHOO_STATUS_ONPHONE, "On Phone"},
	{YAHOO_STATUS_ONVACATION, "On Vacation"},
	{YAHOO_STATUS_OUTTOLUNCH, "Out to Lunch"},
	{YAHOO_STATUS_STEPPEDOUT, "Stepped Out"},
	{YAHOO_STATUS_INVISIBLE, "Invisible"},
	{YAHOO_STATUS_IDLE, "Idle"},
	{YAHOO_STATUS_OFFLINE, "Offline"},
	{YAHOO_STATUS_CUSTOM, "[Custom]"},
	{YPACKET_STATUS_NOTIFY, "Notify"},
	{0, NULL}
};

static void ext_yahoo_close(void *fd);

char * yahoo_status_code(enum yahoo_status s)
{
	int i;
	for(i=0; yahoo_status_codes[i].label; i++)
		if(yahoo_status_codes[i].id == s)
			return yahoo_status_codes[i].label;
	return "Unknown";
}

#define YAHOO_DEBUGLOG ext_yahoo_log

static int ext_yahoo_log(const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);
	fflush(stderr);
	va_end(ap);
	return 0;
}

#define LOG(x) if(do_yahoo_debug) { YAHOO_DEBUGLOG("%s:%d: ", __FILE__, __LINE__); \
	YAHOO_DEBUGLOG x; \
	YAHOO_DEBUGLOG("\n"); }

#define WARNING(x) if(do_yahoo_debug) { YAHOO_DEBUGLOG("%s:%d: warning: ", __FILE__, __LINE__); \
	YAHOO_DEBUGLOG x; \
	YAHOO_DEBUGLOG("\n"); }

#define print_message(x) { printf x; printf("\n"); }

static yahoo_local_account * ylad = NULL;
static YList * buddies = NULL;

static int expired(time_t timer)
{
	if (timer && curTime >= timer)
		return 1;

	return 0;
}

static void rearm(time_t *timer, int seconds)
{
	time(timer);
	*timer += seconds;
}

#ifndef _WIN32
FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);
int gethostname(char *name, size_t len);
#endif

static char * get_local_addresses()
{
	static char addresses[1024];
	char buff[1024];
	struct hostent * hn;
#ifndef _WIN32
	char gateway[16];
	char  * c;
	int i;
	FILE * f;
	f = popen("netstat -nr", "r");
	if(!f)
			goto IP_TEST_2;
	while( fgets(buff, sizeof(buff), f)  != NULL ) {
			c = strtok( buff, " " );
			if( (strstr(c, "default") || strstr(c,"0.0.0.0") ) &&
							!strstr(c, "127.0.0" ) )
					break;
	}
	c = strtok( NULL, " " );
	pclose(f);

	strncpy(gateway,c, 16);



	for(i = strlen(gateway); gateway[i] != '.'; i-- )
		gateway[i] = 0;

	gateway[i] = 0;

	for(i = strlen(gateway); gateway[i] != '.'; i-- )
		gateway[i] = 0;

	f = popen("/sbin/ifconfig -a", "r");
	if(!f)
		goto IP_TEST_2;

	while( fgets(buff, sizeof(buff), f) != NULL ) {
		if( strstr(buff, "inet") && strstr(buff,gateway) )
			break;
	}
	pclose(f);

	c = strtok( buff, " " );
	c = strtok( NULL, " " );

	strncpy ( addresses, c, sizeof(addresses) );
	c = strtok(addresses, ":" );
	strncpy ( buff, c, sizeof(buff) );
	if((c=strtok(NULL, ":")))
		strncpy( buff, c, sizeof(buff) );

	strncpy(addresses, buff, sizeof(addresses));

	return addresses;
		
		
IP_TEST_2:
#endif /* _WIN32 */

	gethostname(buff,sizeof(buff));

	hn = gethostbyname(buff);
	if(hn)
		strncpy(addresses, inet_ntoa( *((struct in_addr*)hn->h_addr)), sizeof(addresses) );
	else
		addresses[0] = 0;

	return addresses;
}

static double get_time()
{
#ifndef _WIN32
	struct timeval ct;
	gettimeofday(&ct, 0);

	/* return time in milliseconds */
	return (ct.tv_sec * 1E3 + ct.tv_usec / 1E3);
#else
	return timeGetTime();
#endif
}

static int yahoo_ping_timeout_callback()
{
	print_message(("Sending a keep alive message"));
	yahoo_keepalive(ylad->id);
	rearm(&pingTimer, 600);
	return 1;
}

static int yahoo_webcam_timeout_callback(int id)
{
	static unsigned image_num = 0;
	unsigned char *image = NULL;
	unsigned int length = 0;
	unsigned int timestamp = get_time() - webcamStart;
	char fname[1024];
	FILE *f_image = NULL;
	struct stat s_image;

	if (send_webcam_images)
	{
		sprintf(fname, "images/image_%.3d.jpc", image_num++);
		if (image_num > 999) image_num = 0;
		if (stat(fname, &s_image) == -1)
			return -1;
		length = s_image.st_size;
		image = y_new0(unsigned char, length);

		if ((f_image = fopen(fname, "r")) != NULL) {
			fread(image, length, 1, f_image);
			fclose(f_image);
		} else {
			printf("Error reading from %s\n", fname);
		}
	}

	print_message(("Sending a webcam image (%d bytes)", length));
	yahoo_webcam_send_image(id, image, length, timestamp);
	FREE(image);
	rearm(&webcamTimer, 2);
	return 1;
}

YList * conferences = NULL;
typedef struct {
	int id;
	char * me;
	char * room_name;
	char * host;
	YList * members;
	int joined;
} conf_room;

static const char * get_buddy_name(const char * yid)
{
	YList * b;
	for (b = buddies; b; b = b->next) {
		yahoo_account * ya = b->data;
		if(!strcmp(yid, ya->yahoo_id))
			return ya->name&&*ya->name?ya->name:ya->yahoo_id;
	}

	return yid;
}

static conf_room * find_conf_room_by_name_and_id(int id, const char * me, const char * name)
{
	YList * l;
	for(l = conferences; l; l=l->next) {
		conf_room * cr = l->data;
		if(cr->id == id && !strcmp(name, cr->room_name) && (me == NULL || cr->me == NULL || !strcmp(me, cr->me))) {
			return cr;
		}
	}

	return NULL;
}

static void ext_yahoo_got_conf_invite(int id, const char *me, const char *who, const char *room, const char *msg, YList *members)
{
	conf_room * cr = y_new0(conf_room, 1);
	cr->room_name = strdup(room);
	cr->me = strdup(me);
	cr->host = strdup(who);
	cr->members = members;
	cr->id = id;

	conferences = y_list_append(conferences, cr);

	print_message(("%s has invited you [%s] to a conference: %s\n"
				"with the message: %s",
				who, me, room, msg));

	for(; members; members=members->next)
		print_message(("\t%s", (char *)members->data));
	
}
static void ext_yahoo_conf_userdecline(int id, const char *me, const char *who, const char *room, const char *msg)
{
	YList * l;
	/* TODO: probably have to use the me arg to find the room */
	conf_room * cr = find_conf_room_by_name_and_id(id, me, room);
	
	if(cr)
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who)) {
			FREE(l->data);
			cr->members = y_list_remove_link(cr->members, l);
			y_list_free_1(l);
			break;
		}
	}

	print_message(("%s declined the invitation from %s to %s\n"
				"with the message: %s", who, me, room, msg));
}

static void ext_yahoo_conf_userjoin(int id, const char *me, const char *who, const char *room)
{
	conf_room * cr = find_conf_room_by_name_and_id(id, me, room);
	if(cr) {
	YList * l = NULL;
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who))
			break;
	}
	if(!l)
		cr->members = y_list_append(cr->members, strdup(who));
	}

	print_message(("%s joined the conference %s [%s]", who, room, me));

}

static void ext_yahoo_conf_userleave(int id, const char *me, const char *who, const char *room)
{
	YList * l;
	conf_room * cr = find_conf_room_by_name_and_id(id, me, room);
	
	if(cr)
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who)) {
			FREE(l->data);
			cr->members = y_list_remove_link(cr->members, l);
			y_list_free_1(l);
			break;
		}
	}

	print_message(("%s left the conference %s [%s]", who, room, me));
}

static void ext_yahoo_conf_message(int id, const char *me, const char *who, const char *room, const char *msg, int utf8)
{
	char * umsg = (char *)msg;

	if(utf8)
		umsg = y_utf8_to_str(msg);

	who = get_buddy_name(who);

	print_message(("%s (in %s [%s]): %s", who, room, me, umsg));

	if(utf8)
		FREE(umsg);
}

static void print_chat_member(struct yahoo_chat_member *ycm) 
{
	printf("%s (%s) ", ycm->id, ycm->alias);
	printf(" Age: %d Sex: ", ycm->age);
	if (ycm->attribs & YAHOO_CHAT_MALE) {
		printf("Male");
	} else if (ycm->attribs & YAHOO_CHAT_FEMALE) {
		printf("Female");
	} else {
		printf("Unknown");
	}
	if (ycm->attribs & YAHOO_CHAT_WEBCAM) {
		printf(" with webcam");
	}

	printf("  Location: %s", ycm->location);
}

static void ext_yahoo_chat_cat_xml(int id, const char *xml) 
{
	print_message(("%s", xml));
}

static void ext_yahoo_chat_join(int id, const char *me, const char *room, const char * topic, YList *members, void *fd)
{
	print_message(("You [%s] have joined the chatroom %s with topic %s", me, room, topic));

	while(members) {
		YList *n = members->next;

		printf("\t");
		print_chat_member(members->data);
		printf("\n");
		FREE(((struct yahoo_chat_member *)members->data)->id);
		FREE(((struct yahoo_chat_member *)members->data)->alias);
		FREE(((struct yahoo_chat_member *)members->data)->location);
		FREE(members->data);
		FREE(members);
		members=n;
	}
}

static void ext_yahoo_chat_userjoin(int id, const char *me, const char *room, struct yahoo_chat_member *who)
{
	print_chat_member(who);
	print_message((" joined the chatroom %s [%s]", room, me));
	FREE(who->id);
	FREE(who->alias);
	FREE(who->location);
	FREE(who);
}

static void ext_yahoo_chat_userleave(int id, const char *me, const char *room, const char *who)
{
	print_message(("%s left the chatroom %s [%s]", who, room, me));
}

static void ext_yahoo_chat_message(int id, const char *me, const char *who, const char *room, const char *msg, int msgtype, int utf8)
{
	char * umsg = (char *)msg;
	char * charpos;

	if(utf8)
		umsg = y_utf8_to_str(msg);
	/* Remove escapes */
	charpos = umsg;
	while(*charpos) {
		if (*charpos == 0x1b) {
			*charpos = ' ';
		}
		charpos++;
	}

	if (msgtype == 2) {
		print_message(("(in %s [%s]) %s %s", room, me, who, umsg));
	} else {
		print_message(("(in %s [%s]) %s: %s", room, me, who, umsg));
	}

	if(utf8)
		FREE(umsg);
}

static void ext_yahoo_status_changed(int id, const char *who, int stat, const char *msg, int away, int idle, int mobile)
{
	yahoo_account * ya=NULL;
	YList * b;
	char buf[1024];

	for(b = buddies; b; b = b->next) {
		if(!strcmp(((yahoo_account *)b->data)->yahoo_id, who)) {
			ya = b->data;
			break;
		}
	}
	
	if (msg == NULL) {
		sprintf(buf, "%s", yahoo_status_code(stat));
	}
	else if (stat == YAHOO_STATUS_CUSTOM) {
		sprintf(buf, "%s", msg);
	} else {
		sprintf(buf, "%s: %s", yahoo_status_code(stat), msg);
	}

	if (away > 0) {
		char away_buf[32];
		sprintf(away_buf, " away[%d]", away);
		strcat(buf, away_buf);
	}

	if (mobile > 0) {
		char mobile_buf[32];
		sprintf(mobile_buf, " mobile[%d]", mobile);
		strcat(buf, mobile_buf);
	}

	if (idle > 0) {
		char time_buf[32];
		sprintf(time_buf, " idle for %d:%02d:%02d", idle/3600, (idle/60)%60, idle%60);
		strcat(buf, time_buf);
	}

	print_message(("%s (%s) is now %s", ya?ya->name:who, who, buf))

	if(ya) {
		ya->status = stat;
		ya->away = away;
		if(msg) {
			FREE(ya->msg);
			ya->msg = strdup(msg);
		}
	}
}

static void ext_yahoo_got_buddies(int id, YList * buds)
{
	while(buddies) {
		FREE(buddies->data);
		buddies = buddies->next;
		if(buddies)
			FREE(buddies->prev);
	}
	for(; buds; buds = buds->next) {
		yahoo_account *ya = y_new0(yahoo_account, 1);
		struct yahoo_buddy *bud = buds->data;
		strncpy(ya->yahoo_id, bud->id, 255);
		if(bud->real_name)
			strncpy(ya->name, bud->real_name, 255);
		strncpy(ya->group, bud->group, 255);
		ya->status = YAHOO_STATUS_OFFLINE;
		buddies = y_list_append(buddies, ya);

/*		print_message(("%s is %s", bud->id, bud->real_name));*/
	}
}

static void ext_yahoo_got_ignore(int id, YList * igns)
{
}

static void ext_yahoo_got_buzz(int id, const char *me, const char *who, long tm)
{
	who = get_buddy_name(who);

	printf("\a");
	if(tm) {
		char timestr[255];

		strncpy(timestr, ctime((time_t *)&tm), sizeof(timestr));
		timestr[strlen(timestr) - 1] = '\0';

		print_message(("[Offline message at %s to %s from %s]: **DING**",
				timestr, me, who))
	} else
		print_message(("[%s]%s: **DING**", me, who))
}

static void ext_yahoo_got_im(int id, const char *me, const char *who, const char *msg, long tm, int stat, int utf8)
{
	char *umsg = (char *)msg;

	if(stat == 2) {
		LOG(("Error sending message from %s to %s", me, who));
		return;
	}

	if(!msg)
		return;

	if(utf8)
		umsg = y_utf8_to_str(msg);
	
	who = get_buddy_name(who);

	if(tm) {
		char timestr[255];

		strncpy(timestr, ctime((time_t *)&tm), sizeof(timestr));
		timestr[strlen(timestr) - 1] = '\0';

		print_message(("[Offline message at %s to %s from %s]: %s", 
				timestr, me, who, umsg))
	} else
		print_message(("[%s]%s: %s", me, who, umsg))

	if(utf8)
		FREE(umsg);
}

static void ext_yahoo_rejected(int id, const char *who, const char *msg)
{
	print_message(("%s has rejected you%s%s", who, 
				(msg?" with the message:\n":"."), 
				(msg?msg:"")));
}

static void ext_yahoo_contact_added(int id, const char *myid, const char *who, const char *msg)
{
	char buff[1024];

	snprintf(buff, sizeof(buff), "%s, the yahoo user %s has added you to their contact list", myid, who);
	if(msg) {
		strcat(buff, " with the following message:\n");
		strcat(buff, msg);
		strcat(buff, "\n");
	} else {
		strcat(buff, ".  ");
	}
	strcat(buff, "Do you want to allow this [Y/N]?");

/*	print_message((buff));
	scanf("%c", &choice);
	if(choice != 'y' && choice != 'Y')
		yahoo_reject_buddy(id, who, "Thanks, but no thanks.");
*/
}

static void ext_yahoo_typing_notify(int id, const char* me, const char *who, int stat)
{
	if(stat && do_typing_notify)
		print_message(("[%s]%s is typing...", me, who));
}

static void ext_yahoo_game_notify(int id, const char *me, const char *who, int stat, const char *msg)
{
}

static void ext_yahoo_mail_notify(int id, const char *from, const char *subj, int cnt)
{
	char buff[1024] = {0};
	
	if(!do_mail_notify)
		return;

	if(from && subj)
		snprintf(buff, sizeof(buff), 
				"You have new mail from %s about %s\n", 
				from, subj);
	if(cnt) {
		char buff2[100];
		snprintf(buff2, sizeof(buff2), 
				"You have %d message%s\n", 
				cnt, cnt==1?"":"s");
		strcat(buff, buff2);
	}

	if(buff[0])
		print_message((buff));
}

static void ext_yahoo_got_webcam_image(int id, const char *who,
		const unsigned char *image, unsigned int image_size, unsigned int real_size,
		unsigned int timestamp)
{
	static unsigned char *cur_image = NULL;
	static unsigned int cur_image_len = 0;
	static unsigned int image_num = 0;
	FILE* f_image;
	char fname[1024];

	/* copy image part to cur_image */
	if (real_size)
	{
		if (!cur_image) cur_image = y_new0(unsigned char, image_size);
		memcpy(cur_image + cur_image_len, image, real_size);
		cur_image_len += real_size;
	}

	if (image_size == cur_image_len)
	{
		print_message(("Received a image update at %d (%d bytes)",
			 timestamp, image_size));

		/* if we recieved an image then write it to file */
		if (image_size)
		{
			sprintf(fname, "images/%s_%.3d.jpc", who, image_num++);

			if ((f_image = fopen(fname, "w")) != NULL) {
				fwrite(cur_image, image_size, 1, f_image);
				fclose(f_image);
			} else {
				printf("Error writing to %s\n", fname);
			}
			FREE(cur_image);
			cur_image_len = 0;
			if (image_num > 999) image_num = 0;
		}
	}
}

static void ext_yahoo_webcam_viewer(int id, const char *who, int connect)
{
	switch (connect)
	{
		case 0:
			print_message(("%s has stopped viewing your webcam", who));
			break;
		case 1:
			print_message(("%s has started viewing your webcam", who));
			break;
		case 2:
			print_message(("%s is trying to view your webcam", who));
			yahoo_webcam_accept_viewer(id, who, accept_webcam_viewers);
			break;
	}
}

static void ext_yahoo_webcam_closed(int id, const char *who, int reason)
{
	switch(reason)
	{
		case 1:
			print_message(("%s stopped broadcasting", who));
			break;
		case 2:
			print_message(("%s cancelled viewing permission", who));
			break;
		case 3:
			print_message(("%s declines permission to view his/her webcam", who));
			break;
		case 4:
			print_message(("%s does not have his/her webcam online", who));
			break;
	}
}

static void ext_yahoo_webcam_data_request(int id, int send)
{
	webcam_id = id;

	if (send) {
		print_message(("Got request to start sending images"));
		if (!webcamTimer)
			rearm(&webcamTimer, 2);
	} else {
		print_message(("Got request to stop sending images"));
	}
	send_webcam_images = send;
}

static void ext_yahoo_webcam_invite(int id, const char *me, const char *from)
{
	print_message(("Got a webcam invitation to %s from %s", me, from));
}

static void ext_yahoo_webcam_invite_reply(int id, const char *me, const char *from, int accept)
{
	if(accept) {
		print_message(("[%s]%s accepted webcam invitation...", me, from));
	} else {
		print_message(("[%s]%s declined webcam invitation...", me, from));
	}
}

static void ext_yahoo_system_message(int id, const char *me, const char *who, const char *msg)
{
	if(ignore_system)
		return;

	print_message(("Yahoo System Message: %s", msg));
}

void yahoo_logout()
{
	if (ylad->id <= 0) {
		return;
	}

	pingTimer=0;

	while(conferences) {
		YList * n = conferences->next;
		conf_room * cr = conferences->data;
		if(cr->joined)
			yahoo_conference_logoff(ylad->id, NULL, cr->members, cr->room_name);
		FREE(cr->me);
		FREE(cr->room_name);
		FREE(cr->host);
		while(cr->members) {
			YList *n = cr->members->next;
			FREE(cr->members->data);
			FREE(cr->members);
			cr->members=n;
		}
		FREE(cr);
		FREE(conferences);
		conferences = n;
	}
	
	yahoo_logoff(ylad->id);
	yahoo_close(ylad->id);

	ylad->status = YAHOO_STATUS_OFFLINE;
	ylad->id = 0;

	poll_loop=0;

	print_message(("logged_out"));
}

static void ext_yahoo_login(yahoo_local_account * ylad, int login_mode)
{
	LOG(("ext_yahoo_login"));

	ylad->id = yahoo_init_with_attributes(ylad->yahoo_id, ylad->password, 
			"local_host", local_host,
			"pager_port", 5050,
			NULL);
	ylad->status = YAHOO_STATUS_OFFLINE;
	yahoo_login(ylad->id, login_mode);

/*	if (ylad->id <= 0) {
		print_message(("Could not connect to Yahoo server.  Please verify that you are connected to the net and the pager host and port are correctly entered."));
		return;
	}
*/
	rearm(&pingTimer, 600);
}

static void ext_yahoo_got_cookies(int id)
{
	/*yahoo_get_yab(id);*/
}

static void ext_yahoo_login_response(int id, int succ, const char *url)
{
	char buff[1024];

	if(succ == YAHOO_LOGIN_OK) {
		ylad->status = yahoo_current_status(id);
		print_message(("logged in"));
		return;
		
	} else if(succ == YAHOO_LOGIN_UNAME) {

		snprintf(buff, sizeof(buff), "Could not log into Yahoo service - username not recognised.  Please verify that your username is correctly typed.");
	} else if(succ == YAHOO_LOGIN_PASSWD) {

		snprintf(buff, sizeof(buff), "Could not log into Yahoo service - password incorrect.  Please verify that your password is correctly typed.");

	} else if(succ == YAHOO_LOGIN_LOCK) {
		
		snprintf(buff, sizeof(buff), "Could not log into Yahoo service.  Your account has been locked.\nVisit %s to reactivate it.", url);

	} else if(succ == YAHOO_LOGIN_DUPL) {

		snprintf(buff, sizeof(buff), "You have been logged out of the yahoo service, possibly due to a duplicate login.");
	} else if(succ == YAHOO_LOGIN_SOCK) {

		snprintf(buff, sizeof(buff), "The server closed the socket.");
	} else {
		snprintf(buff, sizeof(buff), "Could not log in, unknown reason: %d.", succ);
	}

	ylad->status = YAHOO_STATUS_OFFLINE;
	print_message((buff));
	yahoo_logout();
	poll_loop=0;
}

static void ext_yahoo_error(int id, const char *err, int fatal, int num)
{
	fprintf(stdout, "Yahoo Error: ");
	fprintf(stdout, "%s", err);
	switch(num) {
		case E_UNKNOWN:
			fprintf(stdout, "unknown error %s", err);
			break;
		case E_CUSTOM:
			fprintf(stdout, "custom error %s", err);
			break;
		case E_CONFNOTAVAIL:
			fprintf(stdout, "%s is not available for the conference", err);
			break;
		case E_IGNOREDUP:
			fprintf(stdout, "%s is already ignored", err);
			break;
		case E_IGNORENONE:
			fprintf(stdout, "%s is not in the ignore list", err);
			break;
		case E_IGNORECONF:
			fprintf(stdout, "%s is in buddy list - cannot ignore ", err);
			break;
		case E_SYSTEM:
			fprintf(stdout, "system error %s", err);
			break;
		case E_CONNECTION:
			fprintf(stdout, "server connection error %s", err);
			break;
	}
	fprintf(stdout, "\n");
	if(fatal)
		yahoo_logout();
}

void yahoo_set_current_state(int yahoo_state)
{
	if (ylad->status == YAHOO_STATUS_OFFLINE && yahoo_state != YAHOO_STATUS_OFFLINE) {
		ext_yahoo_login(ylad, yahoo_state);
		return;
	} else if (ylad->status != YAHOO_STATUS_OFFLINE && yahoo_state == YAHOO_STATUS_OFFLINE) {
		yahoo_logout();
		return;
	}

	ylad->status = yahoo_state;
	if(yahoo_state == YAHOO_STATUS_CUSTOM) {
		if(ylad->msg)
			yahoo_set_away(ylad->id, yahoo_state, ylad->msg, 1);
		else
			yahoo_set_away(ylad->id, yahoo_state, "delta p * delta x too large", 1);
	} else
		yahoo_set_away(ylad->id, yahoo_state, NULL, 1);
}

static int ext_yahoo_connect(const char *host, int port)
{
	WARNING(("This should not be used anymore. File a bug report."));
	return -1;
}

/*************************************
 * Callback handling code starts here
 */
YList *connections = NULL;
struct _conn {
	int fd;
	SSL *ssl;
	int use_ssl;
	int remove;
};

struct conn_handler {
	struct _conn *con;
	int id;
	int tag;
	yahoo_input_condition cond;
	int remove;
	void *data;
};

static int connection_tags=0;

static int ext_yahoo_add_handler(int id, void *d, yahoo_input_condition cond, void *data)
{
	struct conn_handler *h = y_new0(struct conn_handler, 1);

	h->id = id;
	h->tag = ++connection_tags;
	h->con = d;
	h->cond = cond;
	h->data = data;

	LOG(("Add %d(%d) for %d, tag %d", h->con->fd, cond, id, h->tag));

	connections = y_list_prepend(connections, h);

	return h->tag;
}

static void ext_yahoo_remove_handler(int id, int tag)
{
	YList *l;
	if (!tag)
		return;

	for(l = connections; l; l = y_list_next(l)) {
		struct conn_handler *c = l->data;
		if(c->tag == tag) {
			/* don't actually remove it, just mark it for removal */
			/* we'll remove when we start the next poll cycle */
			LOG(("Marking id:%d fd:%p tag:%d for removal",
				c->id, c->con, c->tag));
			c->remove = 1;
			return;
		}
	}
}

static SSL *do_ssl_connect(int fd)
{
	SSL *ssl;
	SSL_CTX *ctx;

	LOG(("SSL Handshake"));

	SSL_library_init ();
	ctx = SSL_CTX_new(SSLv23_client_method());
	ssl = SSL_new(ctx);
	SSL_CTX_free(ctx);
	SSL_set_fd(ssl, fd);

	if (SSL_connect(ssl) == 1)
		return ssl;

	return NULL;
}

struct connect_callback_data {
	yahoo_connect_callback callback;
	void * callback_data;
	int id;
	int tag;
};

static void connect_complete(void *data, struct _conn *source, yahoo_input_condition condition)
{
	struct connect_callback_data *ccd = data;
	int error, err_size = sizeof(error);

	ext_yahoo_remove_handler(0, ccd->tag);
	getsockopt(source->fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&err_size);

	if(error)
		goto err;

	LOG(("Connected fd: %d, error: %d", source->fd, error));

	if (source->use_ssl) {
		source->ssl = do_ssl_connect(source->fd);

		if (!source->ssl) {
err:
			LOG(("SSL Handshake Failed!"));
			ext_yahoo_close(source);

			ccd->callback(NULL, 0, ccd->callback_data);
			FREE(ccd);
			return;
		}
	}

	fcntl(source->fd, F_SETFL, O_NONBLOCK);

	ccd->callback(source, error, ccd->callback_data);
	FREE(ccd);
}

void yahoo_callback(struct conn_handler *c, yahoo_input_condition cond)
{
	int ret=1;
	char buff[1024]={0};

	if(c->id < 0) {
		connect_complete(c->data, c->con, cond);
	} else {
		if(cond & YAHOO_INPUT_READ)
			ret = yahoo_read_ready(c->id, c->con, c->data);
		if(ret>0 && cond & YAHOO_INPUT_WRITE)
			ret = yahoo_write_ready(c->id, c->con, c->data);

		if(ret == -1)
			snprintf(buff, sizeof(buff), 
				"Yahoo read error (%d): %s", errno, strerror(errno));
		else if(ret == 0)
			snprintf(buff, sizeof(buff), 
				"Yahoo read error: Server closed socket");

		if(buff[0])
			print_message((buff));
	}
}

static int ext_yahoo_write(void *fd, char *buf, int len)
{
	struct _conn *c = fd;

	if (c->use_ssl)
		return SSL_write(c->ssl, buf, len);
	else
		return write(c->fd, buf, len);
}

static int ext_yahoo_read(void *fd, char *buf, int len)
{
	struct _conn *c = fd;

	if (c->use_ssl)
		return SSL_read(c->ssl, buf, len);
	else
		return read(c->fd, buf, len);
}

static void ext_yahoo_close(void *fd)
{
	struct _conn *c = fd;
	YList *l;

	if (c->use_ssl)
		SSL_free(c->ssl);

	close(c->fd);
	c->fd = 0;

	/* Remove all handlers */
	for (l = connections; l; l = y_list_next(l)) {
		struct conn_handler *h = l->data;

		if (h->con == c)
			h->remove = 1;
	}

	c->remove = 1;
}

static int ext_yahoo_connect_async(int id, const char *host, int port, 
		yahoo_connect_callback callback, void *data, int use_ssl)
{
	struct sockaddr_in serv_addr;
	static struct hostent *server;
	int servfd;
	struct connect_callback_data * ccd;
	int error;
	SSL *ssl = NULL;

	struct _conn *c;

	LOG(("Connecting to %s:%d", host, port));
	
	if(!(server = gethostbyname(host))) {
		errno=h_errno;
		return -1;
	}

	if((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, *server->h_addr_list, server->h_length);
	serv_addr.sin_port = htons(port);

	c = y_new0(struct _conn, 1);
	c->fd = servfd;
	c->use_ssl = use_ssl;

	error = connect(servfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	LOG(("Trying to connect: fd:%d error:%d", servfd, error));
	if(!error) {
		LOG(("Connected"));
		if (use_ssl) {
			ssl = do_ssl_connect(servfd);

			if (!ssl) {
				LOG(("SSL Handshake Failed!"));
				ext_yahoo_close(c);

				callback(NULL, 0, data);
				return -1;
			}
		}

		c->ssl = ssl;
		fcntl(c->fd, F_SETFL, O_NONBLOCK);

		callback(c, 0, data);
		return 0;
	} else if(error == -1 && errno == EINPROGRESS) {
		ccd = calloc(1, sizeof(struct connect_callback_data));
		ccd->callback = callback;
		ccd->callback_data = data;
		ccd->id = id;

		ccd->tag = ext_yahoo_add_handler(-1, c, YAHOO_INPUT_WRITE, ccd);
		return ccd->tag;
	} else {
		if(error == -1)
			LOG(("Connection failure: %s", strerror(errno)));

		ext_yahoo_close(c);

		callback(NULL, errno, data);
		return -1;
	}
}
/*
 * Callback handling code ends here
 ***********************************/

static void process_commands(char *line)
{
	char *cmd, *to, *msg;

	char *tmp, *start;
	char *copy = strdup(line);
	int yid = 0;

	enum yahoo_status state;

	start = cmd = copy;
	tmp = strchr(copy, ' ');
	if(tmp) {
		*tmp = '\0';
		copy = tmp+1;
	} else {
		copy = NULL;
	}

	if(!strncasecmp(cmd, "MSG", strlen("MSG"))) {
		/* send a message */
		to = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		msg = copy;
		if(to && msg) {
			if(!strcmp(msg, "\a"))
				yahoo_send_im(ylad->id, NULL, to, "<ding>", 0,0);
			else {
				msg = y_str_to_utf8(msg);
				yahoo_send_im(ylad->id, NULL, to, msg, 1,0);
				FREE(msg)
			}
		}
	} else if(!strncasecmp(cmd, "CMS", strlen("CMS"))) {
		/* send a message */
		conf_room * cr;
		to = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		msg = copy;
		cr = find_conf_room_by_name_and_id(ylad->id, NULL, to);
		if(!cr) {
			print_message(("no such room: %s", copy));
			goto end_parse;
		}
		if(msg)
			yahoo_conference_message(ylad->id, NULL, cr->members, to, msg, 0);
	} else if(!strncasecmp(cmd, "CLS", strlen("CLS"))) {
		YList * l;
		if(copy) {
			conf_room * cr = find_conf_room_by_name_and_id(ylad->id, NULL, copy);
			if(!cr) {
				print_message(("no such room: %s", copy));
				goto end_parse;
			}
			print_message(("Room: %s", copy));
			for(l = cr->members; l; l=l->next) {
				print_message(("%s", (char *)l->data))
			}
		} else {
			print_message(("All Rooms:"));
			for(l = conferences; l; l=l->next) {
				conf_room * cr = l->data;
				print_message(("%s", cr->room_name));
			}
		}

	} else if(!strncasecmp(cmd, "CCR", strlen("CCR"))) {
		conf_room * cr = y_new0(conf_room, 1);
		while((tmp = strchr(copy, ' ')) != NULL) {
			*tmp = '\0';
			if(!cr->room_name)
				cr->room_name = strdup(copy);
			else
				cr->members = y_list_append(cr->members,
						strdup(copy));
			copy = tmp+1;
		}
		cr->members = y_list_append(cr->members, strdup(copy));

		if(!cr->room_name || !cr->members) {
			FREE(cr);
		} else {
			cr->id = ylad->id;
			cr->joined = 1;
			conferences = y_list_append(conferences, cr);
			yahoo_conference_invite(ylad->id, NULL, cr->members, cr->room_name, "Join my conference");
			cr->members = y_list_append(cr->members,strdup(ylad->yahoo_id));
		}
	} else if(!strncasecmp(cmd, "CIN", strlen("CIN"))) {
		conf_room * cr;
		char * room=copy;
		YList * l1, *l = NULL;

		while((tmp = strchr(copy, ' ')) != NULL) {
			*tmp = '\0';
			copy = tmp+1;
			l = y_list_append(l, copy);
		}

		cr = find_conf_room_by_name_and_id(ylad->id, NULL, room);
		if(!cr) {
			print_message(("no such room: %s", room));
			y_list_free(l);
			goto end_parse;
		}

		for(l1 = l; l1; l1=l1->next) {
			char * w = l1->data;
			yahoo_conference_addinvite(ylad->id, NULL, w, room, cr->members, "Join my conference");
			cr->members = y_list_append(cr->members, strdup(w));
		}
		y_list_free(l);

	} else if(!strncasecmp(cmd, "CLN", strlen("CLN"))) {
		conf_room * cr = find_conf_room_by_name_and_id(ylad->id, NULL, copy);
		YList * l;
		if(!cr) {
			print_message(("no such room: %s", copy));
			goto end_parse;
		}

		cr->joined = 1;
		for(l = cr->members; l; l=l->next) {
			char * w = l->data;
			if(!strcmp(w, ylad->yahoo_id))
				break;
		}
		if(!l)
			cr->members = y_list_append(cr->members, strdup(ylad->yahoo_id));
		yahoo_conference_logon(ylad->id, NULL, cr->members, copy);

	} else if(!strncasecmp(cmd, "CLF", strlen("CLF"))) {
		conf_room * cr = find_conf_room_by_name_and_id(ylad->id, NULL, copy);
		
		if(!cr) {
			print_message(("no such room: %s", copy));
			goto end_parse;
		}

		yahoo_conference_logoff(ylad->id, NULL, cr->members, copy);

		conferences = y_list_remove(conferences, cr);
		FREE(cr->room_name);
		FREE(cr->host);
		while(cr->members) {
			YList *n = cr->members->next;
			FREE(cr->members->data);
			FREE(cr->members);
			cr->members=n;
		}
		FREE(cr);

	} else if(!strncasecmp(cmd, "CDC", strlen("CDC"))) {
		conf_room * cr;
		char * room = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
			msg = copy;
		} else {
			msg = "Thanks, but no thanks!";
		}
		
		cr = find_conf_room_by_name_and_id(ylad->id, NULL, room);
		if(!cr) {
			print_message(("no such room: %s", room));
			goto end_parse;
		}

		yahoo_conference_decline(ylad->id, NULL, cr->members, room,msg);

		conferences = y_list_remove(conferences, cr);
		FREE(cr->room_name);
		FREE(cr->host);
		while(cr->members) {
			YList *n = cr->members->next;
			FREE(cr->members->data);
			FREE(cr->members);
			cr->members=n;
		}
		FREE(cr);


	} else if(!strncasecmp(cmd, "CHL", strlen("CHL"))) {
		int roomid;
		roomid = atoi(copy);
		yahoo_get_chatrooms(ylad->id, roomid);
	} else if(!strncasecmp(cmd, "CHJ", strlen("CHJ"))) {
		char *roomid, *roomname;
/* Linux, FreeBSD, Solaris:1 */
/* 1600326591 */
		roomid = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		roomname = copy;
		if(roomid && roomname) {
			yahoo_chat_logon(ylad->id, NULL, roomname, roomid);
		}

	} else if(!strncasecmp(cmd, "CHM", strlen("CHM"))) {
		char *msg, *roomname;
		roomname = copy;
		tmp = strstr(copy, "  ");
		if(tmp) {
			*tmp = '\0';
			copy = tmp+2;
		}
		msg = copy;
		if(roomname && msg) {
			yahoo_chat_message(ylad->id, NULL, roomname, msg, 1, 0);
		}

	} else if(!strncasecmp(cmd, "CHX", strlen("CHX"))) {
		yahoo_chat_logoff(ylad->id, NULL);
	} else if(!strncasecmp(cmd, "STA", strlen("STA"))) {
		if(isdigit(copy[0])) {
			state = (enum yahoo_status)atoi(copy);
			copy = strchr(copy, ' ');
			if(state == 99) {
				if(copy)
					msg = copy;
				else
					msg = "delta x * delta p too large";
			} else
				msg = NULL;
		} else {
			state = YAHOO_STATUS_CUSTOM;
			msg = copy;
		}

		yahoo_set_away(ylad->id, state, msg, 1);

	} else if(!strncasecmp(cmd, "OFF", strlen("OFF"))) {
		/* go offline */
		printf("Going offline\n");
		poll_loop=0;
	} else if(!strncasecmp(cmd, "IDS", strlen("IDS"))) {
		/* print identities */
		const YList * ids = yahoo_get_identities(ylad->id);
		printf("Identities: ");
		for(; ids; ids = ids->next)
			printf("%s, ", (char *)ids->data);
		printf("\n");
	} else if(!strncasecmp(cmd, "AID", strlen("AID"))) {
		/* activate identity */
		yahoo_set_identity_status(ylad->id, copy, 1);
	} else if(!strncasecmp(cmd, "DID", strlen("DID"))) {
		/* deactivate identity */
		yahoo_set_identity_status(ylad->id, copy, 0);
	} else if(!strncasecmp(cmd, "LST", strlen("LST"))) {
		YList * b = buddies;
		for(; b; b=b->next) {
			yahoo_account * ya = b->data;
			if(ya->status == YAHOO_STATUS_OFFLINE)
				continue;
			if(ya->msg)
				print_message(("%s (%s) is now %s", ya->name, ya->yahoo_id, 
							ya->msg))
			else
				print_message(("%s (%s) is now %s", ya->name, ya->yahoo_id, 
						yahoo_status_code(ya->status)))
		}
	} else if(!strncasecmp(cmd, "NAM", strlen("NAM"))) {
		struct yab * yab;
		
		to = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		yid = atoi(copy);

		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		msg = copy;

		if(to && msg) {
			yab = y_new0(struct yab, 1);
			yab->id = to;
			yab->yid = yid;	/* Only do this if you have got it from the server */
			yab->nname = msg;
			yahoo_set_yab(ylad->id, yab);
			FREE(yab);
		}
	} else if(!strncasecmp(cmd, "WCAM", strlen("WCAM"))) {
		if (copy)
		{
			printf("Viewing webcam (%s)\n", copy);
			webcam_direction = YAHOO_WEBCAM_DOWNLOAD;
			yahoo_webcam_get_feed(ylad->id, copy);
		} else {
			printf("Starting webcam\n");
			webcam_direction = YAHOO_WEBCAM_UPLOAD;
			yahoo_webcam_get_feed(ylad->id, NULL);
		}
	} else if(!strncasecmp(cmd, "WINV", strlen("WINV"))) {
		printf("Inviting %s to view webcam\n", copy);
		yahoo_webcam_invite(ylad->id, copy);
	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
	}

end_parse:
	FREE(start);
}

#ifndef _WIN32
static void local_input_callback(int source)
{
	char line[1024] = {0};
	int i;
	char c;
	i=0; c=0;
	do {
		if(read(source, &c, 1) <= 0)
			c='\0';
		if(c == '\r')
			continue;
		if(c == '\n')
			break;
		if(c == '\b') {
			if(!i)
				continue;
			c = '\0';
			i--;
		}
		if(c) {
			line[i++] = c;
			line[i]='\0';
		}
	} while(i<1023 && c != '\n');

	if(line[0])
		process_commands(line);
}
#else
#include <conio.h>
static void local_input_callback(char c)
{
	static char line[1024] = {0};
	static int line_length = 0;

	if (c == '\b' || (int)c == 127) {
		if (line_length > 0) {
			_cputs("\b \b");
			line_length--;
		}
		return;
	}

	if (c == '\n' || c == '\r' || c == 3) {
		_cputs("\n");
		line[line_length] = 0;
		process_commands(line);
		line_length = 0;
		line[0] = 0;
		return;
	}

	_putch(c);
	line[line_length++] = c;
}
#endif

int main(int argc, char * argv[])
{
	int status;
	int log_level;
	int lfd=0;

	fd_set inp, outp;
	struct timeval tv;

#ifndef _WIN32
	int fd_stdin = fileno(stdin);
#endif
	YList *l=connections;

#ifdef _WIN32
	WSADATA wsa;	
	if (WSAStartup(MAKEWORD(2,2), &wsa))
		return -1;
#endif

	ylad = y_new0(yahoo_local_account, 1);

	local_host = strdup(get_local_addresses());

	printf("Yahoo Id: ");
	scanf("%s", ylad->yahoo_id);
	printf("Password: ");
#ifdef _WIN32
	scanf("%s", ylad->password);
#else
	{
		tcflag_t oflags;
		struct termios term;
		tcgetattr(fd_stdin, &term);
		oflags = term.c_lflag;
		term.c_lflag = oflags & ~(ECHO | ECHOK | ICANON);
		term.c_cc[VTIME] = 1;
		tcsetattr(fd_stdin, TCSANOW, &term);
		
		scanf("%s", ylad->password);

		term.c_lflag = oflags;
		term.c_cc[VTIME] = 0;
		tcsetattr(fd_stdin, TCSANOW, &term);
	}
	printf("\n");
#endif

	printf("Initial Status: ");
	scanf("%d", &status);

	printf("Log Level: ");
	scanf("%d", &log_level);
	do_yahoo_debug=log_level;

	register_callbacks();
	yahoo_set_log_level(log_level);

	ext_yahoo_login(ylad, status);

	while(poll_loop) {
		FD_ZERO(&inp);
		FD_ZERO(&outp);
#ifndef _WIN32
		FD_SET(fd_stdin, &inp);
		tv.tv_sec=1;
		tv.tv_usec=0;
#else
		tv.tv_sec=0;
		tv.tv_usec=1E4;
#endif
		lfd=0;

		for(l=connections; l; ) {
			struct conn_handler *c = l->data;
			if(c->remove) {
				YList *n = y_list_next(l);
				LOG(("Removing id:%d fd:%d", c->id, c->con->fd));
				connections = y_list_remove_link(connections, l);
				y_list_free_1(l);
				FREE(c);
				l=n;
			} else {
				if(c->cond & YAHOO_INPUT_READ)
					FD_SET(c->con->fd, &inp);
				if(c->cond & YAHOO_INPUT_WRITE)
					FD_SET(c->con->fd, &outp);
				if(lfd < c->con->fd)
					lfd = c->con->fd;
				l = y_list_next(l);
			}
		}

		select(lfd + 1, &inp, &outp, NULL, &tv);
		time(&curTime);

#ifndef _WIN32
		if(FD_ISSET(fd_stdin, &inp))	local_input_callback(0);
#else
		if (_kbhit()) local_input_callback(_getch());
#endif

		for(l = connections; l; l = y_list_next(l)) {
			struct conn_handler *c = l->data;
			if(c->con->remove) {
				FREE(c->con);
				c->con = NULL;
				continue;
			}
			if(c->remove)
				continue;
			if(FD_ISSET(c->con->fd, &inp))
				yahoo_callback(c, YAHOO_INPUT_READ);
			if(FD_ISSET(c->con->fd, &outp))
				yahoo_callback(c, YAHOO_INPUT_WRITE);
		}

		if(expired(pingTimer))		yahoo_ping_timeout_callback();
		if(expired(webcamTimer))	yahoo_webcam_timeout_callback(webcam_id);
	}
	LOG(("Exited loop"));

	while(connections) {
		YList *tmp = connections;
		struct conn_handler *c = connections->data;
		FREE(c);
		connections = y_list_remove_link(connections, connections);
		y_list_free_1(tmp);
	}

	yahoo_logout();

	FREE(ylad);

#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}

static void ext_yahoo_got_file(int id, const char *me, const char *who, const char *msg, const char *fname, 
	unsigned long fesize, char *trid)
{
	LOG(("Got a File transfer request (%s, %ld bytes) from %s", fname, fesize, who));
}

static void ext_yahoo_file_transfer_done(int id, int response, void *data)
{
}

static char *ext_yahoo_get_ip_addr(const char *domain)
{
	return NULL;
}

static void ext_yahoo_got_ft_data(int id, const unsigned char *in, int count, void *data)
{
}

static void ext_yahoo_got_identities(int id, YList * ids)
{
}

static void ext_yahoo_chat_yahoologout(int id, const char *me)
{ 
 	LOG(("got chat logout for %s", me));
}

static void ext_yahoo_chat_yahooerror(int id, const char *me)
{ 
 	LOG(("got chat error for %s", me));
}

static void ext_yahoo_got_ping(int id, const char *errormsg)
{ 
 	LOG(("got ping errormsg %s", errormsg));
}

static void ext_yahoo_got_search_result(int id, int found, int start, int total, YList *contacts)
{
	LOG(("got search result"));
}

static void ext_yahoo_got_buddyicon_checksum(int id, const char *a, const char *b, int checksum)
{
	LOG(("got buddy icon checksum"));
}

static void ext_yahoo_got_buddy_change_group(int id, const char *me, const char *who, 
	const char *old_group, const char *new_group)
{
}

static void ext_yahoo_got_buddyicon(int id, const char *a, const char *b, const char *c, int checksum)
{
	LOG(("got buddy icon"));
}

static void ext_yahoo_buddyicon_uploaded(int id, const char *url)
{
	LOG(("buddy icon uploaded"));
}

static void ext_yahoo_got_buddyicon_request(int id, const char *me, const char *who)
{
	LOG(("got buddy icon request from %s",who));
}

static void register_callbacks()
{
	static struct yahoo_callbacks yc;

	yc.ext_yahoo_login_response = ext_yahoo_login_response;
	yc.ext_yahoo_got_buddies = ext_yahoo_got_buddies;
	yc.ext_yahoo_got_ignore = ext_yahoo_got_ignore;
	yc.ext_yahoo_got_identities = ext_yahoo_got_identities;
	yc.ext_yahoo_got_cookies = ext_yahoo_got_cookies;
	yc.ext_yahoo_status_changed = ext_yahoo_status_changed;
	yc.ext_yahoo_got_im = ext_yahoo_got_im;
	yc.ext_yahoo_got_buzz = ext_yahoo_got_buzz;
	yc.ext_yahoo_got_conf_invite = ext_yahoo_got_conf_invite;
	yc.ext_yahoo_conf_userdecline = ext_yahoo_conf_userdecline;
	yc.ext_yahoo_conf_userjoin = ext_yahoo_conf_userjoin;
	yc.ext_yahoo_conf_userleave = ext_yahoo_conf_userleave;
	yc.ext_yahoo_conf_message = ext_yahoo_conf_message;
	yc.ext_yahoo_chat_cat_xml = ext_yahoo_chat_cat_xml;
	yc.ext_yahoo_chat_join = ext_yahoo_chat_join;
	yc.ext_yahoo_chat_userjoin = ext_yahoo_chat_userjoin;
	yc.ext_yahoo_chat_userleave = ext_yahoo_chat_userleave;
	yc.ext_yahoo_chat_message = ext_yahoo_chat_message;
	yc.ext_yahoo_chat_yahoologout = ext_yahoo_chat_yahoologout;
	yc.ext_yahoo_chat_yahooerror = ext_yahoo_chat_yahooerror;
	yc.ext_yahoo_got_webcam_image = ext_yahoo_got_webcam_image;
	yc.ext_yahoo_webcam_invite = ext_yahoo_webcam_invite;
	yc.ext_yahoo_webcam_invite_reply = ext_yahoo_webcam_invite_reply;
	yc.ext_yahoo_webcam_closed = ext_yahoo_webcam_closed;
	yc.ext_yahoo_webcam_viewer = ext_yahoo_webcam_viewer;
	yc.ext_yahoo_webcam_data_request = ext_yahoo_webcam_data_request;
	yc.ext_yahoo_got_file = ext_yahoo_got_file;
	yc.ext_yahoo_got_ft_data = ext_yahoo_got_ft_data;
	yc.ext_yahoo_get_ip_addr = ext_yahoo_get_ip_addr;
	yc.ext_yahoo_file_transfer_done = ext_yahoo_file_transfer_done;
	yc.ext_yahoo_contact_added = ext_yahoo_contact_added;
	yc.ext_yahoo_rejected = ext_yahoo_rejected;
	yc.ext_yahoo_typing_notify = ext_yahoo_typing_notify;
	yc.ext_yahoo_game_notify = ext_yahoo_game_notify;
	yc.ext_yahoo_mail_notify = ext_yahoo_mail_notify;
	yc.ext_yahoo_got_search_result = ext_yahoo_got_search_result;
	yc.ext_yahoo_system_message = ext_yahoo_system_message;
	yc.ext_yahoo_error = ext_yahoo_error;
	yc.ext_yahoo_log = ext_yahoo_log;
	yc.ext_yahoo_add_handler = ext_yahoo_add_handler;
	yc.ext_yahoo_remove_handler = ext_yahoo_remove_handler;
	yc.ext_yahoo_connect = ext_yahoo_connect;
	yc.ext_yahoo_connect_async = ext_yahoo_connect_async;
	yc.ext_yahoo_read = ext_yahoo_read;
	yc.ext_yahoo_write = ext_yahoo_write;
	yc.ext_yahoo_close = ext_yahoo_close;
	yc.ext_yahoo_got_buddyicon = ext_yahoo_got_buddyicon;
	yc.ext_yahoo_got_buddyicon_checksum = ext_yahoo_got_buddyicon_checksum;
	yc.ext_yahoo_buddyicon_uploaded = ext_yahoo_buddyicon_uploaded;
	yc.ext_yahoo_got_buddyicon_request = ext_yahoo_got_buddyicon_request;
	yc.ext_yahoo_got_ping = ext_yahoo_got_ping;
	yc.ext_yahoo_got_buddy_change_group = ext_yahoo_got_buddy_change_group;

	yahoo_register_callbacks(&yc);
}


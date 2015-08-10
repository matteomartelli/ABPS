//
//  testlib.c
//  
//
//  Created by Gabriele Di Bernardo on 13/05/15.
//
//

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <linux/errqueue.h>
#include <json-c/json.h>
#include <string.h>

#include "libsend.h"
#include "utils.h"
#include "structs.h"


/* Get TED information of an error message */
void __get_ted_info(ErrMsg *emsg, struct ted_info_s *ted_info) 
{

	ted_info->ip_vers = emsg->c->cmsg_level;
	ted_info->msg_id = ted_msg_id(emsg->ee);
	ted_info->retry_count = ted_retry_count(emsg->ee);
	ted_info->status  = ted_status(emsg->ee);
	//uint8_t more_frag = emsg->ee->ee_code;
	//uint16_t frag_len = (emsg->ee->ee_data >> 16);
	//uint16_t offset = ((emsg->ee->ee_data << 16) >> 16);
	//
	/* TODO: check fragment_offset and more_fragment */
}

int tederror_recv_wait(ErrMsg *em)
{
	struct iovec iov[1];
	int return_value, ip_vers, shared_descriptor;

	struct sockaddr *addr;

	if ((ip_vers = net_check_ip_instance_and_version()) == -1)
		return -1;

	if (ip_vers == IPPROTO_IPV6) {

		memset(&(em->name.ipv6_name), 0, sizeof(em->name.ipv6_name));
		em->name.ipv6_name.sin6_family = AF_INET6;
		em->namelen = sizeof(em->name.ipv6_name);
		em->is_ipv6 = 1;
		
		addr = (struct sockaddr *)&(em->name.ipv6_name);
		
	} else if (ip_vers == IPPROTO_IP) {

		memset(&(em->name.ipv4_name),0,sizeof(em->name.ipv4_name));
		em->name.ipv4_name.sin_family = AF_INET;
		em->namelen = sizeof(em->name.ipv4_name);
		em->is_ipv6 = 0;

		addr = (struct sockaddr *)&(em->name.ipv4_name);

	} else {
		utils_print_error("%s: wrong ip_vers.\n", __func__);	
	}
	
	shared_descriptor = net_get_shared_descriptor();

	return_value = getsockname(shared_descriptor, addr, &(em->namelen));
	
	if(return_value != 0) {
		utils_print_error("%s: getsockname failed (%s).\n",
		                  __func__, strerror(errno));
		return 0;
	}

	do
	{
		memset(&(em->msg),0,sizeof(em->msg));
		em->msg->msg_name = &(em->name);
		em->msg->msg_namelen = sizeof(em->name);
		em->msg->msg_iov = iov;
		em->msg->msg_iovlen = 1;
		iov->iov_base = em->errmsg;
		iov->iov_len = sizeof(em->errmsg);
		em->msg->msg_control = em->control;
		em->msg->msg_controllen = sizeof(em->control);
		return_value = recvmsg(shared_descriptor, em->msg, MSG_ERRQUEUE); 
		em->myerrno=errno;
	} while ((return_value < 0) && ((em->myerrno == EINTR) || (em->myerrno == EAGAIN)));

	if(return_value < 0) {
		if (em->myerrno == EAGAIN) {
			/* No message available on error queue. */
			printf("recvmsg: EAGAIN\n");
			return 0;
		} else {
			errno = em->myerrno;
			utils_print_error("%s: recvmsg failed with error (%s).\n",
			                  __func__, strerror(errno));
			return -1;
		}
	} else {
		if ((em->msg->msg_flags & MSG_ERRQUEUE) != MSG_ERRQUEUE) {

			printf("recvmsg: no errqueue\n");
			return 0;

		} else if (em->msg->msg_flags & MSG_CTRUNC) {

			printf("recvmsg: extended error was truncated\n");
			return 0;

		} else {
			return 1; // read, ok
		}
	}
}

int tederror_check_ted_info(ErrMsg *emsg, struct ted_info_s *ted_info)
{
	/* For each error header of the previuosly sent message, 
	 * look for TED notifications. */
	for (emsg->c = CMSG_FIRSTHDR(emsg->msg); 
	     emsg->c; 
	     emsg->c = CMSG_NXTHDR(emsg->msg, emsg->c)) {
			
		if ((emsg->c->cmsg_level == IPPROTO_IPV6) && 
		    (emsg->c->cmsg_type == IPV6_RECVERR)) {
			
			/*struct sockaddrin_6 *from;
			from = (struct sockaddrin_6 *)SO_EE_OFFENDER(emsg->ee);*/
			emsg->ee = (struct sock_extended_err *)CMSG_DATA(emsg->c);

		} else if((emsg->c->cmsg_level == IPPROTO_IP) && 
		          (emsg->c->cmsg_type == IP_RECVERR)) {

			/*struct sockaddrin *from;
			from = (struct sockaddrin *) SO_EE_OFFENDER(emsg->ee);*/
			emsg->ee = (struct sock_extended_err *) CMSG_DATA(emsg->c);
		} else {
			/* Skip non interesting error messages */
			continue;
		}

		switch (emsg->ee->ee_origin) {
			
			/* TED notification */
			case SO_EE_ORIGIN_LOCAL_NOTIFY: 
				if(emsg->ee->ee_errno == 0) {
					__get_ted_info(emsg, ted_info);
				} else {
					//TODO: handle errno for local notify
				}
				break;
			default:
				if(emsg->ee->ee_errno != 0)
					printf(strerror(emsg->ee->ee_errno));

				break;
		}
	}

	return 0;
}



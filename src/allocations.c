/*
 * allocations.c
 *
 *  Created on: Jan 8, 2014
 *      Author: Benjamin Hesmans
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <string.h>
#include "mptcptrace.h"
#include "TCPOptions.h"
#include "allocations.h"
#include "MPTCPList.h"

mptcp_map* new_mpm(){
	mptcp_map *mpm = (mptcp_map*) malloc(sizeof(mptcp_map));
	mpm->ref_count=0;
	return mpm;
}

mptcp_ack* new_mpa(){
	mptcp_ack *mpa = (mptcp_ack*) malloc(sizeof(mptcp_ack));
	mpa->ref_count=0;
	return mpa;
}

void freemsf(void *element){
	mptcp_sf *msf= (mptcp_sf*) element;
	destroyList(msf->mseqs[S2C]);
	destroyList(msf->mseqs[C2S]);
	destroyList(msf->macks[S2C]);
	destroyList(msf->macks[C2S]);
	destroyList(msf->tcpUnacked[S2C]->l);
	destroyList(msf->tcpUnacked[C2S]->l);
	BOTH(free LP msf->tcpLastAck,RP)
	free(msf->tcpUnacked[S2C]);
	free(msf->tcpUnacked[C2S]);
	free(element);
}
void freecon(void *element){
	mptcp_conn *con = (mptcp_conn*) element;
	destroyList(con->mptcp_sfs);
	destroyList(con->mci->unacked[C2S]->l);
	destroyList(con->mci->unacked[S2C]->l);
	BOTH( free LP con->mci->lastack, RP)
	if(add_addr) fclose(con->addAddr);
	if(rm_addr) fclose(con->rmAddr);
	free(con->mci->unacked[C2S]);
	free(con->mci->unacked[S2C]);
	free(con->mci->firstSeq[C2S]);
	free(con->mci->firstSeq[S2C]);
	free(con->mci);
	free(element);

}
void build_msf(struct sniff_ip *ip, struct sniff_tcp *tcp, mptcp_sf *msf, int revert, int initList){
	if(revert){
		if(IS_IPV4(ip)){
			msf->family=AF_INET;
			memcpy(&msf->ip_dst.in,&ip->ip_src,sizeof(struct in_addr));
			memcpy(&msf->ip_src.in,&ip->ip_dst,sizeof(struct in_addr));
			memcpy(&msf->th_dport,&tcp->th_sport,sizeof(u_short));
			memcpy(&msf->th_sport,&tcp->th_dport,sizeof(u_short));
		}
		else{
			//copy for ipv6 kind of hacky look, to rewrite.
			msf->family=AF_INET6;
			memcpy(&msf->ip_dst.in6,((u_char*)ip)+8,sizeof(struct in6_addr));
			memcpy(&msf->ip_src.in6,((u_char*)ip)+24,sizeof(struct in6_addr));
			memcpy(&msf->th_dport,((u_char*)ip)+40,sizeof(u_short));
			memcpy(&msf->th_sport,((u_char*)ip)+42,sizeof(u_short));
		}
	}
	else{
		if(IS_IPV4(ip)){
			msf->family=AF_INET;
			memcpy(&msf->ip_dst.in,&ip->ip_dst,sizeof(struct in_addr));
			memcpy(&msf->ip_src.in,&ip->ip_src,sizeof(struct in_addr));
			memcpy(&msf->th_dport,&tcp->th_dport,sizeof(u_short));
			memcpy(&msf->th_sport,&tcp->th_sport,sizeof(u_short));
		}
		else{
			msf->family=AF_INET6;
			memcpy(&msf->ip_src.in6,((u_char*)ip)+8,sizeof(struct in6_addr));
			memcpy(&msf->ip_dst.in6,((u_char*)ip)+24,sizeof(struct in6_addr));
			memcpy(&msf->th_sport,((u_char*)ip)+40,sizeof(u_short));
			memcpy(&msf->th_dport,((u_char*)ip)+42,sizeof(u_short));
		}
	}
	if(initList){
		//TODO define the free fun
		msf->mseqs[S2C] = newList(NULL);
		msf->mseqs[C2S] = newList(NULL);
		msf->macks[S2C] = newList(NULL);
		msf->macks[C2S] = newList(NULL);
		msf->tcpUnacked[C2S] = newOrderedList(free,compareTcpMap);
		msf->tcpUnacked[S2C] = newOrderedList(free,compareTcpMap);
		msf->tcpLastAck[C2S] = NULL;
		msf->tcpLastAck[S2C] = NULL;
	}
	BOTH(msf->info , .tput =0)
	msf->mc_parent = NULL;
}

mptcp_sf* new_msf(struct sniff_ip *ip, struct sniff_tcp *tcp){
	mptcp_sf *msf = (mptcp_sf*) exitMalloc(sizeof(mptcp_sf));
	//TODO fill with 0
	build_msf(ip,tcp,msf,DONOTREVERT,1);
	return msf;
}
mptcp_sf* new_msf_revert(struct sniff_ip *ip, struct sniff_tcp *tcp){
	mptcp_sf *msf = (mptcp_sf*) exitMalloc(sizeof(mptcp_sf));
	//TODO fill with 0
	build_msf(ip,tcp,msf,REVERT,1);
	return msf;
}


#include <stdio.h>
#include <stdlib.h>

#include "socket/sockets-util.h"
#include "log/lmlog.h"
#include "client/client_function.h"
#include "transport/transport_function.h"
#include "defs.h"
#include "audio/aud.h"


#define PUT_CMD  1
#define DAIL_CMD 2

typedef struct IPinterCom_control_hdr{
    uint8_t src;
    uint8_t dest;
    uint8_t cmd;
    uint8_t Reserve;
    int  filesize;
    char filename[16];
}IPinterCom_control_hdr;

int check_file_exist(char *name)
{
    return GOOD;
}

int process_put_cmd(int sock, uint8_t *packet, ip_addr_t dest_ip, uint16_t remote_port)
{
    char *name;
    char decode_file_name[32];
    FILE *aud_encode_fp, *aud_raw_fp;
    int status = 0; 
    int ret = 0;

    name = (char *)(((IPinterCom_control_hdr *)packet)->filename);

    snd_ftp_get(name);
    ret = check_file_exist(name);
    if(ret!=GOOD){
        LMLOG(LERR, "%s:The %s file not exist!", __FUNCTION__, name);

        return ret;
    }

    sprintf(decode_file_name, "raw_%s", name);

    aud_encode_fp = fopen((const char *)name, 'r');
    if(aud_encode_fp == NULL){
        LMLOG(LERR, "%s:The %s file not exist!", __FUNCTION__, name);

        return BAD;
    }

    aud_raw_fp = fopen(decode_file_name, 'w+');
    if(aud_raw_fp == NULL){
        LMLOG(LERR, "%s:The %s file open faied!", __FUNCTION__, name);

        return BAD;
    }

    snd_decode_start(aud_encode_fp, aud_raw_fp);

    fclose(aud_encode_fp);

    //judgement the linphone calls is running or not
    status = calls_status();
    if(status == RUNNING){
        fclose(aud_raw_fp);
        return BAD;
    }

    snd_start_play(aud_raw_fp);

    return GOOD;
}

/*
 *  Process a IPinterCom protocol message sitting on
 *  socket s with address family afi
 */
int process_ctl_msg(int sock, int afi, struct in_addr client_ip)
{
    uint8_t     packet[MAX_IP_PACKET];
    uint16_t    remote_port;
    ip_addr_t   dest_ip;

    if ( get_packet_and_socket_inf (sock, afi, packet,&dest_ip,&remote_port) != GOOD ){
        return BAD;
    }

    dest_ip.addr.v4.s_addr = client_ip.s_addr;
    LMLOG(LINF, "Received a IPinterCom control message,the dest_ip:0x%x,remote_port:%d",dest_ip.addr.v4.s_addr,remote_port);

    switch (((IPinterCom_control_hdr *) packet)->cmd) {
        case PUT_CMD:    //Got Cmd Setting Reply
            LMLOG(LINF, "Received a IPinterCom Cmd message");
            if (process_put_cmd(sock,packet,dest_ip,remote_port) != GOOD){
                return (BAD);
            }
            break;
        case DAIL_CMD:
            break;
        default:
            LMLOG(LINF, "Unidentified type control message received");
            return (ERROR);
    }

    LMLOG(LINF, "Processing Completed to IPinterCom protocol control message");

    return(GOOD);
}

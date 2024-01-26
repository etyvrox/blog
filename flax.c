#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <unistd.h> 
#include <stdint.h> 
#include <linux/netfilter.h> 
#include <libnetfilter_queue/pktbuff.h> 
#include <libnetfilter_queue/libnetfilter_queue.h> 
#include <sys/socket.h> 
 
struct Helpers{ 
  int pkt_num; 
  int pkt_rate; 
}; 
 
int process_batch(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) { 
   
    struct Helpers *helpers = (struct Helpers *)data; 
    int pkt_num = helpers->pkt_num; 
    int pkt_rate = helpers->pkt_rate; 
     
    int delay = (1000000 * pkt_rate) / pkt_num; 
 
    printf("Packet ID inside process_batch: %u\n", ntohl(nfq_get_msg_packet_hdr(nfa)->packet_id)); 
     
    return nfq_set_verdict(qh, ntohl(nfq_get_msg_packet_hdr(nfa)->packet_id), NF_ACCEPT, 0, NULL); 
} 
 
int main(int argc, char *argv[]) { 
    struct nfq_handle *h; 
    struct nfq_q_handle *qh; 
 
    int seconds = strtol(argv[1], NULL, 10); 
    int num_packets = strtol(argv[2], NULL, 10); 
     
    printf("num_packets is %d\n", num_packets); 
     
    int delay = (seconds * 1000000) / num_packets; 
     
    struct Helpers helpers_instance; 
    helpers_instance.pkt_rate = seconds; 
    helpers_instance.pkt_num = num_packets; 
     
    int fd; 
    char buf[4096]; 
 
    printf("[*] Started with delay of %d\n", delay); 
 
    h = nfq_open(); 
    if (!h) { 
        fprintf(stderr, "Error in nfq_open()\n"); 
        exit(1); 
    } 
 
    if (nfq_unbind_pf(h, AF_INET) < 0) { 
        fprintf(stderr, "Error in nfq_unbind_pf()\n"); 
        exit(1); 
    } 
 
    if (nfq_bind_pf(h, AF_INET) < 0) { 
        fprintf(stderr, "Error in nfq_bind_pf()\n"); 
        exit(1); 
    } 
 
    qh = nfq_create_queue(h, 0, &process_batch, &helpers_instance); 
    printf("[*] Queue successfully created!\n"); 
    if (!qh) { 
        fprintf(stderr, "Error in nfq_create_queue()\n"); 
        exit(1); 
    } 
 
    // Set the maximum queue length 
    if (nfq_set_queue_maxlen(qh, 1) < 0) { 
        fprintf(stderr, "Error setting queue max length\n"); 
        exit(1); 
    } 
 
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) { 
        fprintf(stderr, "Can't set packet_copy mode\n"); 
        exit(1); 
    } 
 
    fd = nfq_fd(h); 
     
    printf("[*] Starting to recieve data\n"); 
 
    while (1) { 
    int len = recv(fd, buf, sizeof(buf), 0); 
    nfq_handle_packet(h, buf, len); 
    usleep(delay); 
    } 
    nfq_destroy_queue(qh); 
    nfq_close(h); 
 
    return 0; 
}
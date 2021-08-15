#include <stdio.h>
#include <pcap/pcap.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

//===================
// headers
//===================

struct ipheader {
    unsigned char       iph_ihl:4,       /* IP header length, use bit field - 4 bits are sufficient */
                        iph_ver:4;       /* IP version */
    unsigned char       iph_tos;         /* Type of service */
    unsigned short int  iph_len;         /* IP Packet length (data + header) */
    unsigned short int  iph_ident;       /* Identification */
    unsigned short int  iph_flag:3,      /* Fragmentation flags */
                        iph_offset:13;   /* Flags offset */
    unsigned char       iph_ttl;         /* Time to Live */
    unsigned char       iph_protocol;    /* Protocol type */
    unsigned short int  iph_chksum;      /* IP datagram checksum */
    struct in_addr      iph_src_ip;      /* Source IP address */
    struct in_addr      iph_dest_ip;     /* Destination IP address */
};

struct icmpheader {
    unsigned char icmp_type;
    unsigned char icmp_code;
    unsigned short int icmp_chksum;      /* icmp checksum */
    unsigned short int icmp_id;          /* icmp identifier */
    unsigned short int icmp_seq;         /* icmp sequence number */
};

struct ethernetheader  {
    u_char  ether_dhost[ETHER_ADDR_LEN];  /* destination host address */
    u_char  ether_shost[ETHER_ADDR_LEN];  /* source host address */
    u_short ether_type;                   /* protocol type (IP, ARP, RARP, etc) 
                                             tells the OS what kind of data the frame carries */
};


// a function that sends the given packet out 
// the callback to be run on every packet captured.
void packet_out(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    struct ethernetheader* eth = (struct ethernetheader*) packet;
    if(ntohs(eth->ether_type) == 0x0800){  // 0x0800 means that the frame has an IPv4 packet
        struct ipheader* ip = (struct ipheader*) (packet + sizeof(struct ethernetheader));
        if(ip->iph_protocol == IPPROTO_ICMP) {
            struct icmpheader* icmp = (struct icmpheader*) (packet + sizeof(struct ethernetheader) + sizeof(struct ipheader));
            printf("IP_SRC: %s\n", inet_ntoa(ip->iph_src_ip));
            printf("IP_DST: %s\n",inet_ntoa(ip->iph_dest_ip));
            printf("TYPE: %d\n", icmp->icmp_type);
            printf("CODE: %d\n\n", icmp->icmp_code);
        }
    }
}

int main() {

    pcap_t *handle;  // to read packets
    char error_buffer[PCAP_ERRBUF_SIZE];  // for putting the error messages into it 
    struct bpf_program fp;  // a pointer to a bpf_program struct

    char filter_exp[] = "icmp";  // the type of the filter -- ICMP packets only
    bpf_u_int32 net;

    // open a device for capturing  
    // obtain a packet capture handle to look at packets on the network
    handle = pcap_open_live("wlp2s0", BUFSIZ, 1, 1000, error_buffer);

    pcap_activate(handle);  // activate a capture handle to look at packets on the network
    // compile a filter expression
    if(pcap_compile(handle, &fp, filter_exp, 0, net)==-1) {  // an error was occurred
        pcap_perror(handle, "Compile");  // display the error text.
    }

     // to specify a filter program
    pcap_setfilter(handle, &fp);  // fp is the result of a call to pcap_compile()

    // process packets from a live capture
    pcap_loop(handle, -1, packet_out, NULL);
    pcap_close(handle);  // close the capture device 

    return 0;
}

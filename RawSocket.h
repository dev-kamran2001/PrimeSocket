#pragma once

/* RawSocket
* Create a raw socket providing more control over Internet Protocol or TCP/IP if specified
* 
* Currently are in developmenet phase (TODO) and has known issue's, avoid public or important usages
*/

// IPv4
typedef struct ip_hdr
{
	unsigned char ip_header_len : 4; // 4-bit header length (in 32-bit words) normally=5 (Means 20 Bytes may be 24 also)
	unsigned char ip_version : 4; // 4-bit IPv4 version
	unsigned char ip_tos; // IP type of service
	unsigned short ip_total_length; // Total length
	unsigned short ip_id; // Unique identifier

	unsigned char ip_frag_offset : 5; // Fragment offset field

	unsigned char ip_more_fragment : 1;
	unsigned char ip_dont_fragment : 1;
	unsigned char ip_reserved_zero : 1;

	unsigned char ip_frag_offset1; //fragment offset

	unsigned char ip_ttl; // Time to live
	unsigned char ip_protocol; // Protocol(TCP,UDP etc)
	unsigned short ip_checksum; // IP checksum
	unsigned int ip_srcaddr; // Source address
	unsigned int ip_destaddr; // Source address
} IPV4_HDR, * PIPV4_HDR, FAR* LPIPV4_HDR;

// TCP header
typedef struct tcp_header
{
	unsigned short source_port; // source port
	unsigned short dest_port; // destination port
	unsigned int sequence; // sequence number - 32 bits
	unsigned int acknowledge; // acknowledgement number - 32 bits

	unsigned char ns : 1; //Nonce Sum Flag Added in RFC 3540.
	unsigned char reserved_part1 : 3; //according to rfc
	unsigned char data_offset : 4; /*The number of 32-bit words in the TCP header.
	This indicates where the data begins.
	The length of the TCP header is always a multiple
	of 32 bits.*/

	unsigned char fin : 1; //Finish Flag
	unsigned char syn : 1; //Synchronise Flag
	unsigned char rst : 1; //Reset Flag
	unsigned char psh : 1; //Push Flag
	unsigned char ack : 1; //Acknowledgement Flag
	unsigned char urg : 1; //Urgent Flag

	unsigned char ecn : 1; //ECN-Echo Flag
	unsigned char cwr : 1; //Congestion Window Reduced Flag

	////////////////////////////////

	unsigned short window; // window
	unsigned short checksum; // checksum
	unsigned short urgent_pointer; // urgent pointer
} TCP_HDR, * PTCP_HDR, FAR* LPTCP_HDR, TCPHeader, TCP_HEADER;

class RawSocket
{
public:
	PRIMESOCKET_API RawSocket();

	PRIMESOCKET_API char* GetActiveInterfaceAddress();

	/* Enable/Disable the auto IPv4 header generation (enabled by default)
	* If disabled, you have to also include the IP header in your raw packet data
	*/
	PRIMESOCKET_API void setAutoIpHeaderGenEnabled(bool enabled);
	/* Set the protocol number (default is 200)
	* The protocol number is critical for the receiver to understand what packet to look for
	* If there is a missmatch between sender and receiver protocol numbers, the receiver will never receive the expected packet!
	* Note: function useless if auto IPv4 header generation is disabled
	*/
	PRIMESOCKET_API void setProtocolNumber(int number); // values greater than 253 are not allowed
	/* Enable/Disable IP header checksum
	* Note: function useless if auto IPv4 header generation is disabled
	*/
	PRIMESOCKET_API void setAutoChecksumEnabled(bool enabled);
	/* Protocol Identification for the packet's to send (default is 0)
	* Protocol ID are not the same as Protocol Number, Proto number actually define's the protocol type that are being used on top of IP
	*/
	PRIMESOCKET_API void setPacketProtocolId(int id);

	PRIMESOCKET_API bool Write(char* data, size_t size, char* destAddress);

private:
	bool _autoIphGen;
	int _protoNum;
	bool _autoChecksum;
	int _protoId;

	SOCKET _sock;
};
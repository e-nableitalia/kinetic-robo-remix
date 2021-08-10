//
// RTPPackets: packets related classes
//
// Author: A.Navatta

#ifndef RTP_PACKETS_H

#define RTP_PACKETS_H

#include <stdint.h>

#define RTP_STD_HEADER_SZ	12		

#define RTP_MAX_PAYLOAD_SZ	1400
#define RTP_MAX_PKT_SIZE	(RTP_MAX_PAYLOAD_SZ + RTP_STD_HEADER_SZ)

class RTPPacket {
public:
	RTPPacket();
	~RTPPacket();

	void init(unsigned int ptype, unsigned int ssrc = 0);

	int  getSize();
	uint8_t *getData();
	uint8_t *getPayload();
	
	int	 setPayload(uint8_t* p, int psize);
	void setPayloadSize(int size);
	void setVersion(int i);
	void setCC(int i);
	void setM(bool marker);
	void setPT(unsigned int id);
	void setSN(unsigned short seq_num);
	void setTS(unsigned int ts);
	void setSSRC(unsigned int ssrc);

//	void setCSRC(int pos, int csrc);

private:
	void htons(int position, unsigned short v);
	void htonl(int position, unsigned int v);
	uint8_t data[RTP_MAX_PKT_SIZE];
	int  size;
	int  sn;
};

#endif


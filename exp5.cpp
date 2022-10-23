/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <map>
using namespace std;

// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address();

// implemented by students

map<unsigned, stud_route_msg*> routeTable;

void stud_Route_Init()
{
	printf("-----init a routeTable!\n");
	return;
}

void stud_route_add(stud_route_msg *proute)
{
	unsigned ipAddr = proute->dest;
	ipAddr = ntohl(ipAddr);
	stud_route_msg* route = (stud_route_msg*)malloc(sizeof(stud_route_msg));
	memcpy(route, proute, sizeof(stud_route_msg));
	routeTable[ipAddr] = route;
	printf("-----add a route of ip %x,nextIp is %x\n", ipAddr, proute->nexthop);
	return;
}
unsigned getCheckSum(unsigned short* pBuffer, unsigned headLen) {
	unsigned sum = 0;
	int i = 0;
	headLen = headLen / 2;
	while (headLen > i) {
		sum += (unsigned)(pBuffer[i]);
		i++;
	}
	if (sum & 0xffff != sum) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return sum;
}
int stud_fwd_deal(char *pBuffer, int length)
{
	unsigned localAddr = getIpv4Address();
	unsigned recvAddr = ((unsigned*)pBuffer)[4];
	recvAddr = ntohl(recvAddr);
	printf("-----the local addr is %x ,the recv addr is %x\n", localAddr, recvAddr);

	/*本地接受*/
	if (localAddr == recvAddr) {
		fwd_LocalRcv(pBuffer, length);
		return 0;
	}

	/*查找路由表*/
	stud_route_msg* route = routeTable[recvAddr];
	/*错误分组*/
	if (route == NULL) {
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
		printf("-----wrong ip!\n");
		return 1;
	}

	//检查生存时间
	unsigned ttl = (unsigned)(pBuffer[8]);
	printf("-----the ttl is %d\n", ttl);
	if (ttl == 0) {
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
		return 1;
	}

	/*发送至下一个路由*/
	unsigned int nextIp = route->nexthop;
	printf("-----next ip is %x\n", nextIp);
	//init header checksum
	((unsigned short*)pBuffer)[5] = 0;
	//更改ttl
	pBuffer[8] -= 1;
	//计算校验和
	unsigned headLen = (unsigned)(pBuffer[0]) & 0xf;
	headLen *= 4;
	unsigned short checkSum = ~getCheckSum((unsigned short*)pBuffer, headLen);
	((unsigned short*)pBuffer)[5] = checkSum;
	printf("-----checksum is %x\n", checkSum);
	fwd_SendtoLower(pBuffer, length, nextIp);
	return 0;
}



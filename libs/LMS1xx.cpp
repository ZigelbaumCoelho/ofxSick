/*
 * LMS1xx.cpp
 *
 *  Created on: 09-08-2010
 *  Author: Konrad Banachowicz
 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "ofMain.h"

#include "LMS1xx.h"

LMS1xx::LMS1xx() :
connected(false) {
	debug = false;
}

LMS1xx::~LMS1xx() {
	
}

void LMS1xx::connect(std::string host, int port) {
	if (!connected) {
		sockDesc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sockDesc) {
			struct sockaddr_in stSockAddr;
			int Res;
			stSockAddr.sin_family = PF_INET;
			stSockAddr.sin_port = htons(port);
			Res = inet_pton(AF_INET, host.c_str(), &stSockAddr.sin_addr);
			
			int ret = ::connect(sockDesc, (struct sockaddr *) &stSockAddr,
													sizeof stSockAddr);
			if (ret == 0) {
				connected = true;
			}
		}
	}
}

void LMS1xx::disconnect() {
	if (connected) {
		close(sockDesc);
		connected = false;
	}
}

bool LMS1xx::isConnected() {
	return connected;
}

void LMS1xx::startMeas() {
	char buf[100];
	sprintf(buf, "%c%s%c", 0x02, "sMN LMCstartmeas", 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sMN LMCstartmeas return: " << buf << std::endl;
	}
}

void LMS1xx::stopMeas() {
	char buf[100];
	sprintf(buf, "%c%s%c", 0x02, "sMN LMCstopmeas", 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sMN LMCstopmeas return: " << buf << std::endl;
	}
}

status_t LMS1xx::queryStatus() {
	char buf[100];
	sprintf(buf, "%c%s%c", 0x02, "sRN STlms", 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sRN STlms return: " << buf << std::endl;
	}
	int ret;
	sscanf((buf + 10), "%d", &ret);
	
	return (status_t) ret;
}

void LMS1xx::login() {
	char buf[100];
	sprintf(buf, "%c%s%c", 0x02, "sMN SetAccessMode 03 F4724744", 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sMN SetAccessMode return: " << buf << std::endl;
	}
}

scanCfg LMS1xx::getScanCfg() const {
	scanCfg cfg;
	char buf[100];
	sprintf(buf, "%c%s%c", 0x02, "sRN LMPscancfg", 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sRN LMPscancfg return: " << buf << std::endl;
	}
	
	sscanf(buf + 1, "%*s %*s %X %*d %X %X %X", &cfg.scaningFrequency,
				 &cfg.angleResolution, &cfg.startAngle, &cfg.stopAngle);
	return cfg;
}

void LMS1xx::setScanCfg(const scanCfg &cfg) {
	char buf[100];
	//Set start and stop angle
	sprintf(buf, "%c%s +1 %X %X %X%c", 0x02, "sWN LMPoutputRange",
					cfg.angleResolution, cfg.startAngle, cfg.stopAngle, 0x03);
	write(sockDesc, buf, strlen(buf));
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sWN LMPoutputRange return: " << buf << std::endl;
	}
	
	// set scanning frequency and angle resolution
	//char buf[100];
	sprintf(buf, "%c%s %X 1 %X %X %X%c", 0x02, "sMN mLMPsetscancfg",
					cfg.scaningFrequency, cfg.angleResolution, cfg.startAngle,
					cfg.stopAngle, 0x03);
	write(sockDesc, buf, strlen(buf));
	//int
	len = read(sockDesc, buf, 100);
	buf[len - 1] = 0;
	
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sMN mLMPsetscancfg return: " << buf << std::endl;
	}
}

void LMS1xx::setScanDataCfg(const scanDataCfg &cfg) {
	char buf[100];
	sprintf(buf, "%c%s %02X 00 %d %d 0 %02X 00 %d %d 0 %d +%d%c", 0x02,
					"sWN LMDscandatacfg", cfg.outputChannel, cfg.remission ? 1 : 0,
					cfg.resolution, cfg.encoder, cfg.position ? 1 : 0,
					cfg.deviceName ? 1 : 0, cfg.timestamp ? 1 : 0, cfg.outputInterval, 0x03);
	if(debug)
		printf("%s\n", buf);
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	buf[len - 1] = 0;
	
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sWN LMDscandatacfg return: " << buf << std::endl;
	}
}

void LMS1xx::scanContinous(int start) {
	char buf[100];
	sprintf(buf, "%c%s %d%c", 0x02, "sEN LMDscandata", start, 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	
	if (buf[0] != 0x02)
		printf("invalid packet recieved\n");
	
	if (debug) {
		buf[len] = 0;
		printf("%s\n", buf);
	}
	
	if (start = 0) {
		for (int i = 0; i < 10; i++)
			read(sockDesc, buf, 100);
	}
}

string repeat(string str, int n) {
	string out = "";
	for(int i = 0; i < n; i++) {
		out += str;
	}
	return out;
}

// originally was 20000
#define DATA_BUF_LEN 40000
void LMS1xx::getData(scanData& data) {
	char raw[DATA_BUF_LEN];
	
	// step 1: read everything available from the network
	// step 2: read local buffer from STX 0x02 to ETX 0x03
	// step 3: parse most oldest data in fixed size queue
	
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(sockDesc, &rfds);
	
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	
	int socketsAvailable = select(sockDesc + 1, &rfds, NULL, NULL, &tv);
	if(socketsAvailable > 0) { // at least one socket available
		while(true) {
			int curLen = read(sockDesc, raw, DATA_BUF_LEN); // read available data
			if(curLen > 0) { // some data was read
				copy(raw, raw + curLen, back_inserter(parser.data));
				if(parser.parse()) {
					data.dist_len1 = parser.chan_data.size();
					copy(parser.chan_data.begin(), parser.chan_data.end(), data.dist1);
					break;
				}
			} else { // no data remains
				ofLogError() << "no more data available";
			}
		}
	} else { // sockets unavailable
	}
}

void LMS1xx::saveConfig() {
	char buf[100];
	sprintf(buf, "%c%s%c", 0x02, "sMN mEEwriteall", 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sMN mEEwriteall return: " << buf << std::endl;
	}
}

void LMS1xx::startDevice() {
	char buf[100];
	sprintf(buf, "%c%s%c", 0x02, "sMN Run", 0x03);
	
	write(sockDesc, buf, strlen(buf));
	
	int len = read(sockDesc, buf, 100);
	if (buf[0] != 0x02)
		std::cout << "invalid packet recieved" << std::endl;
	if (debug) {
		buf[len] = 0;
		std::cout << "sMN Run return: " << buf << std::endl;
	}
}

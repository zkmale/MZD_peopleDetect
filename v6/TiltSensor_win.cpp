#include "TiltSensor.h"

#ifdef WIN32


// 线程回调
void TiltSensor::onThread() {

}

// 回收资源
void TiltSensor::close_socket(SOCKET sock_fd) {

}

#if 0

#include <stdio.h>
#include <math.h>
#include <thread>
#include <chrono>
#include <bits/socket.h>
#include <netinet/in.h>
#include "logger/logger.hpp"
#include "libhv/hsocket.h"

#ifdef _WIN32

#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

// windows 网络环境初始化
class WindowsNetEnvTool {
public:
	WindowsNetEnvTool() {
		// 初始化WSA
		WORD sockVersion = MAKEWORD(2, 2);
		WSADATA wsaData;
		if (WSAStartup(sockVersion, &wsaData) != 0) {
			log_critical("Windows Net Init Failed.");
			exit(0);
		}
	}

	~WindowsNetEnvTool() {
		WSACleanup();
	}
};

#endif

#define DEFAULT_SVR_IP "127.0.0.1"
//#define DEFAULT_SVR_IP "192.168.1.99"
#define DEFAULT_SVR_PORT 9100
#define read_senser_interval 500  // 读取传感器周期 ms


void print_recv_data(const char *recv_data, int size) {
	for (int i = 0; i < size; ++i) {
		printf("0x%02X ", recv_data[i]);
	}
	printf("\n");
	fflush(stdout);
	// 0x01 0x03 0x08 0xBF 0x38 0xA3 0xE0 0x3E 0x87 0x6E 0x48 0x6F 0xF2

	// 寄存器数据长度
	int len = recv_data[2];
	if (size < 13 || len < 8) {
		log_error("recv data length < 13, len = {}, size = {}", len, size);
		return;
	}

	float x = 0.0f;
	float y = 0.0f;

	// 第一个传感器
	uint8_t x_data[4] = {0};
	x_data[0] = recv_data[6]; //
	x_data[1] = recv_data[5]; //
	x_data[2] = recv_data[4]; //
	x_data[3] = recv_data[3]; //
	memcpy(&x, x_data, sizeof(x_data));
	// 第一个传感器
	uint8_t y_data[4] = {0};
	y_data[0] = recv_data[10]; //
	y_data[1] = recv_data[9];  //
	y_data[2] = recv_data[8];  //
	y_data[3] = recv_data[7];  //
	memcpy(&y, y_data, sizeof(y_data));

	if (std::fabs(x) < 1e-6) {
//		printf("%f\n", std::fabs(x));
//		log_info("x = 0.0f");
		x = 0.0f;
	}

	if (std::fabs(y) < 1e-6) {
//		printf("%f\n", std::fabs(y));
//		log_info("y = 0.0f");
		y = 0.0f;
	}

	char print_data[512] = {0};
	sprintf(print_data, "x = %f, y = %f", x, y);
	log_info("{}", print_data);
//	log_info("x = {}, y = {}", x, y);

	//	方法2 报错
//	float x_ = 0.0f;
//	float y_ = 0.0f;
//	memcpy(&x_, reinterpret_cast<const void *>(recv_data[4]), 4);
//	memcpy(&y_, reinterpret_cast<const void *>(recv_data[8]), 4);
//	x_ = ntohl(x_);
//	y_ = ntohl(y_);
//	log_info("x = {}, y = {}, x_ = {}, y_ = {}", x, y, x_, y_);

}

#ifdef _WIN32

#else
typedef int SOCKET;
#define INVALID_SOCKET  -1
#define SOCKET_ERROR  -1
#endif


int main(int argc, char *argv[]) {
#ifdef _WIN32
	std::unique_ptr<WindowsNetEnvTool> netTool = std::make_unique<WindowsNetEnvTool>();
#endif

	// 创建套接字
	SOCKET listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_fd == INVALID_SOCKET) {
		log_info("socket error !");
		return 0;
	}

	// 绑定IP和端口
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DEFAULT_SVR_PORT);
#if _WIN32
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	sin.sin_addr.s_addr = INADDR_ANY;
#endif
	if (bind(listen_fd, (struct sockaddr *) & sin, sizeof(sin)) == SOCKET_ERROR) {
		log_info("bind error !");
	}

	// 开始监听
	if (listen(listen_fd, 5) == SOCKET_ERROR) {
		log_error("listen error !");
		return 0;
	}

	// 请求倾角传感器数据
	char send_buf[] = {  // 这里不需要转换字节序
			0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09
	};

	//循环接收数据
	SOCKET client_fd;
	sockaddr_in client_addr;
	int client_addrlen = sizeof(client_addr);
	char revData[255] = {0};
	while (true) {
		log_info("Wait Client Connect ...");
		client_fd = accept(listen_fd, (struct sockaddr * ) & client_addr,
						   reinterpret_cast<socklen_t *>(&client_addrlen));
		if (client_fd == INVALID_SOCKET) {
			log_error("accept error !");
			continue;
		}

		auto ip = inet_ntoa(client_addr.sin_addr);
		int port = ntohs(client_addr.sin_port);
		log_info("New Client Connect, client_fd ip = {}, client_fd port = {}", ip, port);

		for (int i = 0; i < 50; ++i) {
			// 发送数据
			const char *sendData = send_buf;
			send(client_fd, sendData, 8, 0);

			// 接收数据
			int ret = recv(client_fd, revData, 255, 0);
			if (ret > 0) {
				log_debug("recv data from client_fd, client_fd ip:port = {}:{}, ret data length = {}", ip, port, ret);
				print_recv_data(revData, ret);
				revData[ret] = 0x00;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(read_senser_interval));
		}

		closesocket(client_fd);
		break;
	}

	closesocket(listen_fd);

	return 0;
}

#ifdef USE_TILT_TEST_DATA

// 模拟数据线程回调
void TiltSensor::onMockThread() {

}

#endif // USE_TILT_TEST_DATA

#endif

#endif


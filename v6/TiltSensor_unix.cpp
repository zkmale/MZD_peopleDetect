#include "TiltSensor.h"

#ifdef __linux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


// 线程回调
void TiltSensor::onThread() {
	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		// log_critical("Create Socket Failed.");
		return;
	}
	struct sockaddr_in addr_server{};
	struct sockaddr_in addr_client{};

	memset(&addr_server, 0, sizeof(addr_server));
	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons(svr_port);
	addr_server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listen_fd, (struct sockaddr *) &addr_server, sizeof(struct sockaddr_in)) < 0) {
		// log_critical("Socket Bind Failed.");
		return;
	}

	if (listen(listen_fd, 5)) {
		// log_critical("Socket Listen Failed.");
		return;
	}

	int client_fd = -1;
	int addr_client_len = sizeof(struct sockaddr);
	char revData[255] = {0};

	while (is_running.load()) {
		if ((client_fd = accept(listen_fd, (struct sockaddr *) &addr_client, (socklen_t *) (&addr_client_len))) < 0) {
			continue;
		}
		if (client_fd < 0) {
			// log_error("accept error !");
			continue;
		}

		auto ip = inet_ntoa(addr_client.sin_addr);
		int port = ntohs(addr_client.sin_port);
		// log_info("New Client Connect, client_fd ip = {}, client_fd port = {}", ip, port);
		std::cout << "New Client Connect, client_fd ip:port ==> " << ip << ":" <<  port << std::endl;

		while (is_running.load()) {
			// 发送数据
			const char *sendData = send_buf;
			send(client_fd, sendData, 8, 0);

			// 接收数据
			ssize_t ret = recv(client_fd, revData, 255, 0);
			if (ret > 0) {
				// log_debug("recv data from client_fd, client_fd ip:port = {}:{}, ret data length = {}", ip, port, ret);
				std_log << "recv data from client_fd, client_fd ip:port = "<< ip << ":" << port << ", ret data length = " << ret;
				parse_recv_data(revData, ret);
				revData[ret] = 0x00;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(request_tilt_data_interval_ms));
		}
		close_socket(client_fd);
	}
	close_socket(listen_fd);
}

void TiltSensor::close_socket(SOCKET sock_fd) {
	close(sock_fd);
}

#endif // __linux
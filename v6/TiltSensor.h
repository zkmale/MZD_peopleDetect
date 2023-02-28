// #ifndef TILTSENSOR_H
//#define TILTSENSOR_H

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <cstring>
#include <cmath>
#include <iostream>

#include "log.hpp"


typedef int SOCKET;


//// 倾角传感器数据处理
class TiltSensor {
public:
	TiltSensor() {
		// 接收数据默认回调函数
		std::function<void(float, float)> onHandle_fb;
		onHandle_fb = [&](float x, float y) {
			onHandleData(x, y);
		};

		// 结果处理回调函数: 设置默认函数
		setSensorResultFb(onHandle_fb);

		// 倾角传感器数据请求时间间隔, ms
		request_tilt_data_interval_ms = 1000;

		// ip port
		svr_ip = std::string("127.0.0.1");
		svr_port = 9100;

		// 默认值
		tilt_x.store(1.0f);
		tilt_y.store(1.0f);
	}

	~TiltSensor() {
		StopTiltSensor();
	}

	// 设置本地ip和端口
	void SetServerAddr(const std::string &ip, int port) {
		svr_ip = ip;
		svr_port = port;
	}

	// 获取数值
	void GetTiltData(float &x, float &y) {
		x = tilt_x.load();
		y = tilt_y.load();
	}

	// 设置结果回调函数
	void setSensorResultFb(const std::function<void(float, float)> &fb) {
		res_fb = fb;
	}

	// 启动
	void StartTiltSensor() {
		is_running.store(true);
		thd = std::thread([&]() {
			onThread();
		});
	}

	// 停止
	void StopTiltSensor() {
		close_socket(listen_fd);
		is_running.store(false);

		if (thd.joinable()) {
			thd.join();
		}
	}

	// 模拟数据
	void MockData();


private:
	// 线程回调
	void onThread();
	
	void onHandleData(float x, float y);


	// 模拟数据线程回调
	void onMockThread();

	// 回收资源
	void close_socket(SOCKET sock_fd);

	// 解析数据
	void parse_recv_data(const char *recv_data, size_t size);

	// 解析模拟数据
	void parse_mock_data(const char *recv_data, size_t size);

	// (默认)回调函数: 处理数据

private:
	std::string svr_ip;
	int svr_port;

	std::thread thd;
	std::atomic_bool is_running;

	SOCKET listen_fd;

	std::function<void(float, float)> res_fb;

	// 请求时间间隔
	int request_tilt_data_interval_ms; 

	// 请求倾角传感器数据
	char send_buf[8] = {  // 这里不需要转换字节序
			0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09
	};

	// 倾角传感器数值
	std::atomic<float> tilt_x;
	std::atomic<float> tilt_y;
};


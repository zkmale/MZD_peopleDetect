#include "TiltSensor.h"

void TiltSensor::parse_recv_data(const char *recv_data, size_t size)
{
#if 0
	// 打印数据
	for (int i = 0; i < size; ++i) {
			printf("0x%02X ", recv_data[i]);
	}
	printf("\n");
	fflush(stdout);
	// 0x01 0x03 0x08 0xBF 0x38 0xA3 0xE0 0x3E 0x87 0x6E 0x48 0x6F 0xF2
#endif

	// 寄存器数据长度
	int len = recv_data[2];
	if (size < 13 || len < 8)
	{
		// log_error("recv data length < 13, len = {}, size = {}", len, size);
		return;
	}

	float x = 0.0f;
	float y = 0.0f;

	// 第一个传感器寄存器
	uint8_t x_data[4] = {0};
	x_data[0] = recv_data[6]; //
	x_data[1] = recv_data[5]; //
	x_data[2] = recv_data[4]; //
	x_data[3] = recv_data[3]; //
	memcpy(&x, x_data, sizeof(x_data));

	// 第一个传感器寄存器
	uint8_t y_data[4] = {0};
	y_data[0] = recv_data[10]; //
	y_data[1] = recv_data[9];  //
	y_data[2] = recv_data[8];  //
	y_data[3] = recv_data[7];  //
	memcpy(&y, y_data, sizeof(y_data));

	if (std::fabs(x) < 1e-6)
	{
		// printf("%f\n", std::fabs(x));
		// log_info("x = 0.0f");
		x = 0.0f;
	}

	if (std::fabs(y) < 1e-6)
	{
		// printf("%f\n", std::fabs(y));
		// log_info("y = 0.0f");
		y = 0.0f;
	}

	char print_data[512] = {0};
	sprintf(print_data, "x = %f, y = %f", x, y);
	// log_info("{}", print_data);
	// log_info("x = {}, y = {}", x, y);

	// 调用回调函数
	if (res_fb)
	{
		res_fb(x, y);
	}
}

// 解析模拟数据
void TiltSensor::parse_mock_data(const char *recv_data, size_t size)
{
	// 寄存器数据长度
	int len = recv_data[2];
	if (size < 13 || len < 8)
	{
		// log_error("recv data length < 13, len = {}, size = {}", len, size);
		return;
	}

	float x = 0.0f;
	float y = 0.0f;

	// 第一个传感器寄存器
	uint8_t x_data[4] = {0};
	x_data[0] = recv_data[6]; //
	x_data[1] = recv_data[5]; //
	x_data[2] = recv_data[4]; //
	x_data[3] = recv_data[3]; //
	memcpy(&x, x_data, sizeof(x_data));

	// 第一个传感器寄存器
	uint8_t y_data[4] = {0};
	y_data[0] = recv_data[10]; //
	y_data[1] = recv_data[9];  //
	y_data[2] = recv_data[8];  //
	y_data[3] = recv_data[7];  //
	memcpy(&y, y_data, sizeof(y_data));

	if (std::fabs(x) < 1e-6)
	{
		x = 0.0f;
	}
	if (std::fabs(y) < 1e-6)
	{
		y = 0.0f;
	}

	char print_data[512] = {0};
	sprintf(print_data, "(x, y) ==> (%f, %f) ", x, y);

	static float mock_x = 0.0f;
	static float mock_y = 0.0f;
	mock_x += 0.01f;
	mock_y += 0.01f;
	char mock_print_data[512] = {0};
	sprintf(mock_print_data, "(mock_x, mock_y) ==> (%f, %f) ", mock_x, mock_y);

	// 调用回调函数
	if (res_fb)
	{
		res_fb(mock_x, mock_y);
	}
}

// 模拟数据线程回调
void TiltSensor::onMockThread()
{
	while (is_running.load())
	{
		MockData();
	}
}

// 模拟数据
void TiltSensor::MockData()
{
	uint8_t recv_data[] = {
		0x01, 0x03, 0x08, 0xBF, 0x38, 0xA3, 0xE0, 0x3E, 0x87, 0x6E, 0x48, 0x6F, 0xF2};

	// 使用模拟数据
	parse_mock_data((const char *)recv_data, sizeof(recv_data));

	// 模拟数据使用间隔
	std::this_thread::sleep_for(std::chrono::milliseconds(request_tilt_data_interval_ms));
}

// (默认)回调函数: 处理数据
void TiltSensor::onHandleData(float x, float y)
{
	std_log << "x = " << x << " y = " << y << std::endl;
	
	// 倾角传感器数值
	tilt_x.store(x);
	tilt_y.store(y);
}
//#include "opencv2/opencv.hpp"

//#include <QDir>
#include <fstream>
#include <unistd.h>
//#include "auto_entercs.h"

#include "HCNetSDK.h"
#include "PlayM4.h"
#include "LinuxPlayM4.h"
#include "WindNetPredictDetect.h"
#include "TiltSensor.h"
#include <X11/Xlib.h>
#include <queue>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

using namespace cv;
using namespace std;
using namespace std::chrono; // calc fps

LONG lUserID;

double fps()
{
    static double fps = 0.0;
    static int frameCount = 0;
    static auto lastTime = system_clock::now();
    static auto curTime = system_clock::now();

    curTime = system_clock::now();

    auto duration = duration_cast<microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * microseconds::period::num / microseconds::period::den;

    if (duration_s > 2)//2秒之后开始统计FPS
    {
        fps = frameCount / duration_s;
        frameCount = 0;
        lastTime = curTime;
    }
    ++frameCount;
    return fps;
}

void frame_write(vector<Mat>& frame_buffer) {
    cout << "this is write" << endl;
    //球机信息
    char IP[] = "192.168.8.31";   //����������������ͷ��ip
    char UName[] = "admin";                 //����������������ͷ���û���
    char PSW[] = "a123456789";           //����������������ͷ������
    NET_DVR_Init();
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(1000, true);
    NET_DVR_SetLogToFile(3, "./sdkLog1");
    NET_DVR_DEVICEINFO_V30 struDeviceInfo = { 0 };
    NET_DVR_SetRecvTimeOut(5000);
    lUserID = NET_DVR_Login_V30(IP, 8000, UName, PSW, &struDeviceInfo);
    //NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);


    Mat input, blob;
    VideoCapture capture;
    capture.open("rtsp://admin:a123456789@192.168.8.31:554/h264/ch1/main/av_stream/1");
    if (capture.isOpened())
    {
        cout << "Capture is opened" << endl;
        for (;;)
        {
            capture >> input;
            if (input.empty()){
				std::cout << "image is empty " << std::endl;
                continue;
				}
			if (frame_buffer.size() < 100000000){
				//std::cout << "frame_buffer get input" << std::endl;
                frame_buffer.push_back(input);
			}
			else {
                cout << "thread ==============> after read stop, frame_buffer.size() > 100 , write stop";
                return;
            }
        }
    }
    else {
        cout << "open camera failed" << endl;
    }
}


int  HexToDecMa(int wHex)//ʮ������תʮ����
{
    return (wHex / 4096) * 1000 + ((wHex % 4096) / 256) * 100 + ((wHex % 256) / 16) * 10 + (wHex % 16);
}

int DEC2HEX_doc(int x)//ʮ����תʮ������
{
    return (x / 1000) * 4096 + ((x % 1000) / 100) * 256 + ((x % 100) / 10) * 16 + x % 10;
}

void contral_cam(int wPanPos1, int wTiltPos1, int wZoomPos1, DWORD dwtmp, NET_DVR_PTZPOS m_ptzPosCurrent){

    bool a = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_PTZPOS, 0, &m_ptzPosCurrent, sizeof(NET_DVR_PTZPOS), &dwtmp);
    //转换相机信息到十进制
    int m_iPara1 = HexToDecMa(m_ptzPosCurrent.wPanPos);
    int m_iPara2 = HexToDecMa(m_ptzPosCurrent.wTiltPos);
    int m_iPara3 = HexToDecMa(m_ptzPosCurrent.wZoomPos);
    std::vector<int> TempPosture(3);
    TempPosture[0] = m_iPara1 / 10 ;    //P水平方向
    TempPosture[1] = m_iPara2 / 10 ;    //T仰角
    TempPosture[2] = m_iPara3 / 10 ;    //Z焦距
    
	std::cout << "TempPosture[2] = " << TempPosture[2] << std::endl;
    
    //如果大于1将焦距调到1
    //将俯仰角调到90°
    m_ptzPosCurrent.wPanPos = DEC2HEX_doc(wPanPos1*10);
    m_ptzPosCurrent.wTiltPos = DEC2HEX_doc(wTiltPos1*10);
    m_ptzPosCurrent.wZoomPos = DEC2HEX_doc(wZoomPos1*10);
    
    bool b = NET_DVR_SetDVRConfig(lUserID, NET_DVR_SET_PTZPOS, 0, &m_ptzPosCurrent, sizeof(NET_DVR_PTZPOS));
    sleep(2);
}

int display_wTiltPos_cam(DWORD dwtmp, NET_DVR_PTZPOS m_ptzPosCurrent){
    bool a = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_PTZPOS, 0, &m_ptzPosCurrent, sizeof(NET_DVR_PTZPOS), &dwtmp);
    int m_iPara2 = HexToDecMa(m_ptzPosCurrent.wTiltPos);
    return m_iPara2 / 10 ;    //T仰角
}

#include "httplib.h"

/*!
 * 通过http请求请教传感器数据
 * x: 倾角传感器数据; 传出参数;
 * y: 倾角传感器数据; 传出参数;
 * return : false, 请求失败或参数解析失败;
 * */
bool function_GetAngle(float &x, float &y) {
    std::cout << "222222222" << std::endl;

    static auto client = httplib::Client("127.0.0.1", 18080);
    auto result = client.Get("/TiltSensor");
    if (!result || result->status != 200) {
        return false;
    }

    auto body = result->body;

    cJSON *root = cJSON_Parse(body.c_str());
    if(!root) {
        return false;
    }

    if(cJSON_GetObjectItem(root, "tilt_x")){
        x = std::abs((float)cJSON_GetObjectItem(root, "tilt_x")->valuedouble);
    } else {
        return false;
    }


    if(cJSON_GetObjectItem(root, "tilt_y")){
        y = (float)cJSON_GetObjectItem(root, "tilt_y")->valuedouble;
    }else {
        return false;
    }

    return true;
}


//void getframe1(std::vector<cv::Mat>& frame_buffer) {
//void *getframe1(void *arg) {
void frame_read(vector<Mat>& frame_buffer) {
	//std::vector<cv::Mat> frame_buffer = *(std::vector<cv::Mat>*)arg;
	std::cout << "this is read" << std::endl;
    clock_t start, end;
    //***************************************************************
    //***************************************************************
    //读取配置文件
    Json::Reader reader;
	Json::Value root;
    std::ifstream in("./output.json", std::ios::binary);
    if (!in.is_open())
	{
		std::cout << "Error opening file\n";
	}
    std::string net_bins_people;
    std::string net_paras_people;
    std::string labels;
    int is_save;
    int interval;
    int Pans;
    int Tilts;
    int Zooms;
    int is_read_rtsp_sign;
    int people_area_x;
    int people_area_y;
    int area;


    float standard_bijia_angles;  //臂架 Tilt
    float standard_cam_angle; //俯仰角 Tilt
    int standard_cam_Pans; //旋转角  Pan
    int angle_interval = 150;
    float distance_angle_threshold = 1.0;

    int min_points_x;
    int min_points_y;
    int max_points_x;
    int max_points_y;
    int width_people_area;
    int heigth_people_area;

    int smoothing_factor = 5;
    int send_interval = 150;

    if (reader.parse(in, root))
	{
		net_bins_people = root["net_bin"].asString();
		net_paras_people = root["net_param"].asString();
		Pans = root["Pan"].asInt();
        Tilts = root["Tilt"].asInt();
        Zooms = root["Zoom"].asInt();
        is_read_rtsp_sign = root["is_read_rtsp_sign"].asInt();
		is_save = root["is_save_img"]["is_save"].asInt();
		interval = root["is_save_img"]["interval"].asInt();
        labels = root["label"].asString();
        people_area_x = root["people_area_x"].asInt();
        people_area_y = root["people_area_y"].asInt();
        area = root["area"].asInt();
        standard_bijia_angles = root["Tilt_sensor"]["standard_bijia_angles"].asFloat();
        standard_cam_angle = root["Tilt_sensor"]["standard_cam_angle"].asFloat();
        standard_cam_Pans = root["Tilt_sensor"]["standard_cam_Pans"].asInt();
        angle_interval = root["Tilt_sensor"]["angle_interval"].asInt();
        distance_angle_threshold = root["Tilt_sensor"]["distance_angle_threshold"].asFloat();
        min_points_x = root["Tilt_sensor"]["min_Pans_people_area"]["min_points_x"].asInt();
        min_points_y = root["Tilt_sensor"]["min_Pans_people_area"]["min_points_y"].asInt();
        max_points_x = root["Tilt_sensor"]["max_Pans_people_area"]["max_points_x"].asInt();
        max_points_y = root["Tilt_sensor"]["max_Pans_people_area"]["max_points_y"].asInt();
        width_people_area = root["Tilt_sensor"]["width_people_area"].asInt();
        heigth_people_area = root["Tilt_sensor"]["heigth_people_area"].asInt();

        smoothing_factor = root["smoothing_factor"].asInt();
        send_interval = root["send_interval"].asInt();


        //const char* net_params = net_para.c_str();
        //const char* net_bins = net_bin.c_str();
		std::cout << "net_bins =  " << net_bins_people << std::endl;
		std::cout << "net_params =  " << net_paras_people << std::endl;
		std::cout << "Pans =  " << Pans << std::endl;
        std::cout << "Tilts =  " << Tilts << std::endl;
		std::cout << "Zooms =  " << Zooms << std::endl;
		std::cout << "interval =  " << interval << std::endl;
        std::cout << "net_bins_people =  " << net_bins_people << std::endl;
		std::cout << "net_paras_people =  " << net_paras_people << std::endl;
		std::cout << "standard_bijia_angles =  " << standard_bijia_angles << std::endl;
		std::cout << "angle_interval =  " << angle_interval << std::endl;
		std::cout << "distance_angle_threshold =  " << distance_angle_threshold << std::endl;
		std::cout << "min_points_x =  " << min_points_x << std::endl;
		std::cout << "min_points_y =  " << min_points_y << std::endl;
		std::cout << "max_points_x =  " << max_points_x << std::endl;
		std::cout << "max_points_y =  " << max_points_y << std::endl;
		std::cout << "width_people_area =  " << width_people_area << std::endl;
		std::cout << "smoothing_factor =  " << smoothing_factor << std::endl;
		std::cout << "send_interval =  " << send_interval << std::endl;

    }
    else
	{
        net_bins_people = "./models/hook_wlxd_exp14.bin";
        net_paras_people = "./models/hook_wlxd_exp14.param";
        is_save = 0;
        interval = 1500;
        Pans = 11;
        Tilts = 87;
        Zooms = 7;
        is_read_rtsp_sign = 0;
		is_save = 0;
		interval = 1500;
        labels = "danger_zone";
        people_area_x = 438;
        people_area_y = 602;
        area = 350;
        standard_bijia_angles = 61.6;
        standard_cam_angle = 90.0;
        standard_cam_Pans = 73;
        angle_interval = 1500;
        min_points_x = 0;
        min_points_y = 200;
        max_points_x = 1760;
        max_points_y = 440;
        width_people_area = 800;
        heigth_people_area = 800;

		std::cout << "parse error\n" << std::endl;
	}
	in.close();
    //*****************************************************************
    //*****************************************************************

    //**************************
    //创建倾角传感器
    void* p_TiltSensor_class;
    p_TiltSensor_class = (void*)(new TiltSensor());
    TiltSensor* get_TiltSensor_data = (TiltSensor*)(p_TiltSensor_class);


    void* p_algorithm_people;
    p_algorithm_people = (void*)(new WindNetDetect());
    //std::string net_bins_people = "./models/people_exp0.bin";
    //std::string net_paras_people = "./models/people_exp0.param";

    int init_res_people = ((WindNetDetect*)p_algorithm_people)->init1(net_bins_people, net_paras_people);
    WindNetDetect* tmp_people = (WindNetDetect*)(p_algorithm_people);

	cv::Mat frame1;
	//pthread_mutex_lock(&mutex);
	int image_name = 1;
	
    //调焦参数定义
    NET_DVR_PTZPOS m_ptzPosCurrent;
    DWORD dwtmp;
    //std::vector<int> TempPosture(3);


    //控制球机焦距和俯仰角
    //contral_cam(11, 88, 7, dwtmp, m_ptzPosCurrent);
    contral_cam(Pans, Tilts, Zooms, dwtmp, m_ptzPosCurrent);
    int contral_num = 0;
    
    //cv::namedWindow("people");
    //cv::namedWindow("hook");
    cv::Mat img_roi;
    cv::Mat image;

    //***************************************************************
    //***************************************************************
    //裁剪行人检测区域
    cv::Rect rect;
    cv::Rect min_pans_rect;
    cv::Rect max_pans_rect;
    std::vector<std::string> people_area;
    if(is_read_rtsp_sign == 1){
        //*************************************************************
        //读取json文件，提取区域信息。
        std::vector<int> pointss;
        pointss = tmp_people->readFileJson();
        std::cout<<"pointss[0] = " << pointss[0] << std::endl;
        std::cout<<"pointss[1] = " << pointss[1] << std::endl;
        std::cout<<"pointss[2] = " << pointss[2] << std::endl;
        std::cout<<"pointss[3] = " << pointss[3] << std::endl;
        rect.x = pointss[0];
        rect.y = pointss[1];
        rect.width = std::abs(pointss[2] - pointss[0]);
        rect.height = std::abs(pointss[3] - pointss[1]);
        //rect(pointss[0], pointss[1], std::abs(pointss[2] - pointss[0]), std::abs(pointss[3] - pointss[1]));
        //people_area.push_back(std::to_string(pointss[0]));
        //people_area.push_back(std::to_string(pointss[1]));
        //people_area.push_back(std::to_string(pointss[2]));
        //people_area.push_back(std::to_string(pointss[3]));
    }
    else{
        //int people_x = 438; //738
        //int people_y = 602;
        //int area = 300;
        //rect.x = people_area_x - area;
        //rect.y = people_area_y - area;
        //rect.width = 2*area;
        //rect.height = 2*area;
        min_pans_rect.x = min_points_x;
        min_pans_rect.y = min_points_y;
        min_pans_rect.width = width_people_area;
        min_pans_rect.height = heigth_people_area;

        max_pans_rect.x = max_points_x;
        max_pans_rect.y = max_points_y;
        max_pans_rect.width = width_people_area;
        max_pans_rect.height = heigth_people_area;

        rect.x = min_points_x;
        rect.y = min_points_y;
        rect.width = width_people_area;
        rect.height = heigth_people_area;
        

        //rect(people_x - area, people_y - area, 2*area + 260, 2*area + 260);
        //rect(138, 302, 860, 860);

        //people_area.push_back(std::to_string(people_x - area));
        //people_area.push_back(std::to_string(people_y - area));
        //people_area.push_back(std::to_string(2*area + 260));
        //people_area.push_back(std::to_string(2*area + 260));
    }

    
    //cv::Rect rect(people_x - area, people_y - area, 2*area + 260, 2*area + 260);
    //***************************************************************
    //***************************************************************

    //cv::Rect rect;
    
    int k = 1;
    int img_name = 1;
    
    //int angle_interval = 1500;
    //int angle_count = 1;

    get_TiltSensor_data->StartTiltSensor();
    // 倾角传感器数值
    //float angle_x = 0.0f;
    float angle = 0.0f;
    float angle_y = 0.0f;

    float distance_angle;
    float origin_bijia_angles = standard_bijia_angles;

    float result_cam_angle;

    int smoothing_warn_it = 0;


    while(1){
        sleep(1);
        if (!frame_buffer.empty()) {
            frame1 = frame_buffer.back();
			//pthread_cond_signal(&cond);
			frame_buffer.pop_back();
            
            std::vector<Object> objects_people;
            start = clock();
            if (!frame1.empty()){
                //*******************************************************
                //*******************************************************
                //通过倾角传感器的臂架角度来控制球机的拍摄角度
                
                if(k%angle_interval == 10){ 
                    get_TiltSensor_data->GetTiltData(angle, angle_y);
                    std::cout << "angle_x = " << angle << " angle_y = " << angle_y << std::endl;

                    distance_angle = origin_bijia_angles - angle;
                    std::cout << "distance_angle = " << distance_angle << std::endl;

                    if(std::abs(distance_angle) > distance_angle_threshold){
                        origin_bijia_angles = angle;
                        std::cout <<"##########change cam angle############";
                        //设置标准的时候，旋转角度必须小于180°
                        result_cam_angle = standard_cam_angle + angle - standard_bijia_angles;
                        std::cout << "standard_cam_Pans = " << standard_cam_Pans << std::endl;
                        std::cout << "result_cam_angle = " << result_cam_angle << std::endl;

                        if(result_cam_angle <= 90){
                            //***************************
                            //表示旋转角度小于180°  
                            contral_cam(standard_cam_Pans, result_cam_angle, 5, dwtmp, m_ptzPosCurrent);
                            rect.x = min_points_x;
                            rect.y = min_points_y;
                            rect.width = width_people_area;
                            rect.height = heigth_people_area;
                        }
                        else{
                            contral_cam(180 + standard_cam_Pans, 180 - result_cam_angle, 5, dwtmp, m_ptzPosCurrent);
                            rect.x = max_points_x;
                            rect.y = max_points_y;
                            rect.width = width_people_area;
                            rect.height = heigth_people_area;
                        }
                    }
                }
                //**********************************************************
                //**********************************************************
        
                img_roi = frame1(rect);
                tmp_people->detect(img_roi, objects_people);

                for (size_t i = 0; i < objects_people.size(); i++)
                {
                    const Object& obj = objects_people[i];

                    fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.label, obj.prob,
                        obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);

                    cv::rectangle(img_roi, obj.rect, cv::Scalar(255, 0, 0), 2);

                    char text_people[256];
                    //sprintf(text_people, "%s", class_names_people[obj.label]);
                    //sprintf(text, "%s %.1f%%", class_names[obj.label], obj.prob * 100);

                    int baseLine = 1;
                    cv::Size label_size = cv::getTextSize(text_people, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

                    int x_people = obj.rect.x;
                    int y_people = obj.rect.y - label_size.height - baseLine;
                    if (y_people < 0)
                        y_people = 0;
                    if (x_people + label_size.width > img_roi.cols)
                        x_people = img_roi.cols - label_size.width;
                    

                    cv::rectangle(img_roi, cv::Rect(cv::Point(x_people, y_people), cv::Size(label_size.width, label_size.height + baseLine)),
                        cv::Scalar(255, 255, 255), -1);

                    cv::putText(img_roi, text_people, cv::Point(x_people, y_people + label_size.height),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
                }

            }
            if(k%interval == 2 && is_save == 1){
                cv::imwrite("./data/" + std::to_string(img_name) + ".jpg", img_roi);
                img_name++;
            }
            k++;

            if(objects_people.size() > 0){
                smoothing_warn_it++;
                std::cout << "smoothing_warn_it" << smoothing_warn_it << std::endl;
                if(smoothing_warn_it >= smoothing_factor){
                    if(smoothing_warn_it % send_interval == smoothing_factor){
                        tmp_people->send_json_people(img_roi, labels, "warn");
                    }
                }
            }
            else{
                smoothing_warn_it = 0;
            }

            //tmp->draw_objects(frame1, objects);
            objects_people.clear();
            end = clock();
            float rumtime = (float)(end - start) / CLOCKS_PER_SEC;
            std::stringstream buf;
            buf.precision(3);	//瑕嗙洊榛樿?ょ簿搴?
            buf.setf(std::ios::fixed);	//淇濈暀灏忔暟浣?
            buf << rumtime;
            std::string strtime;
            strtime = buf.str();
            //std::cout << "strtime22222 = " << strtime << std::endl;
            //sleep(100);
            if (frame_buffer.size() > 10) { // 隔帧抽取一半删除
				auto iter = frame_buffer.erase(frame_buffer.begin(), frame_buffer.end() - 5);
                //auto iter = frame_buffer.begin();
                //for (int inde = 0; inde < frame_buffer.size() / 2; inde++)
                //    frame_buffer.erase(iter++);
            }

        }
    }
	 //std::cout << "thread ==============> read stop" << std::endl;

}

#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
#if 1
    vector<Mat> frame_buffer;
    XInitThreads();
    std::thread tw(frame_write, ref(frame_buffer)); // pass by value
    std::thread tr(frame_read, ref(frame_buffer)); // pass by value

    tw.join();
    tr.join();
#else
    TiltSensor* get_TiltSensor_data = new TiltSensor();
    get_TiltSensor_data->StartTiltSensor();

    float angle = 0.0f;
    float angle_y = 0.0f;

    for (size_t i = 0; i < 10; i++)
    {
        get_TiltSensor_data->GetTiltData(angle, angle_y);
        std::cout << "angle_x = " << angle << " angle_y = " << angle_y << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(60*1000));
    }
    

#endif
    return 0;
}


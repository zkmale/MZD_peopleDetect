//#include "opencv2/opencv.hpp"

//#include <QDir>
#include <fstream>
#include <unistd.h>
//#include "auto_entercs.h"

#include "HCNetSDK.h"
#include "PlayM4.h"
#include "LinuxPlayM4.h"
#include "WindNetPredictDetect.h"
#include <X11/Xlib.h>
#include <queue>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>


#define HPR_ERROR       -1
#define HPR_OK               0
#define USECOLOR          0

static cv::Mat dst;
HWND h = NULL;
LONG nPort = -1;
LONG lUserID;



//pthread_mutex_t mutex;

//std::list<cv::Mat> g_frameList;

std::vector<cv::Mat> g_frameList;

FILE* g_pFile = NULL;

pthread_mutex_t lock;
pthread_cond_t cond;

void CALLBACK PsDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE* pPacketBuffer, DWORD nPacketSize, void* pUser)
{

    if (dwDataType == NET_DVR_SYSHEAD)
    {
        //д��ͷ����
        g_pFile = fopen("/home/lds/source/ps.dat", "wb");

        if (g_pFile == NULL)
        {
            printf("CreateFileHead fail\n");
            return;
        }

        //д��ͷ����
        fwrite(pPacketBuffer, sizeof(unsigned char), nPacketSize, g_pFile);
        printf("write head len=%d\n", nPacketSize);
    }
    else
    {
        if (g_pFile != NULL)
        {
            fwrite(pPacketBuffer, sizeof(unsigned char), nPacketSize, g_pFile);
            printf("write data len=%d\n", nPacketSize);
        }
    }

}

//void CALLBACK DecCBFun(LONG nPort, char *pBuf, LONG nSize, FRAME_INFO *pFrameInfo, LONG nReserved1, LONG nReserved2)
void CALLBACK DecCBFun(LONG nPort, char* pBuf, LONG nSize, FRAME_INFO* pFrameInfo, void* nReserved1, LONG nReserved2)
{
    long lFrameType = pFrameInfo->nType;

    if (lFrameType == T_YV12)
    {
        //cv::Mat dst(pFrameInfo->nHeight, pFrameInfo->nWidth,
        //            CV_8UC3);  // 8UC3��ʾ8bit uchar�޷�������,3ͨ��ֵ
        dst.create(pFrameInfo->nHeight, pFrameInfo->nWidth,
            CV_8UC3);

        cv::Mat src(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, (uchar*)pBuf);
        cv::cvtColor(src, dst, CV_YUV2BGR_YV12);
        if (!dst.empty())
            //continue;
			if (g_frameList.size() < 10000000){
				
				g_frameList.push_back(dst);
				//std::cout << "strtime33333 = " << std::endl;
			}
			else {
				std::cout << "thread ==============> after read stop, frame_buffer.size() > 100 , write stop";
				//return;
			}
        //pthread_mutex_lock(&mutex);
        //g_frameList.push_back(dst);
        //pthread_mutex_unlock(&mutex);
    }
    //usleep(100);

    //cv::Mat src(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, (uchar *)pBuf);
    //cv::cvtColor(src, dst, CV_YUV2BGR_YV12);
    //cv::imshow("bgr", dst);
    //pthread_mutex_lock(&mutex);
    //g_frameList.push_back(dst);
    //pthread_mutex_unlock(&mutex);
    //vw << dst;
    //cv::waitKey(10);

}

void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* dwUser)
{
    /*
    if (dwDataType == 1)
    {
        PlayM4_GetPort(&nPort);
        PlayM4_SetStreamOpenMode(nPort, STREAME_REALTIME);
        PlayM4_OpenStream(nPort, pBuffer, dwBufSize, 1024 * 1024);
        PlayM4_SetDecCallBackEx(nPort, DecCBFun, NULL, NULL);
        PlayM4_Play(nPort, h);
    }
    else
    {
        BOOL inData = PlayM4_InputData(nPort, pBuffer, dwBufSize);
    }*/
    DWORD dRet;
    switch (dwDataType)
    {
    case NET_DVR_SYSHEAD:           //ϵͳͷ
        if (!PlayM4_GetPort(&nPort))  //��ȡ���ſ�δʹ�õ�ͨ����
        {
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(nPort, STREAME_REALTIME)) {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }
            if (!PlayM4_OpenStream(nPort, pBuffer, dwBufSize, 1024 * 1024)) {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }
            //���ý���ص����� ֻ���벻��ʾ
           //  if (!PlayM4_SetDecCallBack(nPort, DecCBFun)) {
           //     dRet = PlayM4_GetLastError(nPort);
           //     break;
           //  }

            //���ý���ص����� ��������ʾ
            if (!PlayM4_SetDecCallBackEx(nPort, DecCBFun, NULL, NULL))
            {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }

            //����Ƶ����
            if (!PlayM4_Play(nPort, h))
            {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }

            //����Ƶ����, ��Ҫ�����Ǹ�����
            if (!PlayM4_PlaySound(nPort)) {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }
        }
        break;
        //usleep(500);
    case NET_DVR_STREAMDATA:  //��������
        if (dwBufSize > 0 && nPort != -1) {
            BOOL inData = PlayM4_InputData(nPort, pBuffer, dwBufSize);
            while (!inData) {
                sleep(100);
                inData = PlayM4_InputData(nPort, pBuffer, dwBufSize);
                std::cerr << "PlayM4_InputData failed \n" << std::endl;
            }
        }
        break;
    }
}

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void* pUser)
{
    char tempbuf[256] = { 0 };
    std::cout << "EXCEPTION_RECONNECT = " << EXCEPTION_RECONNECT << std::endl;
    switch (dwType)
    {
    case EXCEPTION_RECONNECT:	//Ԥ��ʱ����
        printf("pyd----------reconnect--------%d\n", time(NULL));
        break;
    default:
        break;
    }
}

//void* RunIPCameraInfo(void*)
void *RunIPCameraInfo(void *arg)
{
    char IP[] = "192.168.8.31";   //����������������ͷ��ip
    char UName[] = "admin";                 //����������������ͷ���û���
    char PSW[] = "a123456789";           //����������������ͷ������
    NET_DVR_Init();
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(1000, true);
    NET_DVR_SetLogToFile(3, "./sdkLog");
    NET_DVR_DEVICEINFO_V30 struDeviceInfo = { 0 };
    NET_DVR_SetRecvTimeOut(5000);
    lUserID = NET_DVR_Login_V30(IP, 8000, UName, PSW, &struDeviceInfo);

    NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);

    long lRealPlayHandle;
    NET_DVR_CLIENTINFO ClientInfo = { 0 };

    ClientInfo.lChannel = 1;
    ClientInfo.lLinkMode = 0;
    ClientInfo.hPlayWnd = 0;
    ClientInfo.sMultiCastIP = NULL;
	//pthread_cond_wait(&cond,&lock);

    //lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, PsDataCallBack, NULL, 0);
    lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, g_RealDataCallBack_V30, NULL, 0);
    //NET_DVR_SaveRealData(lRealPlayHandle, "/home/lds/source/yuntai.mp4");
    if (lRealPlayHandle < 0)
    {
        printf("pyd1---NET_DVR_RealPlay_V30 error\n");
    }
    sleep(-1);

    NET_DVR_Cleanup();
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
        x = (float)cJSON_GetObjectItem(root, "tilt_x")->valuedouble;
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
void *getframe1(void *arg) {
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
    int origin_bijia_angles;

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
        origin_bijia_angles = root["origin_bijia_angle"].asInt();


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
		std::cout << "origin_bijia_angles =  " << origin_bijia_angles << std::endl;
    }
    else
	{
        net_bins_people = "./models/hook_wlxd_exp1.bin";
        net_paras_people = "./models/hook_wlxd_exp1.param";
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
        origin_bijia_angles = 62.9;
		std::cout << "parse error\n" << std::endl;
	}
	in.close();
    //*****************************************************************
    //*****************************************************************

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
        rect.x = people_area_x - area;
        rect.y = people_area_y - area;
        rect.width = 2*area;
        rect.height = 2*area;
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
    
    int angle_interval = 1500;
    int angle_count = 1;
    float angle;
    float angle_x = 0.0f;
    float angle_y = 0.0f;
    float distance_angle;
    float x_angle;
    float y_angle;

    while(1){
        if (!g_frameList.empty()) {
            frame1 = g_frameList.back();
			//pthread_cond_signal(&cond);
			g_frameList.pop_back();

            //*******************************************************
            //*******************************************************
            //通过倾角传感器的臂架角度来控制球机的拍摄角度
            if(angle_count%angle_interval == 10){
                function_GetAngle(x_angle, y_angle);
                std::cout << "x_angle = " << x_angle << std::endl;
                std::cout << "y_angle = " << y_angle << std::endl;
                distance_angle = std::abs(origin_bijia_angles - angle);
                if(distance_angle > 2){
                    origin_bijia_angles = angle;
                    if(angle > 62.9){
                        //***************************
                        //表示旋转角度小于180°
                        contral_cam(73, 27.1 + angle, 5, dwtmp, m_ptzPosCurrent);
                    }
                    else{
                        contral_cam(253, 152.9 - angle, 5, dwtmp, m_ptzPosCurrent);
                    }
                }
            }
            //**********************************************************
            //**********************************************************

            std::vector<Object> objects_people;
            start = clock();
            if (!frame1.empty()){
                img_roi = frame1(rect);
                tmp_people->detect(img_roi, objects_people);

                for (size_t i = 0; i < objects_people.size(); i++)
                {
                    const Object& obj = objects_people[i];

                    fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.label, obj.prob,
                        obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);

                    cv::rectangle(img_roi, obj.rect, cv::Scalar(255, 0, 0));

                    char text_people[256];
                    sprintf(text_people, "%s", class_names_people[obj.label]);
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
                tmp_people->send_json_people(img_roi, labels, "NO");
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
            std::cout << "strtime22222 = " << strtime << std::endl;
            //sleep(100);
            if (g_frameList.size() > 10) { // 隔帧抽取一半删除
				auto iter = g_frameList.erase(g_frameList.begin(), g_frameList.end() - 5);
                //auto iter = frame_buffer.begin();
                //for (int inde = 0; inde < frame_buffer.size() / 2; inde++)
                //    frame_buffer.erase(iter++);
            }

        }
    }
	 //std::cout << "thread ==============> read stop" << std::endl;

}

int main(int argc, char* argv[])
{
    //pthread_t getframe;
    XInitThreads();
	pthread_mutex_init(&lock,NULL);//初始化锁
	pthread_cond_init(&cond,NULL);//初始化环境变量
	
	pthread_t t1;
	pthread_t t2;

	pthread_create(&t1,NULL,RunIPCameraInfo,NULL);
	pthread_create(&t2,NULL,getframe1, NULL);

	pthread_join(t1,NULL);
	pthread_join(t2,NULL);

	
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&cond);

    //std::thread tw(RunIPCameraInfo);
    //std::thread tr(getframe1, ref(g_frameList));
    //tw.join();
    //tr.join();

    
    //ret = pthread_create(&getframe, NULL, RunIPCameraInfo, NULL);

    return 0;
}


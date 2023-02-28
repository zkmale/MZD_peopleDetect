//
// Created by xx on 2022/11/5.
//

#ifndef HTTP_DEMO_IMGINFO_H
#define HTTP_DEMO_IMGINFO_H

#include <utility>
#include <vector>

#include "cJSON.h"
#include "base64.h"

class ImgInfo {
private:
    std::string img;
//    std::vector<std::string> label;
    std::string resp;
    std::string type;
    std::string start_algorithm_time;
    std::string detection_time;
    std::string solve_time;
    std::string level;
    std::vector<std::string> label;
public:
    //ImgInfo() = default;

    ImgInfo(std::string img, std::string resp, std::string type, std::string level) : img(
            std::move(
                    img)), resp(std::move(resp)), type(std::move(type)), level(std::move(level)) {}

    ImgInfo(std::string img, std::string resp, std::string type,
            std::string startAlgorithmTime, std::string detectionTime, std::string solveTime,
            std::string level, const std::vector<std::string> &label) : img(std::move(img)), resp(std::move(resp)),
                                                                        type(std::move(type)),
                                                                        start_algorithm_time(
                                                                                std::move(startAlgorithmTime)),
                                                                        detection_time(std::move(detectionTime)),
                                                                        solve_time(std::move(solveTime)),
                                                                        level(std::move(level)),
                                                                        label(label) {}

    std::string tostring() {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "img", img.c_str());
        cJSON_AddStringToObject(root, "resp", resp.c_str());
        cJSON_AddStringToObject(root, "type", type.c_str());
        cJSON_AddStringToObject(root, "level", level.c_str());
        cJSON_AddStringToObject(root, "start_algorithm_time", start_algorithm_time.c_str());
        cJSON_AddStringToObject(root, "detection_time", detection_time.c_str());
        cJSON_AddStringToObject(root, "solve_time", solve_time.c_str());
        cJSON *label_array = cJSON_CreateArray();
        for (auto &i: label) {
            cJSON_AddItemToArray(label_array, cJSON_CreateString(i.c_str()));
        }
        cJSON_AddItemToObject(root, "label", label_array);
        char *out = cJSON_Print(root);
        std::string result(out);
        free(out);
        cJSON_Delete(root);
        return result;
    }

};

#endif //HTTP_DEMO_IMGINFO_H

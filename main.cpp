#define NOMINMAX

#include "global.hpp"
#include "ByteTrack/BYTETracker.h"
#include "ByteTrack/Object.h"
#include "ByteTrack/Rect.h"
#include "ByteTrack/STrack.h"
#include "curl/curl.h"
#include "json.hpp"

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cstddef>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>
#include <ncnn/net.h>
#include <ncnn/mat.h>
#include <opencv2/dnn.hpp>

const float mean_vals[3] = {0.f, 0.f, 0.f};
const float norm_vals[3] = {
    1.0f / 255.0f,
    1.0f / 255.0f,
    1.0f / 255.0f
};
const int stopNum = 0;
const int seats = 45;
const float lat = 20.272410;
const float lng = 85.833850;
uint16_t netP = seats;

ncnn::Net net;
ncnn::Mat out0;
using json = nlohmann::json;

byte_track::BYTETracker tracker(30, 30, 0.5f, 0.6f, 0.8f);
std::map<size_t, uint8_t> trackerId;
std::vector<byte_track::Object> objects;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    auto* responseString = static_cast<std::string*>(userp);
    responseString->append(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}

cv::Mat letterbox(const cv::Mat& frame) {
    int w = frame.cols;
    int h = frame.rows;

    float scale = std::min(640.0f / w, 640.0f / h);

    int new_w = static_cast<int>(std::round(w * scale));
    int new_h = static_cast<int>(std::round(h * scale));

    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(new_w, new_h), 0, 0, cv::INTER_LINEAR);

    int dw = 640 - new_w;
    int dh = 640 - new_h;

    int left   = dw / 2;
    int right  = dw - left;
    int top    = dh / 2;
    int bottom = dh - top;

    cv::Mat output;
    cv::copyMakeBorder(resized, output, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
    return output;
}

uint8_t getPointSide(cv::Point2f p, cv::Point2f a, cv::Point2f b) {
    float cross = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);

    if (cross > 0)
        return 1; // DOWN

    if (cross < 0)
        return 2; // UP

    return 0; // ON LINE
}

void getResponse(CURL* curl, const char* url, std::string* response, long* statusCode){
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << '\n';
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, statusCode);
        std::cout << "HTTP Status Code: " << *statusCode << '\n';
        std::cout << "Response Body:\n" << *response << '\n';
    }
    curl_easy_cleanup(curl);
}

int main() {
    uint16_t in = 0; uint16_t out = 0;

    net.opt.use_vulkan_compute = false;
    net.opt.num_threads = 4;

    std::string param = "models/best.ncnn.param";
    std::string bin = "models/best.ncnn.bin";

    std::string res; long code;
    bool capture = false;

    if (net.load_param(param.c_str()) != 0) {
        std::cerr << "Failed to load param\n";
        return -1;
    }

    if (net.load_model(bin.c_str()) != 0) {
        std::cerr << "Failed to load model\n";
        return -1;
    }

    cv::VideoCapture cap("assets/video.mp4", cv::CAP_FFMPEG);

    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open camera!" << std::endl;
        return -1;
    }

    cv::Mat frame;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL* curl = curl_easy_init();

    while (true) {
        objects.clear();
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Empty frame!" << std::endl;
            if(!capture){
                json data = json::array();
                in = 30; out = 10;
                netP -= (in - out);
                if(netP < 0) netP = 0;
                if(netP > seats) netP = seats;       
                data.push_back(stopNum);
                data.push_back(lat);
                data.push_back(lng);
                data.push_back(in);
                data.push_back(out);
                data.push_back(netP);
                getResponse(curl, (std::string("http://127.0.0.1:8080/update") + std::string("?data=") + std::string(data.dump())).c_str(), &res, &code);
                std::cout << res << ", status: " << code << std::endl;
                in = 0;
                out = 0;
                capture = true;
            }
            break;
        }

        cv::Size shape = frame.size();
        cv::Point p1(shape.width * 0.1, shape.height * 0.6), p2(shape.width * 0.9, shape.height * 0.6);

        cv::Mat input_image = letterbox(frame);
        ncnn::Mat in0 = ncnn::Mat::from_pixels(input_image.data, ncnn::Mat::PIXEL_BGR2RGB, 640, 640);
        in0.substract_mean_normalize(mean_vals, norm_vals);

        ncnn::Extractor ex = net.create_extractor();
        if (ex.input("in0", in0) != 0) {
            std::cerr << "Input failed\n";
            return -1;
        }

        if (ex.extract("out0", out0) != 0) {
            std::cerr << "Output extraction failed\n";
            return -1;
        }

        std::vector<Detection> detections = decode(out0, frame.size(), 0.35f, 0.45f);
        objects.reserve(detections.size());

        for (const auto& det : detections) {
            byte_track::Rect<float> rect(det.box.x, det.box.y, det.box.width, det.box.height);
            objects.emplace_back(rect, 0, det.confidence);
            cv::rectangle(frame, det.box, cv::Scalar(0, 255, 0), 2);
        }

        auto tracks = tracker.update(objects);
        for (const auto& track : tracks) {

            byte_track::Rect rect = track->getRect();

            float x = rect.x() + rect.width()/2;
            float y = rect.y() + rect.height()/2;

            uint8_t side = getPointSide(cv::Point2f(x, y), p1, p2);

            if(trackerId.find(track->getTrackId()) == trackerId.end()) {
                trackerId[track->getTrackId()] = side;
            }
            else{
                if(side != trackerId[track->getTrackId()]){
                    if(side == 1){
                        out += 1;
                    }
                    else{
                        in += 1;
                    }
                    trackerId[track->getTrackId()] = side;
                }
            }
        }

        cv::line(frame, p1, p2, cv::Scalar(255, 0, 0), 3, cv::LINE_AA);
        cv::imshow("Webcam", frame);
        if (cv::waitKey(1) == 27) {
            break;
        }
    }

    curl_global_cleanup();
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
#define NOMINMAX

#include <vector>
#include <ncnn/Mat.h>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

struct Detection {
    cv::Rect2f box;
    float confidence;
    int class_id;
};

extern std::vector<Detection> decode(const ncnn::Mat& out0, const cv::Size& original_size, float conf_threshold = 0.35f,float nms_threshold = 0.45f);
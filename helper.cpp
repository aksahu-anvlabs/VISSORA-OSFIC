#include "global.hpp"

#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <ncnn/Mat.h>

std::vector<Detection> decode(const ncnn::Mat& out0, const cv::Size& original_size, float conf_threshold, float nms_threshold) {
    std::vector<Detection> detections;

    if (out0.dims != 2 || out0.w != 8400 || out0.h != 5)
        return detections;

    int original_w = original_size.width;
    int original_h = original_size.height;

    float scale = std::min(640.0f / original_w, 640.0f / original_h);

    int resized_w = static_cast<int>(std::round(original_w * scale));
    int resized_h = static_cast<int>(std::round(original_h * scale));

    int pad_x = (640 - resized_w) / 2;
    int pad_y = (640 - resized_h) / 2;

    const float* cx_data   = out0.row(0);
    const float* cy_data   = out0.row(1);
    const float* w_data    = out0.row(2);
    const float* h_data    = out0.row(3);
    const float* conf_data = out0.row(4);

    std::vector<cv::Rect2d> boxes;
    std::vector<float> scores;

    for (int i = 0; i < 8400; i++) {

        float confidence = conf_data[i];

        if (confidence < conf_threshold)
            continue;

        float cx = cx_data[i];
        float cy = cy_data[i];
        float bw = w_data[i];
        float bh = h_data[i];

        // Center XYWH -> XYXY in 640x640 space
        float x1 = cx - bw * 0.5f;
        float y1 = cy - bh * 0.5f;
        float x2 = cx + bw * 0.5f;
        float y2 = cy + bh * 0.5f;

        // Reverse letterbox padding and scaling
        x1 = (x1 - pad_x) / scale;
        y1 = (y1 - pad_y) / scale;
        x2 = (x2 - pad_x) / scale;
        y2 = (y2 - pad_y) / scale;

        // Clamp to original frame boundaries
        x1 = std::clamp(x1, 0.0f, (float)original_w);
        y1 = std::clamp(y1, 0.0f, (float)original_h);
        x2 = std::clamp(x2, 0.0f, (float)original_w);
        y2 = std::clamp(y2, 0.0f, (float)original_h);

        float box_w = x2 - x1;
        float box_h = y2 - y1;

        if (box_w <= 0 || box_h <= 0)
            continue;

        boxes.emplace_back(x1, y1, box_w, box_h);
        scores.push_back(confidence);
    }

    std::vector<int> indices;

    cv::dnn::NMSBoxes(boxes, scores,conf_threshold, nms_threshold, indices);
    for (int idx : indices) {
        detections.push_back({cv::Rect2f(boxes[idx]), scores[idx], 0});
    }

    return detections;
}
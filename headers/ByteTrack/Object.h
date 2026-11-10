#pragma once

#include "ByteTrack/Rect.h"

#ifdef BUILDING_BYTETRACK_DLL
    #define BYTETRACK_API __declspec(dllexport)
#else
    #define BYTETRACK_API __declspec(dllimport)
#endif

namespace byte_track
{
struct BYTETRACK_API Object
{
    Rect<float> rect;
    int label;
    float prob;

    Object(const Rect<float> &_rect,
           const int &_label,
           const float &_prob);
};
}
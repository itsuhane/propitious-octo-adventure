#pragma once

#include <opencv2/opencv.hpp>
#include <assert.h>

/*
This piece of code implements the method described in following paper:
    Fr¨¦d¨¦rique Cr¨¦t¨¦-Roffet, Thierry Dolmiere, Patricia Ladret, Marina Nicolas. The Blur Effect: Perception and Estimation with a New No-Reference Perceptual Blur Metric.
    SPIE Electronic Imaging Symposium Conf Human Vision and Electronic Imaging, Jan 2007, San Jose, United States. XII, pp.EI 6492-16, 2007.
    <hal-00232709>
*/

float blurriness(const cv::Mat &grayimage) {
    using namespace cv;
    assert(grayimage.channels() == 1);

    Mat F;
    grayimage.convertTo(F, CV_32F, 1.0 / 255.0);

    int w = F.cols;
    int h = F.rows;

    if (w > 1024 || h > 1024) {
        if (w > h) {
            resize(F, F, cv::Size(1024, h * 1024 / w), 0, 0, cv::INTER_AREA);
        }
        else {
            resize(F, F, cv::Size(w * 1024 / h, 1024), 0, 0, cv::INTER_AREA);
        }
    }

    w = F.cols;
    h = F.rows;

    Mat B_Ver, B_Hor;
    boxFilter(F, B_Ver, -1, Size(9, 1));
    boxFilter(F, B_Hor, -1, Size(1, 9));


    Mat D_F_Ver, D_F_Hor, D_B_Ver, D_B_Hor, D_V_Ver, D_V_Hor;
    absdiff(F(Rect(1, 0, w - 1, h)), F(Rect(0, 0, w - 1, h)), D_F_Ver);
    absdiff(F(Rect(0, 1, w, h - 1)), F(Rect(0, 0, w, h - 1)), D_F_Hor);
    absdiff(B_Ver(Rect(1, 0, w - 1, h)), B_Ver(Rect(0, 0, w - 1, h)), D_B_Ver);
    absdiff(B_Hor(Rect(0, 1, w, h - 1)), B_Hor(Rect(0, 0, w, h - 1)), D_B_Hor);

    Mat V_Ver, V_Hor;
    cv::threshold(D_F_Ver - D_B_Ver, V_Ver, 0, 0, THRESH_TOZERO);
    cv::threshold(D_F_Hor - D_B_Hor, V_Hor, 0, 0, THRESH_TOZERO);

    absdiff(V_Ver(Rect(1, 0, w - 2, h)), V_Ver(Rect(0, 0, w - 2, h)), D_V_Ver);
    absdiff(V_Hor(Rect(0, 1, w, h - 2)), V_Hor(Rect(0, 0, w, h - 2)), D_V_Hor);

    float s_F_Ver = cv::sum(D_F_Ver)(0);
    float s_F_Hor = cv::sum(D_F_Hor)(0);
    float s_V_Ver = cv::sum(D_V_Ver)(0);
    float s_V_Hor = cv::sum(D_V_Hor)(0);

    float b_F_Ver = (s_F_Ver - s_V_Ver) / s_F_Ver;
    float b_F_Hor = (s_F_Hor - s_V_Hor) / s_F_Hor;

    float blur_F = max(b_F_Ver, b_F_Hor);

    return blur_F;
}

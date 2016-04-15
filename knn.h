#pragma once

// A simple knn with cv::flann.
// Its document is too obsecured so I have to note this down.

#include <array>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <opencv2/flann.hpp>

template <typename T /* = float */, size_t Dimension = 3>
class KnnIndex {
public:
    typedef T value_type;
    typedef std::array<value_type, Dimension> point_type;
    static const size_t dimension = Dimension;

    KnnIndex(const std::vector<point_type> &points, bool bruteforce = false) {
        cv::Mat data(points.size(), dimension, cv::DataType<value_type>::type, (void*)points[0].data());
        if (bruteforce) {
            flannIndex = std::make_unique<cv::flann::GenericIndex<cvflann::L2<value_type>>>(data, cvflann::LinearIndexParams());
        }
        else {
            flannIndex = std::make_unique<cv::flann::GenericIndex<cvflann::L2<value_type>>>(data, cvflann::AutotunedIndexParams(0.95));
        }
    }

    std::vector<int> knn(const point_type &point, int k) {
        std::vector<int> indices(k);
        std::vector<value_type> distances(k);
        std::vector<value_type> vpoint(point.begin(), point.end());
        flannIndex->knnSearch(vpoint, indices, distances, k, cvflann::SearchParams());
        return indices;
    }

private:
    std::unique_ptr<cv::flann::GenericIndex<cvflann::L2<value_type>>> flannIndex;
};
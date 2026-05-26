#include <pyqtgraph/Point.hpp>
#include <pyqtgraph/Vector.hpp>
#include <pyqtgraph/functions.hpp>

#include <opencv2/core.hpp>

#include <cassert>
#include <cmath>

int main()
{
    pyqtgraph::Point point(3.0, 4.0);
    assert(std::abs(point.length() - 5.0) < 1e-12);

    pyqtgraph::Vector vector(1.0, 2.0, 3.0);
    assert(std::abs(vector.length() - std::sqrt(14.0f)) < 1e-6);

    const QColor color = pyqtgraph::mkColor("r");
    assert(color.isValid());
    assert(color.red() == 255);

    cv::Mat image(2, 3, CV_8UC3, cv::Scalar(1, 2, 3));
    assert(image.rows == 2);
    assert(image.cols == 3);
    const cv::Vec3b pixel = image.at<cv::Vec3b>(0, 0);
    assert(pixel[0] == 1);
    assert(pixel[1] == 2);
    assert(pixel[2] == 3);

    return 0;
}

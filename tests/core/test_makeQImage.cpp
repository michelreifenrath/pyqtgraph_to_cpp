#include "../../include/pyqtgraph/functions_qimage.hpp"

#include <QColor>
#include <QImage>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

bool check(bool condition, std::string_view expression, std::string_view file, int line)
{
    if (!condition) {
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
        return false;
    }
    return true;
}

#define CHECK(expression) \
    do { \
        if (!check((expression), #expression, __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (false)

bool checkPixel(const QImage& image, int x, int y, int red, int green, int blue, int alpha, std::string_view label)
{
    const QColor color = image.pixelColor(x, y);
    if (color.red() != red || color.green() != green || color.blue() != blue || color.alpha() != alpha) {
        std::cerr << label << " at (" << x << ", " << y << "): expected rgba(" << red << ", " << green << ", "
                  << blue << ", " << alpha << ") got rgba(" << color.red() << ", " << color.green() << ", "
                  << color.blue() << ", " << color.alpha() << ")\n";
        return false;
    }
    return true;
}

#define CHECK_PIXEL(image, x, y, red, green, blue, alpha) \
    do { \
        if (!checkPixel((image), (x), (y), (red), (green), (blue), (alpha), #image)) { \
            return false; \
        } \
    } while (false)

template <typename Callable>
bool checkThrowsInvalidArgument(Callable callable, std::string_view label)
{
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception& error) {
        std::cerr << label << ": expected std::invalid_argument, got " << error.what() << '\n';
        return false;
    }

    std::cerr << label << ": expected std::invalid_argument\n";
    return false;
}

bool testGrayscaleTransposeDefault()
{
    const std::array<std::uint8_t, 6> data{10, 20, 30, 40, 50, 60};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3});

    const QImage image = pyqtgraph::makeQImage(view);
    CHECK(image.width() == 2);
    CHECK(image.height() == 3);
    CHECK(image.format() == QImage::Format_Grayscale8);
    CHECK_PIXEL(image, 0, 0, 10, 10, 10, 255);
    CHECK_PIXEL(image, 1, 0, 40, 40, 40, 255);
    CHECK_PIXEL(image, 0, 2, 30, 30, 30, 255);
    CHECK_PIXEL(image, 1, 2, 60, 60, 60, 255);
    return true;
}

bool testBgrOrderMatchesPyQtGraph()
{
    const std::array<std::uint8_t, 12> data{1, 2, 3, 10, 20, 30, 0, 128, 255, 7, 8, 9};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {2, 2, 3});

    const QImage image = pyqtgraph::makeQImage(view);
    CHECK(image.width() == 2);
    CHECK(image.height() == 2);
    CHECK(image.format() == QImage::Format_RGB32);
    CHECK_PIXEL(image, 0, 0, 3, 2, 1, 255);
    CHECK_PIXEL(image, 0, 1, 30, 20, 10, 255);
    CHECK_PIXEL(image, 1, 0, 255, 128, 0, 255);
    CHECK_PIXEL(image, 1, 1, 9, 8, 7, 255);
    return true;
}

bool testBgraPreservesAlpha()
{
    const std::array<std::uint8_t, 16> data{1, 2, 3, 0, 10, 20, 30, 128, 0, 128, 255, 255, 7, 8, 9, 64};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {2, 2, 4});

    const QImage image = pyqtgraph::makeQImage(view);
    CHECK(image.width() == 2);
    CHECK(image.height() == 2);
    CHECK(image.format() == QImage::Format_ARGB32);
    CHECK_PIXEL(image, 0, 0, 3, 2, 1, 0);
    CHECK_PIXEL(image, 0, 1, 30, 20, 10, 128);
    CHECK_PIXEL(image, 1, 0, 255, 128, 0, 255);
    CHECK_PIXEL(image, 1, 1, 9, 8, 7, 64);
    return true;
}

bool testAlphaFalseDropsBgraAlpha()
{
    const std::array<std::uint8_t, 4> data{1, 2, 3, 4};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {1, 1, 4});
    pyqtgraph::MakeQImageOptions options;
    options.alpha = false;

    const QImage image = pyqtgraph::makeQImage(view, options);
    CHECK(image.width() == 1);
    CHECK(image.height() == 1);
    CHECK(image.format() == QImage::Format_RGB32);
    CHECK_PIXEL(image, 0, 0, 3, 2, 1, 255);
    return true;
}

bool testAlphaTrueAddsOpaqueAlphaForBgr()
{
    const std::array<std::uint8_t, 3> data{1, 2, 3};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {1, 1, 3});
    pyqtgraph::MakeQImageOptions options;
    options.alpha = true;

    const QImage image = pyqtgraph::makeQImage(view, options);
    CHECK(image.width() == 1);
    CHECK(image.height() == 1);
    CHECK(image.format() == QImage::Format_ARGB32);
    CHECK_PIXEL(image, 0, 0, 3, 2, 1, 255);
    return true;
}

bool testTransposeFalse()
{
    const std::array<std::uint8_t, 6> data{10, 20, 30, 40, 50, 60};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3});
    pyqtgraph::MakeQImageOptions options;
    options.transpose = false;

    const QImage image = pyqtgraph::makeQImage(view, options);
    CHECK(image.width() == 3);
    CHECK(image.height() == 2);
    CHECK_PIXEL(image, 0, 0, 10, 10, 10, 255);
    CHECK_PIXEL(image, 2, 0, 30, 30, 30, 255);
    CHECK_PIXEL(image, 0, 1, 40, 40, 40, 255);
    CHECK_PIXEL(image, 2, 1, 60, 60, 60, 255);
    return true;
}

bool testStridedInputCopiesElements()
{
    const std::array<std::uint8_t, 12> data{1, 99, 2, 99, 3, 99, 4, 99, 5, 99, 6, 99};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3}, {6, 2});
    pyqtgraph::MakeQImageOptions options;
    options.transpose = false;

    QImage image = pyqtgraph::makeQImage(view, options);
    CHECK(image.width() == 3);
    CHECK(image.height() == 2);
    CHECK_PIXEL(image, 0, 0, 1, 1, 1, 255);
    CHECK_PIXEL(image, 2, 0, 3, 3, 3, 255);
    CHECK_PIXEL(image, 0, 1, 4, 4, 4, 255);
    CHECK_PIXEL(image, 2, 1, 6, 6, 6, 255);

    return true;
}

bool testInvalidInputs()
{
    const std::array<std::uint8_t, 20> data{};
    CHECK(checkThrowsInvalidArgument(
        [&] { (void)pyqtgraph::makeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3>(data.data(), {2, 2, 1})); },
        "one channel rank-3 input"));
    CHECK(checkThrowsInvalidArgument(
        [&] { (void)pyqtgraph::makeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3>(data.data(), {2, 2, 2})); },
        "two channels"));
    CHECK(checkThrowsInvalidArgument(
        [&] { (void)pyqtgraph::makeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3>(data.data(), {2, 2, 5})); },
        "five channels"));
    CHECK(checkThrowsInvalidArgument(
        [] { (void)pyqtgraph::makeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 2>(nullptr, {1, 1})); },
        "null data"));
    CHECK(checkThrowsInvalidArgument(
        [&] { (void)pyqtgraph::makeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 2>(data.data(), {0, 1})); },
        "zero first extent"));
    CHECK(checkThrowsInvalidArgument(
        [&] { (void)pyqtgraph::makeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 2>(data.data(), {1, 0})); },
        "zero second extent"));
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testGrayscaleTransposeDefault() && success;
    success = testBgrOrderMatchesPyQtGraph() && success;
    success = testBgraPreservesAlpha() && success;
    success = testAlphaFalseDropsBgraAlpha() && success;
    success = testAlphaTrueAddsOpaqueAlphaForBgr() && success;
    success = testTransposeFalse() && success;
    success = testStridedInputCopiesElements() && success;
    success = testInvalidInputs() && success;

    return success ? 0 : 1;
}

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <span>
#include <string>

#define PYQTGRAPH_CPP_SIMPLEPLOT_NO_MAIN
#include "../../examples/SimplePlot.cpp"

namespace {

struct Options {
    QString example;
    QString output;
    int width = 800;
    int height = 600;
};

void printUsage(const char* program)
{
    std::cerr << "usage: " << program << " SimplePlot --output PATH [--width N] [--height N]\n";
}

bool parsePositiveInt(const std::string& text, int& value)
{
    try {
        std::size_t consumed = 0;
        const int parsed = std::stoi(text, &consumed);
        if (consumed != text.size() || parsed <= 0) {
            return false;
        }
        value = parsed;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parseOptions(int argc, char** argv, Options& options)
{
    if (argc < 4) {
        printUsage(argv[0]);
        return false;
    }

    options.example = QString::fromLocal8Bit(argv[1]);
    if (options.example != QStringLiteral("SimplePlot")) {
        std::cerr << "error: unsupported example: " << argv[1] << "\n";
        return false;
    }

    for (int index = 2; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--output") {
            if (++index >= argc) {
                std::cerr << "error: --output requires a path\n";
                return false;
            }
            options.output = QString::fromLocal8Bit(argv[index]);
        } else if (argument == "--width") {
            if (++index >= argc || !parsePositiveInt(argv[index], options.width)) {
                std::cerr << "error: --width must be a positive integer\n";
                return false;
            }
        } else if (argument == "--height") {
            if (++index >= argc || !parsePositiveInt(argv[index], options.height)) {
                std::cerr << "error: --height must be a positive integer\n";
                return false;
            }
        } else {
            std::cerr << "error: unknown argument: " << argument << "\n";
            return false;
        }
    }

    if (options.output.isEmpty()) {
        std::cerr << "error: --output is required\n";
        return false;
    }
    return true;
}

struct Bounds {
    double minimum;
    double maximum;
};

Bounds dataBounds(std::span<const double> values)
{
    Bounds bounds{0.0, 1.0};
    bool initialized = false;
    for (const double value : values) {
        if (!std::isfinite(value)) {
            continue;
        }
        if (!initialized) {
            bounds = Bounds{value, value};
            initialized = true;
            continue;
        }
        bounds.minimum = std::min(bounds.minimum, value);
        bounds.maximum = std::max(bounds.maximum, value);
    }
    if (!initialized) {
        return bounds;
    }
    if (bounds.minimum == bounds.maximum) {
        bounds.minimum -= 0.5;
        bounds.maximum += 0.5;
    }
    return bounds;
}

QPointF mapPoint(double x, double y, const Bounds& xBounds, const Bounds& yBounds, const QRectF& plotRect)
{
    const double xRatio = (x - xBounds.minimum) / (xBounds.maximum - xBounds.minimum);
    const double yRatio = (y - yBounds.minimum) / (yBounds.maximum - yBounds.minimum);
    return QPointF(plotRect.left() + xRatio * plotRect.width(),
                   plotRect.bottom() - yRatio * plotRect.height());
}

QString tickLabel(double value)
{
    if (std::abs(value) < 1.0e-9) {
        return QStringLiteral("0");
    }
    return QString::number(value, 'g', 3);
}

void drawTicks(QPainter& painter, const QRectF& plotRect, const Bounds& xBounds, const Bounds& yBounds,
               double axisLeft, double axisBottom)
{
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
    painter.setPen(QPen(QColor(150, 150, 150), 1));

    for (int tick = 0; tick <= 50; ++tick) {
        const double value = tick * 2.0;
        const double x = mapPoint(value, yBounds.minimum, xBounds, yBounds, plotRect).x();
        const double length = tick % 10 == 0 ? 7.0 : 4.0;
        painter.drawLine(QPointF(x, axisBottom), QPointF(x, axisBottom + length));
        if (tick % 10 == 0) {
            painter.drawText(QRectF(x - 22.0, axisBottom + 8.0, 44.0, 18.0), Qt::AlignCenter, tickLabel(value));
        }
    }

    const double firstYTick = std::ceil(yBounds.minimum * 10.0) / 10.0;
    const double lastYTick = std::floor(yBounds.maximum * 10.0) / 10.0;
    for (int step = static_cast<int>(std::round(firstYTick * 10.0)); step <= static_cast<int>(std::round(lastYTick * 10.0)); ++step) {
        const double value = step / 10.0;
        const double y = mapPoint(xBounds.minimum, value, xBounds, yBounds, plotRect).y();
        const bool major = step % 5 == 0;
        painter.drawLine(QPointF(axisLeft - (major ? 7.0 : 4.0), y), QPointF(axisLeft, y));
        if (major) {
            painter.drawText(QRectF(1.0, y - 9.0, axisLeft - 8.0, 18.0), Qt::AlignRight | Qt::AlignVCenter,
                             tickLabel(value));
        }
    }
}

bool renderSimplePlot(const Options& options)
{
    auto example = pyqtgraph::examples::createSimplePlotExample();
    const std::span<const double> xData = example.curve->xData();
    const std::span<const double> yData = example.curve->yData();
    if (xData.empty() || xData.size() != yData.size()) {
        std::cerr << "error: SimplePlot curve has no renderable data\n";
        return false;
    }

    QFileInfo outputInfo(options.output);
    if (!outputInfo.dir().exists() && !QDir().mkpath(outputInfo.dir().absolutePath())) {
        std::cerr << "error: failed to create output directory\n";
        return false;
    }

    QImage image(options.width, options.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0, 0, 0));

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const double scaleX = options.width / 800.0;
    const double scaleY = options.height / 600.0;
    const double axisLeft = 35.0 * scaleX;
    const double axisBottom = 580.0 * scaleY;
    const QRectF plotRect(62.0 * scaleX, 24.0 * scaleY, std::max(1.0, 710.0 * scaleX),
                          std::max(1.0, 532.0 * scaleY));
    const Bounds xBounds = dataBounds(xData);
    const Bounds yBounds = dataBounds(yData);

    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawLine(QPointF(axisLeft, 0.0), QPointF(axisLeft, axisBottom));
    painter.drawLine(QPointF(axisLeft, axisBottom), QPointF(options.width - 1.0, axisBottom));
    drawTicks(painter, plotRect, xBounds, yBounds, axisLeft, axisBottom);

    QPainterPath curvePath;
    curvePath.moveTo(mapPoint(xData[0], yData[0], xBounds, yBounds, plotRect));
    for (std::size_t index = 1; index < xData.size(); ++index) {
        curvePath.lineTo(mapPoint(xData[index], yData[index], xBounds, yBounds, plotRect));
    }
    painter.setPen(QPen(QColor(200, 200, 200), 1.0));
    painter.drawPath(curvePath);
    painter.end();

    if (!image.save(options.output, "PNG")) {
        std::cerr << "error: failed to write output PNG\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    Options options;
    if (!parseOptions(argc, argv, options)) {
        return 1;
    }

    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);

    if (!renderSimplePlot(options)) {
        return 1;
    }

    QJsonObject dimensions;
    dimensions.insert(QStringLiteral("width"), options.width);
    dimensions.insert(QStringLiteral("height"), options.height);

    QJsonObject status;
    status.insert(QStringLiteral("example"), options.example);
    status.insert(QStringLiteral("output"), QFileInfo(options.output).absoluteFilePath());
    status.insert(QStringLiteral("dimensions"), dimensions);
    status.insert(QStringLiteral("placeholder"), false);

    std::cout << QJsonDocument(status).toJson(QJsonDocument::Compact).constData() << '\n';
    return 0;
}

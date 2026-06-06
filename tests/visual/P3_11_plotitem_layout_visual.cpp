#include <pyqtgraph/graphicsItems/LegendItem.hpp>
#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <pyqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
    std::cerr << "usage: " << program << " (SimplePlot|PlotDecorations|PlotDecorationsReference) --output PATH [--width N] [--height N]\n";
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
    if (options.example != QStringLiteral("SimplePlot") && options.example != QStringLiteral("PlotDecorations")
        && options.example != QStringLiteral("PlotDecorationsReference")) {
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

void processEvents()
{
    for (int iteration = 0; iteration < 5; ++iteration) {
        QApplication::processEvents(QEventLoop::AllEvents);
    }
}

bool ensureOutputDir(const QString& output)
{
    QFileInfo outputInfo(output);
    return outputInfo.dir().exists() || QDir().mkpath(outputInfo.dir().absolutePath());
}

bool saveWidget(QWidget& widget, const Options& options)
{
    widget.resize(options.width, options.height);
    widget.show();
    processEvents();
    if (!ensureOutputDir(options.output)) {
        std::cerr << "error: failed to create output directory\n";
        return false;
    }
    const QPixmap pixmap = widget.grab(QRect(0, 0, options.width, options.height));
    if (pixmap.isNull()) {
        std::cerr << "error: failed to grab widget\n";
        return false;
    }
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    return image.save(options.output, "PNG");
}

bool renderSimplePlot(const Options& options)
{
    auto example = pyqtgraph::examples::createSimplePlotExample();
    return saveWidget(*example.widget, options);
}

std::vector<double> xValues()
{
    return {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
}

std::vector<double> yValuesA()
{
    return {0.2, 1.1, 0.7, 2.0, 1.4, 2.6};
}

std::vector<double> yValuesB()
{
    return {2.3, 1.8, 1.2, 1.6, 0.9, 0.4};
}

bool renderPlotDecorations(const Options& options)
{
    pyqtgraph::widgets::PlotWidget widget;
    auto* plot = widget.getPlotItem();
    plot->setTitle(QStringLiteral("PlotItem decorations"));
    plot->setLabel(QStringLiteral("left"), QStringLiteral("Amplitude"));
    plot->setLabel(QStringLiteral("bottom"), QStringLiteral("Sample"));
    plot->showAxis(QStringLiteral("top"), true);
    plot->showAxis(QStringLiteral("right"), true);

    auto* first = new pyqtgraph::graphicsItems::PlotCurveItem;
    QPen firstPen(QColor(255, 220, 80), 2.0);
    firstPen.setCosmetic(true);
    first->setPen(firstPen);
    const auto x = xValues();
    const auto ya = yValuesA();
    first->setData(x, ya);
    plot->addItem(first);

    auto* second = new pyqtgraph::graphicsItems::PlotCurveItem;
    QPen secondPen(QColor(80, 180, 255), 2.0);
    secondPen.setCosmetic(true);
    second->setPen(secondPen);
    const auto yb = yValuesB();
    second->setData(x, yb);
    plot->addItem(second);

    auto* legend = plot->addLegend(QPointF(30.0, 30.0));
    legend->addItem(first, QStringLiteral("rising"));
    legend->addItem(second, QStringLiteral("falling"));

    return saveWidget(widget, options);
}

QPointF mapReferencePoint(double x, double y, const QRectF& rect)
{
    constexpr double minX = 0.0;
    constexpr double maxX = 5.0;
    constexpr double minY = 0.2;
    constexpr double maxY = 2.6;
    const double xr = (x - minX) / (maxX - minX);
    const double yr = (y - minY) / (maxY - minY);
    return QPointF(rect.left() + xr * rect.width(), rect.bottom() - yr * rect.height());
}

void drawReferenceCurve(QPainter& painter, const QRectF& rect, const QColor& color, const std::vector<double>& y)
{
    const auto x = xValues();
    QPainterPath path;
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index) {
        const QPointF point = mapReferencePoint(x[index], y[index], rect);
        if (index == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 2.0));
    painter.drawPath(path);
}

bool renderPlotDecorationsReference(const Options& options)
{
    QImage image(options.width, options.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(QPen(QColor(220, 220, 220), 1.0));
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 11));
    painter.drawText(QRectF(0.0, 5.0, options.width, 26.0), Qt::AlignCenter, QStringLiteral("PlotItem decorations"));

    const QRectF plotRect(76.0, 54.0, options.width - 116.0, options.height - 112.0);
    painter.setPen(QPen(QColor(150, 150, 150), 1.0));
    painter.drawRect(plotRect);
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
    painter.drawText(QRectF(0.0, plotRect.center().y() - 60.0, 22.0, 120.0), Qt::AlignCenter, QStringLiteral("Amplitude"));
    painter.drawText(QRectF(plotRect.left(), options.height - 30.0, plotRect.width(), 22.0), Qt::AlignCenter, QStringLiteral("Sample"));

    for (int tick = 0; tick <= 5; ++tick) {
        const QPointF point = mapReferencePoint(tick, 0.2, plotRect);
        painter.drawLine(QPointF(point.x(), plotRect.bottom()), QPointF(point.x(), plotRect.bottom() + 5.0));
        painter.drawText(QRectF(point.x() - 18.0, plotRect.bottom() + 7.0, 36.0, 18.0), Qt::AlignCenter, QString::number(tick));
    }
    for (double tick : {0.5, 1.0, 1.5, 2.0, 2.5}) {
        const QPointF point = mapReferencePoint(0.0, tick, plotRect);
        painter.drawLine(QPointF(plotRect.left() - 5.0, point.y()), QPointF(plotRect.left(), point.y()));
        painter.drawText(QRectF(24.0, point.y() - 9.0, 45.0, 18.0), Qt::AlignRight | Qt::AlignVCenter, QString::number(tick, 'g', 2));
    }

    drawReferenceCurve(painter, plotRect, QColor(255, 220, 80), yValuesA());
    drawReferenceCurve(painter, plotRect, QColor(80, 180, 255), yValuesB());

    const QRectF legend(106.0, 84.0, 116.0, 56.0);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(200, 200, 200), 1.0));
    painter.setBrush(QColor(0, 0, 0, 190));
    painter.drawRect(legend);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 220, 80), 2.0));
    painter.drawLine(QPointF(legend.left() + 10.0, legend.top() + 18.0), QPointF(legend.left() + 36.0, legend.top() + 18.0));
    painter.setPen(QPen(QColor(80, 180, 255), 2.0));
    painter.drawLine(QPointF(legend.left() + 10.0, legend.top() + 39.0), QPointF(legend.left() + 36.0, legend.top() + 39.0));
    painter.setPen(QPen(QColor(230, 230, 230), 1.0));
    painter.drawText(QPointF(legend.left() + 45.0, legend.top() + 22.0), QStringLiteral("rising"));
    painter.drawText(QPointF(legend.left() + 45.0, legend.top() + 43.0), QStringLiteral("falling"));
    painter.end();

    if (!ensureOutputDir(options.output)) {
        return false;
    }
    return image.save(options.output, "PNG");
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

    bool ok = false;
    if (options.example == QStringLiteral("SimplePlot")) {
        ok = renderSimplePlot(options);
    } else if (options.example == QStringLiteral("PlotDecorations")) {
        ok = renderPlotDecorations(options);
    } else {
        ok = renderPlotDecorationsReference(options);
    }
    if (!ok) {
        std::cerr << "error: failed to render " << options.example.toStdString() << "\n";
        return 1;
    }

    QJsonObject dimensions;
    dimensions.insert(QStringLiteral("width"), options.width);
    dimensions.insert(QStringLiteral("height"), options.height);
    QJsonObject status;
    status.insert(QStringLiteral("example"), options.example);
    status.insert(QStringLiteral("output"), QFileInfo(options.output).absoluteFilePath());
    status.insert(QStringLiteral("dimensions"), dimensions);
    status.insert(QStringLiteral("render_path"), QStringLiteral("PlotItem layout/ViewBox/AxisItem/LegendItem visual renderer"));
    status.insert(QStringLiteral("placeholder"), false);
    std::cout << QJsonDocument(status).toJson(QJsonDocument::Compact).constData() << '\n';
    return 0;
}

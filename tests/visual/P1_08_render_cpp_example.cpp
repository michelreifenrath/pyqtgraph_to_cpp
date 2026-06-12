#include <cppqtgraph/imageview/ImageView.hpp>
#include <cppqtgraph/widgets/GraphicsView.hpp>

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <iostream>
#include <string>

#define CPPQTGRAPH_SIMPLEPLOT_NO_MAIN
#include "../../examples/SimplePlot.cpp"

#define CPPQTGRAPH_PLOTTING_NO_MAIN
#include "../../examples/Plotting.cpp"

#define CPPQTGRAPH_IMAGEVIEW_NO_MAIN
#include "../../examples/ImageView.cpp"

namespace {

struct Options {
    QString example;
    QString output;
    int width = 800;
    int height = 600;
};

void printUsage(const char* program)
{
    std::cerr << "usage: " << program << " <SimplePlot|Plotting|ImageView> --output PATH [--width N] [--height N]\n";
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
    if (options.example != QStringLiteral("SimplePlot") && options.example != QStringLiteral("Plotting")
        && options.example != QStringLiteral("ImageView")) {
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
    for (int iteration = 0; iteration < 3; ++iteration) {
        QApplication::processEvents(QEventLoop::AllEvents);
    }
}

bool renderSimplePlot(const Options& options)
{
    auto example = cppqtgraph::examples::createSimplePlotExample();
    example.widget->resize(options.width, options.height);
    example.widget->show();
    processEvents();

    QFileInfo outputInfo(options.output);
    if (!outputInfo.dir().exists() && !QDir().mkpath(outputInfo.dir().absolutePath())) {
        std::cerr << "error: failed to create output directory\n";
        return false;
    }

    const QPixmap pixmap = example.widget->grab(QRect(0, 0, options.width, options.height));
    if (pixmap.isNull()) {
        std::cerr << "error: failed to grab SimplePlot widget\n";
        return false;
    }

    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (!image.save(options.output, "PNG")) {
        std::cerr << "error: failed to write output PNG\n";
        return false;
    }
    return true;
}

bool renderImageView(const Options& options, QJsonObject* imageCrop)
{
    auto example = cppqtgraph::examples::createImageViewExample();
    example.window->resize(options.width, options.height);
    example.window->show();
    processEvents();

    QFileInfo outputInfo(options.output);
    if (!outputInfo.dir().exists() && !QDir().mkpath(outputInfo.dir().absolutePath())) {
        std::cerr << "error: failed to create output directory\n";
        return false;
    }

    const QPixmap pixmap = example.window->grab(QRect(0, 0, options.width, options.height));
    if (pixmap.isNull()) {
        std::cerr << "error: failed to grab ImageView widget\n";
        return false;
    }

    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (!image.save(options.output, "PNG")) {
        std::cerr << "error: failed to write output PNG\n";
        return false;
    }

    if (imageCrop != nullptr && example.imageView != nullptr && example.imageView->getView() != nullptr) {
        auto* graphicsView = example.imageView->getView();
        const QPoint topLeft = graphicsView->mapTo(example.window.get(), QPoint(0, 0));
        imageCrop->insert(QStringLiteral("x"), topLeft.x());
        imageCrop->insert(QStringLiteral("y"), topLeft.y());
        imageCrop->insert(QStringLiteral("width"), graphicsView->width());
        imageCrop->insert(QStringLiteral("height"), graphicsView->height());
    }
    return true;
}

bool renderPlotting(const Options& options)
{
    auto example = cppqtgraph::examples::createPlottingExample();
    example.widget->resize(options.width, options.height);
    example.widget->show();
    processEvents();

    QFileInfo outputInfo(options.output);
    if (!outputInfo.dir().exists() && !QDir().mkpath(outputInfo.dir().absolutePath())) {
        std::cerr << "error: failed to create output directory\n";
        return false;
    }

    const QPixmap pixmap = example.widget->grab(QRect(0, 0, options.width, options.height));
    if (pixmap.isNull()) {
        std::cerr << "error: failed to grab Plotting widget\n";
        return false;
    }

    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
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

    QJsonObject imageCrop;
    const bool rendered = options.example == QStringLiteral("SimplePlot")
                              ? renderSimplePlot(options)
                              : options.example == QStringLiteral("Plotting") ? renderPlotting(options)
                                                                              : renderImageView(options, &imageCrop);
    if (!rendered) {
        return 1;
    }

    QJsonObject dimensions;
    dimensions.insert(QStringLiteral("width"), options.width);
    dimensions.insert(QStringLiteral("height"), options.height);
    if (!imageCrop.isEmpty()) {
        dimensions.insert(QStringLiteral("image_crop"), imageCrop);
    }

    QJsonObject status;
    status.insert(QStringLiteral("example"), options.example);
    status.insert(QStringLiteral("output"), QFileInfo(options.output).absoluteFilePath());
    status.insert(QStringLiteral("dimensions"), dimensions);
    status.insert(QStringLiteral("render_path"), QStringLiteral("QWidget::grab"));
    status.insert(QStringLiteral("placeholder"), false);

    std::cout << QJsonDocument(status).toJson(QJsonDocument::Compact).constData() << '\n';
    return 0;
}

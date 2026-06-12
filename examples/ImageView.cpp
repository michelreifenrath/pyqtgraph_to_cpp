// Source note: translated/adapted from PyQtGraph examples/ImageView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/imageview/ImageView.hpp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <memory>

namespace cppqtgraph::examples {

struct ImageViewExample {
    std::unique_ptr<QMainWindow> window;
    imageview::ImageView* imageView = nullptr;
};

ImageViewExample createImageViewExample()
{
    auto window = std::make_unique<QMainWindow>();
    window->resize(800, 800);
    window->setWindowTitle(QStringLiteral("pyqtgraph example: ImageView"));

    auto* imageView = new imageview::ImageView(window.get());
    window->setCentralWidget(imageView);

    return {.window = std::move(window), .imageView = imageView};
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_IMAGEVIEW_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    auto example = cppqtgraph::examples::createImageViewExample();
    example.window->show();
    return QApplication::exec();
}
#endif

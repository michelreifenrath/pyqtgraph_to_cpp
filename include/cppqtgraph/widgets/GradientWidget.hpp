#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GradientWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/graphicsItems/GradientEditorItem.hpp>
#include <cppqtgraph/widgets/GraphicsView.hpp>

#include <QtCore/QString>

namespace cppqtgraph::widgets {

class GradientWidget final : public GraphicsView {
    Q_OBJECT

public:
    explicit GradientWidget(QWidget* parent = nullptr, const QString& orientation = QStringLiteral("bottom"));

    [[nodiscard]] graphicsItems::GradientEditorItem* item() const noexcept { return item_; }
    [[nodiscard]] QString orientation() const noexcept { return orientation_; }
    [[nodiscard]] int maxDim() const noexcept { return maxDim_; }

    void setOrientation(const QString& orientation);
    void setMaxDim(int maxDim = -1);

    [[nodiscard]] cppqtgraph::ColorMap colorMap() const;
    void setColorMap(const cppqtgraph::ColorMap& colorMap);
    [[nodiscard]] graphicsItems::GradientEditorState saveState() const;
    void restoreState(const graphicsItems::GradientEditorState& state);
    void setLength(qreal length);

signals:
    void sigGradientChanged(graphicsItems::GradientEditorItem* item);
    void sigGradientChangeFinished(graphicsItems::GradientEditorItem* item);

private:
    void installEditor(graphicsItems::GradientEditorItem* editor);
    void applyOrientationSizing();
    void updateViewRange();

    graphicsItems::GradientEditorItem* item_ = nullptr;
    QString orientation_{QStringLiteral("bottom")};
    int maxDim_ = 31;
};

} // namespace cppqtgraph::widgets

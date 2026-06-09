#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/VerticalLabel.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtWidgets/QLabel>

class QPaintEvent;

namespace pyqtgraph::widgets {

class VerticalLabel : public QLabel {
    Q_OBJECT

public:
    explicit VerticalLabel(const QString& text = QString(),
        const QString& orientation = QStringLiteral("vertical"),
        bool forceWidth = true);

    VerticalLabel(const VerticalLabel&) = delete;
    VerticalLabel& operator=(const VerticalLabel&) = delete;
    VerticalLabel(VerticalLabel&&) = delete;
    VerticalLabel& operator=(VerticalLabel&&) = delete;

    void setOrientation(const QString& orientation);
    [[nodiscard]] QString orientation() const { return orientation_; }
    [[nodiscard]] bool forceWidth() const { return forceWidth_; }
    [[nodiscard]] bool hasTextHint() const { return hasTextHint_; }
    [[nodiscard]] QRect textHint() const { return textHint_; }

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString orientation_ = QStringLiteral("vertical");
    bool forceWidth_ = true;
    bool hasTextHint_ = false;
    QRect textHint_;
};

} // namespace pyqtgraph::widgets

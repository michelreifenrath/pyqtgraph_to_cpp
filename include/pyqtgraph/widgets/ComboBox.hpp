#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ComboBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtWidgets/QComboBox>

namespace pyqtgraph::widgets {

class ComboBox : public QComboBox {
    Q_OBJECT

public:
    explicit ComboBox(QWidget* parent = nullptr, const QVariant& items = QVariant(), const QVariant& defaultValue = QVariant());

    ComboBox(const ComboBox&) = delete;
    ComboBox& operator=(const ComboBox&) = delete;
    ComboBox(ComboBox&&) = delete;
    ComboBox& operator=(ComboBox&&) = delete;

    [[nodiscard]] QVariant value() const;
    void setValue(const QVariant& value);
    void setText(const QString& text);

    void setItems(const QVariant& items);
    void updateList(const QVariant& items) { setItems(items); }

    [[nodiscard]] QMap<QString, QVariant> items() const { return items_; }

    [[nodiscard]] QVariant saveState() const;
    void restoreState(const QVariant& state);

    void addItem(const QString& text, const QVariant& value = QVariant());
    void addItems(const QVariant& items);

    void clear();

    void insertItem(int index, const QString& text, const QVariant& value = QVariant());
    void insertItems(int index, const QStringList& texts);

    void setItemValue(const QString& name, const QVariant& value);

private:
    class IgnoreIndexChangeGuard {
    public:
        explicit IgnoreIndexChangeGuard(ComboBox& box);
        ~IgnoreIndexChangeGuard();

        IgnoreIndexChangeGuard(const IgnoreIndexChangeGuard&) = delete;
        IgnoreIndexChangeGuard& operator=(const IgnoreIndexChangeGuard&) = delete;

    private:
        ComboBox& box_;
        bool previous_;
    };

    template <typename Fn>
    auto withBlockedSignalsIfUnchanged(Fn&& fn) -> decltype(fn())
    {
        const QVariant previousValue = value();
        const bool wasBlocked = signalsBlocked();
        blockSignals(true);
        decltype(fn()) result{};
        try {
            result = fn();
        } catch (...) {
            blockSignals(wasBlocked);
            throw;
        }
        blockSignals(wasBlocked);
        if (value() != previousValue) {
            QComboBox::currentIndexChanged(currentIndex());
        }
        return result;
    }

    void indexChanged(int index);
    void itemsChanged();

    bool ignoreIndexChange_ = false;
    bool hasChosenText_ = false;
    QString chosenText_;
    QMap<QString, QVariant> items_;
};

} // namespace pyqtgraph::widgets

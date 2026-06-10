#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/WidgetGroup.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include <optional>
#include <vector>

namespace cppqtgraph {

// State manager for supported Qt Widgets, mirroring PyQtGraph WidgetGroup's
// built-in widget handling. The Python custom widgetGroupInterface hook is not
// ported in this C++ shard. QLineEdit follows upstream signal behavior: setText
// restores the widget, while the cached state refreshes on editingFinished() or
// an explicit readWidget().
class WidgetGroup : public QObject {
    Q_OBJECT

public:
    explicit WidgetGroup(QObject* parent = nullptr);
    ~WidgetGroup() override;

    WidgetGroup(const WidgetGroup&) = delete;
    WidgetGroup& operator=(const WidgetGroup&) = delete;
    WidgetGroup(WidgetGroup&&) = delete;
    WidgetGroup& operator=(WidgetGroup&&) = delete;

    void addWidget(QWidget* widget, const QString& name = QString(), std::optional<double> scale = std::nullopt);
    void autoAdd(QObject* object);
    [[nodiscard]] bool acceptsType(const QObject* object) const;
    [[nodiscard]] QWidget* findWidget(const QString& name) const;

    QVariantMap state();
    void setState(const QVariantMap& state);
    QVariant readWidget(QWidget* widget);
    void setWidget(QWidget* widget, const QVariant& value);
    void setScale(QWidget* widget, std::optional<double> scale);

signals:
    void sigChanged(QString name, QVariant value);

private slots:
    void widgetChanged();

private:
    struct Entry {
        QPointer<QWidget> widget;
        QString name;
        std::optional<double> scale;
        bool uncached = false;
        QMetaObject::Connection connection;
    };

    [[nodiscard]] Entry* findEntry(QWidget* widget);
    [[nodiscard]] const Entry* findEntry(QWidget* widget) const;
    [[nodiscard]] bool checkForChildren(const QObject* object) const;
    [[nodiscard]] bool connectChangeSignal(QWidget* widget, QMetaObject::Connection& connection);
    [[nodiscard]] QVariant valueForWidget(QWidget* widget) const;
    void restoreSplitter(QSplitter* splitter, const QVariant& value) const;
    [[nodiscard]] QVariant comboState(QComboBox* combo) const;
    void setComboState(QComboBox* combo, const QVariant& value) const;

    std::vector<Entry> entries_;
    QVariantMap cache_;
};

} // namespace cppqtgraph

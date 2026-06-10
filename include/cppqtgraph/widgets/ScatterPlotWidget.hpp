#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ScatterPlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "ColorMapWidget.hpp"

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace cppqtgraph::graphicsItems {
class ScatterPlotItem;
}

namespace cppqtgraph::widgets {

class PlotWidget;

struct ScatterPlotFieldOptions final {
    QString mode{QStringLiteral("range")};
    QString units;
    QVector<QVariant> values;
};

using ScatterPlotRecord = QMap<QString, QVariant>;
using ScatterPlotRecordArray = QVector<ScatterPlotRecord>;

class ScatterPlotWidget final : public QSplitter {
    Q_OBJECT

public:
    explicit ScatterPlotWidget(QWidget* parent = nullptr);
    ~ScatterPlotWidget() override;

    ScatterPlotWidget(const ScatterPlotWidget&) = delete;
    ScatterPlotWidget& operator=(const ScatterPlotWidget&) = delete;
    ScatterPlotWidget(ScatterPlotWidget&&) = delete;
    ScatterPlotWidget& operator=(ScatterPlotWidget&&) = delete;

    void setFields(const QVector<QPair<QString, ScatterPlotFieldOptions>>& fields,
        const QString& mouseOverField = {});
    void setSelectedFields(const QStringList& fields);
    void setData(const ScatterPlotRecordArray& data);
    void setSelectedIndices(const QVector<int>& indices);
    void setFilterMask(const QVector<bool>& mask);

    [[nodiscard]] QListWidget* fieldList() const noexcept { return fieldList_; }
    [[nodiscard]] PlotWidget* plotWidget() const noexcept { return plot_; }
    [[nodiscard]] ColorMapWidget* colorMapWidget() const noexcept { return colorMap_; }

    [[nodiscard]] const ScatterPlotRecordArray& data() const noexcept { return data_; }
    [[nodiscard]] QVector<int> originalIndices() const noexcept { return indices_; }
    [[nodiscard]] QVector<int> visibleIndices() const noexcept { return visibleIndices_; }
    [[nodiscard]] ScatterPlotRecordArray visibleData() const noexcept { return visibleData_; }
    [[nodiscard]] bool hasVisiblePlot() const noexcept { return scatterPlot_ != nullptr; }

    void emitPointClicked(int visibleIndex);
    void emitPointHovered(const QVector<int>& visibleIndices);

signals:
    void sigScatterPlotClicked(ScatterPlotWidget* widget, const QVariantList& points, const QVariant& event);
    void sigScatterPlotHovered(ScatterPlotWidget* widget, const QVariantList& points, const QVariant& event);

private slots:
    void fieldSelectionChanged();
    void colorMapChanged(ColorMapWidget* widget);

private:
    void updatePlot();
    void updateSelected();
    void clearPlotItems();
    [[nodiscard]] QHash<int, int> indexMap() const;
    [[nodiscard]] double fieldValueAsDouble(const ScatterPlotRecord& record, const QString& fieldName) const;
    [[nodiscard]] bool passesFilter(const ScatterPlotRecord& record, int originalIndex) const;

    QSplitter* ctrlPanel_ = nullptr;
    QListWidget* fieldList_ = nullptr;
    ColorMapWidget* colorMap_ = nullptr;
    PlotWidget* plot_ = nullptr;

    QVector<QPair<QString, ScatterPlotFieldOptions>> fields_;
    QString mouseOverField_;
    ScatterPlotRecordArray data_;
    QVector<int> indices_;
    QVector<bool> filterMask_;
    QVector<int> selectedIndices_;

    ScatterPlotRecordArray visibleData_;
    QVector<int> visibleIndices_;
    std::vector<double> visibleX_;
    std::vector<double> visibleY_;
    mutable std::optional<QHash<int, int>> indexMapCache_;

    graphicsItems::ScatterPlotItem* scatterPlot_ = nullptr;
    graphicsItems::ScatterPlotItem* selectionScatter_ = nullptr;
    std::unique_ptr<graphicsItems::ScatterPlotItem> ownedScatterPlot_;
    std::unique_ptr<graphicsItems::ScatterPlotItem> ownedSelectionScatter_;
};

[[nodiscard]] std::vector<double> pseudoScatter(std::span<const double> data, bool bidir = false);

} // namespace cppqtgraph::widgets

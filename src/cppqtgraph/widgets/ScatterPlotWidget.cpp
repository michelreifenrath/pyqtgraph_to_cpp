// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ScatterPlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/ScatterPlotWidget.hpp"

#include "../../../include/cppqtgraph/functions.hpp"
#include "../../../include/cppqtgraph/graphicsItems/ScatterPlotItem.hpp"
#include "../../../include/cppqtgraph/widgets/PlotWidget.hpp"

#include <QtCore/QMetaType>
#include <QtCore/QSignalBlocker>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <span>

namespace cppqtgraph::widgets {
namespace {

double computeStdDev(std::span<const double> values)
{
    if (values.empty()) {
        return 0.0;
    }
    const double mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    double variance = 0.0;
    for (double value : values) {
        const double delta = value - mean;
        variance += delta * delta;
    }
    variance /= static_cast<double>(values.size());
    return std::sqrt(variance);
}

ColorMapFieldOptions toColorMapFieldOptions(const ScatterPlotFieldOptions& options)
{
    ColorMapFieldOptions mapped;
    mapped.mode = options.mode;
    mapped.units = options.units;
    mapped.values.reserve(options.values.size());
    for (const QVariant& value : options.values) {
        mapped.values.push_back(value.toDouble());
    }
    return mapped;
}

ColorMapRecordArray toColorMapRecords(const ScatterPlotRecordArray& records)
{
    ColorMapRecordArray converted;
    converted.reserve(records.size());
    for (const ScatterPlotRecord& record : records) {
        ColorMapRecord mapped;
        for (auto it = record.cbegin(); it != record.cend(); ++it) {
            mapped.insert(it.key(), it.value().toDouble());
        }
        converted.push_back(mapped);
    }
    return converted;
}

} // namespace

std::vector<double> pseudoScatter(std::span<const double> data, bool bidir)
{
    const std::size_t count = data.size();
    std::vector<double> yvals(count, 0.0);
    if (count == 0) {
        return yvals;
    }

    double spacing = 2.0 * computeStdDev(data) / std::sqrt(static_cast<double>(count));
    if (!std::isfinite(spacing) || spacing <= 0.0) {
        spacing = 1.0;
    }

    const double minimum = *std::min_element(data.begin(), data.end());
    const double maximum = *std::max_element(data.begin(), data.end());
    const int binCount = static_cast<int>((maximum - minimum) / spacing) + 1;
    const double binWidth = binCount <= 1 ? spacing : (maximum - minimum) / static_cast<double>(binCount - 1);

    QHash<int, int> binCounts;
    for (std::size_t index = 0; index < count; ++index) {
        const int bin = binWidth <= 0.0 ? 0 : static_cast<int>((data[index] - minimum) / binWidth);
        const int stacked = binCounts.value(bin, -1) + 1;
        binCounts.insert(bin, stacked);
        yvals[index] = static_cast<double>(stacked);
    }

    if (bidir) {
        for (int bin = 0; bin < binCount; ++bin) {
            const int stacked = binCounts.value(bin, 0);
            const double center = static_cast<double>(stacked) * 0.5;
            for (std::size_t index = 0; index < count; ++index) {
                const int pointBin = binWidth <= 0.0 ? 0 : static_cast<int>((data[index] - minimum) / binWidth);
                if (pointBin == bin) {
                    yvals[index] -= center;
                }
            }
        }
    }

    return yvals;
}

ScatterPlotWidget::ScatterPlotWidget(QWidget* parent)
    : QSplitter(Qt::Horizontal, parent)
{
    ctrlPanel_ = new QSplitter(Qt::Vertical, this);
    fieldList_ = new QListWidget(ctrlPanel_);
    fieldList_->setSelectionMode(QAbstractItemView::ExtendedSelection);

    auto* colorMapContainer = new QWidget(ctrlPanel_);
    auto* colorMapLayout = new QVBoxLayout(colorMapContainer);
    colorMapLayout->setContentsMargins(0, 0, 0, 0);
    colorMap_ = new ColorMapWidget(colorMapContainer);
    colorMapLayout->addWidget(colorMap_);

    plot_ = new PlotWidget(this);

    ctrlPanel_->addWidget(fieldList_);
    ctrlPanel_->addWidget(colorMapContainer);
    addWidget(ctrlPanel_);
    addWidget(plot_);

    connect(fieldList_, &QListWidget::itemSelectionChanged, this, &ScatterPlotWidget::fieldSelectionChanged);
    connect(colorMap_, &ColorMapWidget::sigColorMapChanged, this, &ScatterPlotWidget::colorMapChanged);
}

ScatterPlotWidget::~ScatterPlotWidget()
{
    clearPlotItems();
}

void ScatterPlotWidget::setFields(const QVector<QPair<QString, ScatterPlotFieldOptions>>& fields,
    const QString& mouseOverField)
{
    fields_ = fields;
    mouseOverField_ = mouseOverField;
    fieldList_->clear();

    QVector<QPair<QString, ColorMapFieldOptions>> colorMapFields;
    colorMapFields.reserve(fields.size());
    for (const auto& [name, options] : fields) {
        fieldList_->addItem(name);
        colorMapFields.push_back({name, toColorMapFieldOptions(options)});
    }
    colorMap_->setFields(colorMapFields);
    updatePlot();
}

void ScatterPlotWidget::setSelectedFields(const QStringList& fields)
{
    const QSignalBlocker blocker(fieldList_);
    fieldList_->clearSelection();
    for (const QString& field : fields) {
        for (int row = 0; row < fieldList_->count(); ++row) {
            if (fieldList_->item(row)->text() == field) {
                fieldList_->item(row)->setSelected(true);
                break;
            }
        }
    }
    fieldSelectionChanged();
}

void ScatterPlotWidget::setData(const ScatterPlotRecordArray& data)
{
    data_ = data;
    indices_.resize(data.size());
    std::iota(indices_.begin(), indices_.end(), 0);
    filterMask_.clear();
    indexMapCache_.reset();
    updatePlot();
}

void ScatterPlotWidget::setSelectedIndices(const QVector<int>& indices)
{
    selectedIndices_ = indices;
    updateSelected();
}

void ScatterPlotWidget::setFilterMask(const QVector<bool>& mask)
{
    filterMask_ = mask;
    indexMapCache_.reset();
    updatePlot();
}

void ScatterPlotWidget::emitPointClicked(int visibleIndex)
{
    if (visibleIndex < 0 || visibleIndex >= visibleIndices_.size()) {
        return;
    }

    QVariantMap point;
    point.insert(QStringLiteral("index"), visibleIndex);
    point.insert(QStringLiteral("originalIndex"), visibleIndices_.at(visibleIndex));
    sigScatterPlotClicked(this, {point}, {});
}

void ScatterPlotWidget::emitPointHovered(const QVector<int>& hoveredVisibleIndices)
{
    QVariantList points;
    points.reserve(hoveredVisibleIndices.size());
    for (int visibleIndex : hoveredVisibleIndices) {
        if (visibleIndex < 0 || visibleIndex >= visibleIndices_.size()) {
            continue;
        }
        QVariantMap point;
        point.insert(QStringLiteral("index"), visibleIndex);
        point.insert(QStringLiteral("originalIndex"), visibleIndices_.at(visibleIndex));
        points.push_back(point);
    }
    sigScatterPlotHovered(this, points, {});
}

void ScatterPlotWidget::fieldSelectionChanged()
{
    const QList<QListWidgetItem*> selected = fieldList_->selectedItems();
    if (selected.size() > 2) {
        const QSignalBlocker blocker(fieldList_);
        for (int index = 1; index < selected.size() - 1; ++index) {
            selected.at(index)->setSelected(false);
        }
    }
    updatePlot();
}

void ScatterPlotWidget::colorMapChanged(ColorMapWidget* /*widget*/)
{
    updatePlot();
}

void ScatterPlotWidget::clearPlotItems()
{
    if (plot_ == nullptr || plot_->getPlotItem() == nullptr) {
        scatterPlot_ = nullptr;
        selectionScatter_ = nullptr;
        ownedScatterPlot_.reset();
        ownedSelectionScatter_.reset();
        return;
    }

    if (scatterPlot_ != nullptr) {
        plot_->getPlotItem()->removeItem(scatterPlot_);
    }
    if (selectionScatter_ != nullptr) {
        plot_->getPlotItem()->removeItem(selectionScatter_);
    }
    scatterPlot_ = nullptr;
    selectionScatter_ = nullptr;
    ownedScatterPlot_.reset();
    ownedSelectionScatter_.reset();
}

double ScatterPlotWidget::fieldValueAsDouble(const ScatterPlotRecord& record, const QString& fieldName) const
{
    const auto it = record.constFind(fieldName);
    if (it == record.cend()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const QVariant value = it.value();
    if (value.typeId() == QMetaType::Double || value.typeId() == QMetaType::Int || value.typeId() == QMetaType::LongLong) {
        return value.toDouble();
    }

    for (const auto& [name, options] : fields_) {
        if (name != fieldName) {
            continue;
        }
        const QString stringValue = value.toString();
        const int enumIndex = options.values.indexOf(value);
        if (enumIndex >= 0) {
            return static_cast<double>(enumIndex);
        }
        const int listedIndex = std::distance(options.values.begin(),
            std::find_if(options.values.begin(), options.values.end(), [&stringValue](const QVariant& candidate) {
                return candidate.toString() == stringValue;
            }));
        if (listedIndex < options.values.size()) {
            return static_cast<double>(listedIndex);
        }
        return static_cast<double>(options.values.size());
    }

    return value.toDouble();
}

bool ScatterPlotWidget::passesFilter(const ScatterPlotRecord& /*record*/, int originalIndex) const
{
    if (filterMask_.isEmpty()) {
        return true;
    }
    if (originalIndex < 0 || originalIndex >= filterMask_.size()) {
        return false;
    }
    return filterMask_.at(originalIndex);
}

void ScatterPlotWidget::updatePlot()
{
    clearPlotItems();
    visibleData_.clear();
    visibleIndices_.clear();
    visibleX_.clear();
    visibleY_.clear();
    indexMapCache_.reset();

    if (data_.isEmpty()) {
        plot_->getPlotItem()->setTitle({});
        return;
    }

    const QList<QListWidgetItem*> selectedItems = fieldList_->selectedItems();
    if (selectedItems.isEmpty()) {
        plot_->getPlotItem()->setTitle({});
        return;
    }

    QStringList selectedFields;
    QStringList selectedUnits;
    selectedFields.reserve(selectedItems.size());
    selectedUnits.reserve(selectedItems.size());
    for (QListWidgetItem* item : selectedItems) {
        selectedFields.push_back(item->text());
        QString units;
        for (const auto& [name, options] : fields_) {
            if (name == item->text()) {
                units = options.units;
                break;
            }
        }
        selectedUnits.push_back(units);
    }

    ScatterPlotRecordArray filteredRecords;
    QVector<int> filteredIndices;
    filteredRecords.reserve(data_.size());
    filteredIndices.reserve(data_.size());
    for (int row = 0; row < data_.size(); ++row) {
        if (!passesFilter(data_.at(row), row)) {
            continue;
        }
        filteredRecords.push_back(data_.at(row));
        filteredIndices.push_back(indices_.at(row));
    }

    if (filteredRecords.isEmpty()) {
        return;
    }

    std::vector<double> xValues;
    std::vector<double> yValues;
    xValues.reserve(filteredRecords.size());
    yValues.reserve(filteredRecords.size());

    const QString xField = selectedFields.at(0);
    for (const ScatterPlotRecord& record : filteredRecords) {
        xValues.push_back(fieldValueAsDouble(record, xField));
    }

    if (selectedFields.size() == 1) {
        plot_->getPlotItem()->setLabel(QStringLiteral("bottom"), xField, selectedUnits.at(0));
        plot_->getPlotItem()->setLabel(QStringLiteral("left"), QStringLiteral("N"));
        plot_->getPlotItem()->setTitle({});

        std::vector<double> finiteX;
        finiteX.reserve(xValues.size());
        for (double value : xValues) {
            if (std::isfinite(value)) {
                finiteX.push_back(value);
            }
        }

        yValues.assign(xValues.size(), 0.0);
        if (!finiteX.empty()) {
            const std::vector<double> pseudoY =
                pseudoScatter(std::span<const double>(finiteX.data(), finiteX.size()));
            std::size_t pseudoIndex = 0;
            for (std::size_t index = 0; index < xValues.size(); ++index) {
                if (std::isfinite(xValues[index])) {
                    yValues[index] = pseudoY[pseudoIndex++];
                }
            }
        }
    } else {
        const QString yField = selectedFields.at(1);
        plot_->getPlotItem()->setLabel(QStringLiteral("bottom"), xField, selectedUnits.at(0));
        plot_->getPlotItem()->setLabel(QStringLiteral("left"), yField, selectedUnits.at(1));
        plot_->getPlotItem()->setTitle({});
        for (const ScatterPlotRecord& record : filteredRecords) {
            yValues.push_back(fieldValueAsDouble(record, yField));
        }
    }

    QVector<bool> mask;
    mask.resize(static_cast<int>(xValues.size()), true);
    for (int index = 0; index < static_cast<int>(xValues.size()); ++index) {
        if (!std::isfinite(xValues[static_cast<std::size_t>(index)])) {
            mask[index] = false;
        }
        if (selectedFields.size() == 2 && !std::isfinite(yValues[static_cast<std::size_t>(index)])) {
            mask[index] = false;
        }
    }

    ScatterPlotRecordArray maskedRecords;
    QVector<int> maskedIndices;
    std::vector<double> maskedX;
    std::vector<double> maskedY;
    for (int index = 0; index < static_cast<int>(xValues.size()); ++index) {
        if (!mask[index]) {
            continue;
        }
        maskedRecords.push_back(filteredRecords.at(index));
        maskedIndices.push_back(filteredIndices.at(index));
        maskedX.push_back(xValues[static_cast<std::size_t>(index)]);
        maskedY.push_back(yValues[static_cast<std::size_t>(index)]);
    }

    if (maskedX.empty()) {
        return;
    }

    visibleData_ = maskedRecords;
    visibleIndices_ = maskedIndices;
    visibleX_ = maskedX;
    visibleY_ = maskedY;

    const auto colorRecords = toColorMapRecords(maskedRecords);
    const auto colorBytes = colorMap_->mapBytes(colorRecords);
    std::vector<QBrush> brushes;
    brushes.reserve(colorBytes.size());
    for (const auto& rgba : colorBytes) {
        brushes.push_back(QBrush(QColor(rgba[0], rgba[1], rgba[2], rgba[3])));
    }

    ownedScatterPlot_ = std::make_unique<graphicsItems::ScatterPlotItem>();
    scatterPlot_ = ownedScatterPlot_.get();
    scatterPlot_->setData(std::span<const double>(visibleX_.data(), visibleX_.size()),
        std::span<const double>(visibleY_.data(), visibleY_.size()));
    scatterPlot_->setSymbol(QStringLiteral("o"));
    scatterPlot_->setPen(QPen(Qt::NoPen));
    if (brushes.size() == visibleX_.size()) {
        scatterPlot_->setBrushes(std::span<const QBrush>(brushes.data(), brushes.size()));
    }
    plot_->getPlotItem()->addItem(scatterPlot_, false);
    updateSelected();
}

void ScatterPlotWidget::updateSelected()
{
    if (selectionScatter_ != nullptr) {
        plot_->getPlotItem()->removeItem(selectionScatter_);
        selectionScatter_ = nullptr;
        ownedSelectionScatter_.reset();
    }

    if (visibleX_.empty() || selectedIndices_.isEmpty()) {
        return;
    }

    const QHash<int, int> mapping = indexMap();
    std::vector<double> selectedX;
    std::vector<double> selectedY;
    for (int originalIndex : selectedIndices_) {
        const auto mapped = mapping.constFind(originalIndex);
        if (mapped == mapping.cend()) {
            continue;
        }
        const int visibleIndex = mapped.value();
        selectedX.push_back(visibleX_[static_cast<std::size_t>(visibleIndex)]);
        selectedY.push_back(visibleY_[static_cast<std::size_t>(visibleIndex)]);
    }

    if (selectedX.empty()) {
        return;
    }

    ownedSelectionScatter_ = std::make_unique<graphicsItems::ScatterPlotItem>();
    selectionScatter_ = ownedSelectionScatter_.get();
    selectionScatter_->setData(std::span<const double>(selectedX.data(), selectedX.size()),
        std::span<const double>(selectedY.data(), selectedY.size()));
    selectionScatter_->setSymbol(QStringLiteral("s"));
    selectionScatter_->setSize(12.0);
    selectionScatter_->setPen(QPen(QColor(Qt::yellow)));
    selectionScatter_->setBrush(QBrush(Qt::NoBrush));
    plot_->getPlotItem()->addItem(selectionScatter_, false);
}

QHash<int, int> ScatterPlotWidget::indexMap() const
{
    if (!indexMapCache_.has_value()) {
        QHash<int, int> mapping;
        for (int visibleIndex = 0; visibleIndex < visibleIndices_.size(); ++visibleIndex) {
            mapping.insert(visibleIndices_.at(visibleIndex), visibleIndex);
        }
        indexMapCache_ = mapping;
    }
    return indexMapCache_.value();
}

} // namespace cppqtgraph::widgets

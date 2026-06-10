#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsWidget.hpp"
#include "../ViewBox/ViewBox.hpp"

#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtWidgets/QGraphicsItem>

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <vector>

class QGraphicsGridLayout;
class QGraphicsSceneResizeEvent;
class QMenu;
class QObject;
class QPainter;
class QStyleOptionGraphicsItem;
class QVariant;
class QWidget;

namespace cppqtgraph::graphicsItems {

class AxisItem;
class ButtonItem;
class LegendItem;
class PlotCurveItem;
class TitleLabel;

namespace PlotItemConfig {
class Ui_Form;
}

class PlotItem : public GraphicsWidget {
public:
    struct DownsampleState {
        int factor = 1;
        bool automatic = false;
        QString method = QStringLiteral("peak");
    };

    struct GridState {
        bool x = false;
        bool y = false;
        double alpha = 128.0 / 255.0;
        int alphaSliderValue = 128;
    };

    struct AlphaState {
        double alpha = 1.0;
        bool automatic = false;
    };

    explicit PlotItem(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{}, bool enableMenu = true);
    ~PlotItem() override;

    PlotItem(const PlotItem&) = delete;
    PlotItem& operator=(const PlotItem&) = delete;
    PlotItem(PlotItem&&) = delete;
    PlotItem& operator=(PlotItem&&) = delete;

    [[nodiscard]] ViewBox* getViewBox() noexcept;
    [[nodiscard]] const ViewBox* getViewBox() const noexcept;

    void addItem(QGraphicsItem* item, bool ignoreBounds = false, const QString& name = QString{});
    void removeItem(QGraphicsItem* item);
    void clear();

    PlotCurveItem* plot(std::span<const double> y, const QString& name = QString{});
    PlotCurveItem* plot(std::span<const double> x, std::span<const double> y, const QString& name = QString{});

    LegendItem* addLegend(std::optional<QPointF> offset = QPointF(30.0, 30.0));
    [[nodiscard]] LegendItem* legend() noexcept;
    [[nodiscard]] const LegendItem* legend() const noexcept;

    AxisItem* getAxis(const QString& name);
    const AxisItem* getAxis(const QString& name) const;
    void setLabel(const QString& axis,
                  const QString& text = QString{},
                  const QString& units = QString{},
                  const QString& unitPrefix = QString{});
    void setTitle(const QString& title = QString{});
    void showAxis(const QString& axis, bool show = true);
    void hideAxis(const QString& axis);

    [[nodiscard]] QMenu* getMenu() noexcept;
    [[nodiscard]] const QMenu* getMenu() const noexcept;
    [[nodiscard]] QMenu* getContextMenus(const QObject* event = nullptr) noexcept;
    void setMenuEnabled(bool enableMenu = true, std::optional<bool> enableViewBoxMenu = std::nullopt);
    [[nodiscard]] bool menuEnabled() const noexcept;
    void setContextMenuActionVisible(const QString& name, bool visible);

    void setLogMode(std::optional<bool> x = std::nullopt, std::optional<bool> y = std::nullopt);
    [[nodiscard]] std::array<bool, 2> logMode() const noexcept;
    void showGrid(std::optional<bool> x = std::nullopt,
                  std::optional<bool> y = std::nullopt,
                  std::optional<double> alpha = std::nullopt);
    [[nodiscard]] GridState gridState() const noexcept;
    void setDownsampling(std::optional<int> factor = std::nullopt,
                         std::optional<bool> automatic = std::nullopt,
                         std::optional<QString> mode = std::nullopt);
    void setDownsampling(int factor, bool automatic, const QString& mode);
    [[nodiscard]] DownsampleState downsampleMode() const;
    void setClipToView(bool clip);
    [[nodiscard]] bool clipToViewMode() const noexcept;
    [[nodiscard]] AlphaState alphaState() const;
    [[nodiscard]] std::optional<bool> pointMode() const;
    void hideButtons();
    void showButtons();
    [[nodiscard]] bool buttonsHidden() const noexcept;

    void setRange(const QRectF& rect, qreal padding = 0.02, bool update = true, bool disableAutoRange = true);
    void setXRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void setYRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void autoRange(std::optional<qreal> padding = std::nullopt);
    [[nodiscard]] ViewBox::Range2D viewRange() const;
    [[nodiscard]] QRectF viewRect() const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    friend class PlotCurveItem;

    enum class AxisSlot : std::size_t {
        Top = 0,
        Bottom = 1,
        Left = 2,
        Right = 3,
    };

    static AxisSlot axisSlot(const QString& name);
    [[nodiscard]] AxisItem* axis(AxisSlot slot) noexcept;
    [[nodiscard]] const AxisItem* axis(AxisSlot slot) const noexcept;
    [[nodiscard]] bool isInternalChild(const QGraphicsItem* item) const noexcept;
    void connectAxisRanges();
    void syncAxisRanges();
    void updateCurveTransforms();
    void detachDirectChild(QGraphicsItem* item);
    void setupConfigMenu(bool enableMenu);
    void updateLogMode();
    void updateGrid();
    void updateDownsampling();
    void updateAlpha();

    QGraphicsGridLayout* layout_ = nullptr;
    ViewBox* vb_ = nullptr;
    std::array<AxisItem*, 4> axes_{{nullptr, nullptr, nullptr, nullptr}};
    TitleLabel* titleLabel_ = nullptr;
    ButtonItem* autoBtn_ = nullptr;
    LegendItem* legend_ = nullptr;
    std::unique_ptr<PlotItemConfig::Ui_Form> ctrl_;
    std::unique_ptr<QWidget> ctrlWidget_;
    std::unique_ptr<QMenu> ctrlMenu_;
    std::vector<QGraphicsItem*> items_;
    std::vector<std::unique_ptr<PlotCurveItem>> ownedCurves_;
    GridState gridState_;
    std::array<bool, 2> logMode_{{false, false}};
    bool initialized_ = false;
    bool forwardingChild_ = false;
    bool menuEnabled_ = true;
    bool buttonsHidden_ = false;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;
};

} // namespace cppqtgraph::graphicsItems

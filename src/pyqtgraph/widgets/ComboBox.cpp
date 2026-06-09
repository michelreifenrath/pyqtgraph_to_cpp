// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ComboBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/ComboBox.hpp"

#include <QtCore/QMetaType>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

#ifdef Q_OS_DARWIN
#include <QtWidgets/QComboBox>
#endif

namespace pyqtgraph::widgets {

namespace {

QMap<QString, QVariant> normalizeItems(const QVariant& items)
{
    if (items.metaType().id() == QMetaType::QStringList) {
        QMap<QString, QVariant> mapped;
        for (const QString& text : items.toStringList()) {
            mapped.insert(text, text);
        }
        return mapped;
    }

    if (items.canConvert<QVariantList>()) {
        const QVariantList list = items.toList();
        QMap<QString, QVariant> mapped;
        for (const QVariant& entry : list) {
            if (entry.metaType().id() == QMetaType::QString) {
                const QString text = entry.toString();
                mapped.insert(text, text);
            } else if (entry.canConvert<QVariantList>()) {
                const QVariantList pair = entry.toList();
                if (pair.size() < 2) {
                    throw std::invalid_argument("item pair must contain text and value");
                }
                mapped.insert(pair.at(0).toString(), pair.at(1));
            } else {
                throw std::invalid_argument("items argument must be list or dict or tuple");
            }
        }
        return mapped;
    }

    if (items.canConvert<QVariantMap>()) {
        QMap<QString, QVariant> mapped;
        const QVariantMap map = items.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            mapped.insert(it.key(), it.value());
        }
        return mapped;
    }

    throw std::invalid_argument("items argument must be list or dict or tuple");
}

void ensureUniqueTexts(const QMap<QString, QVariant>& incoming, const QMap<QString, QVariant>& existing = {})
{
    for (auto it = incoming.constBegin(); it != incoming.constEnd(); ++it) {
        if (existing.contains(it.key())) {
            throw std::runtime_error(QStringLiteral("ComboBox already has item named \"%1\".")
                                         .arg(it.key())
                                         .toStdString());
        }
    }
}

} // namespace

ComboBox::IgnoreIndexChangeGuard::IgnoreIndexChangeGuard(ComboBox& box)
    : box_(box)
    , previous_(box.ignoreIndexChange_)
{
    box_.ignoreIndexChange_ = true;
}

ComboBox::IgnoreIndexChangeGuard::~IgnoreIndexChangeGuard()
{
    box_.ignoreIndexChange_ = previous_;
}

ComboBox::ComboBox(QWidget* parent, const QVariant& items, const QVariant& defaultValue)
    : QComboBox(parent)
{
#ifdef Q_OS_DARWIN
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
#endif
    connect(this, qOverload<int>(&QComboBox::currentIndexChanged), this, &ComboBox::indexChanged);

    if (items.isValid() && !items.isNull()) {
        setItems(items);
        if (defaultValue.isValid() && !defaultValue.isNull()) {
            setValue(defaultValue);
        }
    }
}

QVariant ComboBox::value() const
{
    if (count() == 0) {
        return QVariant();
    }
    const QString text = currentText();
    return items_.value(text);
}

void ComboBox::setValue(const QVariant& value)
{
    QString text;
    bool found = false;
    for (auto it = items_.constBegin(); it != items_.constEnd(); ++it) {
        if (it.value() == value) {
            text = it.key();
            found = true;
            break;
        }
    }
    if (!found) {
        throw std::invalid_argument(value.toString().toStdString());
    }
    setText(text);
}

void ComboBox::setText(const QString& text)
{
    const int index = findText(text);
    if (index == -1) {
        throw std::invalid_argument(text.toStdString());
    }
    setCurrentIndex(index);
}

void ComboBox::setItems(const QVariant& items)
{
    withBlockedSignalsIfUnchanged([&]() {
        IgnoreIndexChangeGuard guard(*this);
        clear();
        addItems(items);
        return true;
    });
}

void ComboBox::addItem(const QString& text, const QVariant& value)
{
    IgnoreIndexChangeGuard guard(*this);
    if (items_.contains(text)) {
        throw std::runtime_error(QStringLiteral("ComboBox already has item named \"%1\".")
                                     .arg(text)
                                     .toStdString());
    }

    const QVariant itemValue = value.isValid() ? value : QVariant(text);
    items_.insert(text, itemValue);
    QComboBox::addItem(text, itemValue);
    itemsChanged();
}

void ComboBox::addItems(const QVariant& items)
{
    IgnoreIndexChangeGuard guard(*this);
    const QMap<QString, QVariant> normalized = normalizeItems(items);
    ensureUniqueTexts(normalized, items_);

    QStringList texts;
    texts.reserve(normalized.size());
    for (auto it = normalized.constBegin(); it != normalized.constEnd(); ++it) {
        items_.insert(it.key(), it.value());
        texts.append(it.key());
    }
    QComboBox::addItems(texts);
    for (int index = 0; index < texts.size(); ++index) {
        setItemData(index, items_.value(texts.at(index)));
    }
    itemsChanged();
}

void ComboBox::clear()
{
    IgnoreIndexChangeGuard guard(*this);
    items_.clear();
    QComboBox::clear();
    itemsChanged();
}

void ComboBox::insertItem(int index, const QString& text, const QVariant& value)
{
    Q_UNUSED(index);
    Q_UNUSED(text);
    Q_UNUSED(value);
    throw std::logic_error("ComboBox::insertItem is not implemented");
}

void ComboBox::insertItems(int index, const QStringList& texts)
{
    Q_UNUSED(index);
    Q_UNUSED(texts);
    throw std::logic_error("ComboBox::insertItems is not implemented");
}

void ComboBox::setItemValue(const QString& name, const QVariant& value)
{
    if (!items_.contains(name)) {
        addItem(name, value);
        return;
    }
    items_.insert(name, value);
}

QVariant ComboBox::saveState() const
{
    const int index = currentIndex();
    if (index < 0) {
        return QVariant();
    }

    QVariant data = itemData(index);
    if (data.isValid()) {
        bool ok = false;
        const int intValue = data.toInt(&ok);
        if (ok) {
            return intValue;
        }
    }
    return itemText(index);
}

void ComboBox::restoreState(const QVariant& state)
{
    if (state.metaType().id() == QMetaType::Int || state.metaType().id() == QMetaType::LongLong) {
        const int index = findData(state);
        if (index > -1) {
            setCurrentIndex(index);
            return;
        }
    }
    setCurrentIndex(findText(state.toString()));
}

void ComboBox::indexChanged(int index)
{
    Q_UNUSED(index);
    if (ignoreIndexChange_) {
        return;
    }
    chosenText_ = currentText();
}

void ComboBox::itemsChanged()
{
    if (chosenText_.isEmpty()) {
        return;
    }
    try {
        setText(chosenText_);
    } catch (const std::invalid_argument&) {
    }
}

} // namespace pyqtgraph::widgets

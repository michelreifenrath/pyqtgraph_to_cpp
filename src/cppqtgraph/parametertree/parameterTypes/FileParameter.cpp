// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/file.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/FileParameter.hpp"

#include "../../../../include/cppqtgraph/parametertree/Parameter.hpp"
#include "../../../../include/cppqtgraph/widgets/FileDialog.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <algorithm>

namespace cppqtgraph::parametertree {
namespace {

QVariant defaultFilePickerDialog(const FilePickerRequest& request);

FilePickerProvider& mutableFilePickerProvider()
{
    static FilePickerProvider provider = defaultFilePickerDialog;
    return provider;
}

template <typename Enum>
bool enumValueFromName(const QString& name, Enum& out)
{
    const QMetaEnum meta = QMetaEnum::fromType<Enum>();
    if (!meta.isValid()) {
        return false;
    }
    bool ok = false;
    const int value = meta.keyToValue(name.toUtf8().constData(), &ok);
    if (!ok) {
        return false;
    }
    out = static_cast<Enum>(value);
    return true;
}

QFileDialog::Options optionsFromVariant(const QVariant& value)
{
    QFileDialog::Options options;
    QStringList names;
    if (value.metaType().id() == QMetaType::QString) {
        names.append(value.toString());
    } else if (value.canConvert<QStringList>()) {
        names = value.toStringList();
    } else if (value.canConvert<QVariantList>()) {
        for (const QVariant& entry : value.toList()) {
            names.append(entry.toString());
        }
    }

    const QMetaEnum optionEnum = QMetaEnum::fromType<QFileDialog::Option>();
    for (const QString& name : names) {
        if (name.isEmpty()) {
            continue;
        }
        bool ok = false;
        const int flag = optionEnum.keyToValue(name.toUtf8().constData(), &ok);
        if (ok) {
            options |= QFileDialog::Option(flag);
        }
    }
    return options;
}

QString normalizePickerPath(const QString& path)
{
    return QDir::cleanPath(path);
}

QStringList normalizePickerPaths(const QStringList& paths)
{
    QStringList normalized;
    normalized.reserve(paths.size());
    for (const QString& path : paths) {
        normalized.append(normalizePickerPath(path));
    }
    return normalized;
}

QStringList applyRelativeTo(const QStringList& paths, const QString& relativeTo)
{
    if (relativeTo.isEmpty()) {
        return paths;
    }
    QStringList relative;
    relative.reserve(paths.size());
    for (const QString& path : paths) {
        relative.append(QDir(relativeTo).relativeFilePath(path));
    }
    return relative;
}

void applyFilePickerRequest(cppqtgraph::widgets::FileDialog& dialog, const FilePickerRequest& request)
{
    dialog.setModal(true);
    dialog.setWindowTitle(request.windowTitle);
    dialog.setNameFilter(request.nameFilter);
    if (!request.directory.isEmpty()) {
        dialog.setDirectory(request.directory);
    }
    if (!request.selectFile.isEmpty()) {
        dialog.selectFile(request.selectFile);
    }
    dialog.setAcceptMode(request.acceptMode);
    dialog.setFileMode(request.fileMode);
    dialog.setViewMode(request.viewMode);
    dialog.setOptions(request.options);
}

QVariant defaultFilePickerDialog(const FilePickerRequest& request)
{
    cppqtgraph::widgets::FileDialog dialog;
    applyFilePickerRequest(dialog, request);
    if (!dialog.exec()) {
        return QVariant();
    }

    const QRegularExpression suffixPattern(QStringLiteral("(\\.\\w+)+"));
    const QRegularExpressionMatch match = suffixPattern.match(dialog.selectedNameFilter());
    if (match.hasMatch()) {
        QString extension = match.captured(1);
        if (extension.startsWith(QLatin1Char('.'))) {
            extension = extension.mid(1);
        }
        dialog.setDefaultSuffix(extension);
    }

    QStringList files = dialog.selectedFiles();
    if (request.fileMode == QFileDialog::ExistingFiles) {
        return QVariant::fromValue(files);
    }
    if (files.isEmpty()) {
        return QVariant();
    }
    return files.front();
}

} // namespace

void setFilePickerProvider(FilePickerProvider provider)
{
    mutableFilePickerProvider() = std::move(provider);
}

FilePickerProvider filePickerProvider()
{
    return mutableFilePickerProvider();
}

FilePickerRequest filePickerRequestFromOptions(const QVariantMap& opts, const QString& title)
{
    FilePickerRequest request;
    request.windowTitle = opts.value(QStringLiteral("windowTitle"), title).toString();
    request.nameFilter = opts.value(QStringLiteral("nameFilter")).toString();
    request.directory = opts.value(QStringLiteral("directory")).toString();
    request.selectFile = opts.value(QStringLiteral("selectFile")).toString();
    request.relativeTo = opts.value(QStringLiteral("relativeTo")).toString();

    if (opts.contains(QStringLiteral("acceptMode"))) {
        enumValueFromName(opts.value(QStringLiteral("acceptMode")).toString(), request.acceptMode);
    }
    if (opts.contains(QStringLiteral("fileMode"))) {
        enumValueFromName(opts.value(QStringLiteral("fileMode")).toString(), request.fileMode);
    }
    if (opts.contains(QStringLiteral("viewMode"))) {
        enumValueFromName(opts.value(QStringLiteral("viewMode")).toString(), request.viewMode);
    }
    if (opts.contains(QStringLiteral("options"))) {
        request.options = optionsFromVariant(opts.value(QStringLiteral("options")));
    }
    return request;
}

QVariant shapeFilePickerResult(const QVariant& picked, QFileDialog::FileMode fileMode)
{
    if (fileMode == QFileDialog::ExistingFiles) {
        if (picked.metaType().id() == QMetaType::QStringList) {
            if (picked.toStringList().isEmpty()) {
                return QVariant();
            }
            return picked;
        }
        if (picked.metaType().id() == QMetaType::QString) {
            const QString path = picked.toString();
            if (path.isEmpty()) {
                return QVariant();
            }
            return QVariant::fromValue(QStringList{path});
        }
        return QVariant();
    }

    if (picked.metaType().id() == QMetaType::QStringList) {
        const QStringList paths = picked.toStringList();
        if (paths.isEmpty()) {
            return QVariant();
        }
        return paths.front();
    }
    if (picked.metaType().id() == QMetaType::QString && !picked.toString().isEmpty()) {
        return picked;
    }
    return QVariant();
}

FileParameter::FileParameter(QVariantMap opts, QObject* parent)
    : Parameter([&opts]() {
          if (!opts.contains(QStringLiteral("readonly"))) {
              opts.insert(QStringLiteral("readonly"), true);
          }
          return opts;
      }(),
      parent)
{
}

ParameterItem* FileParameter::makeTreeItem(int depth)
{
    return new FileParameterItem(this, depth);
}

FileParameterItem::FileParameterItem(Parameter* param, int depth)
    : StrParameterItem(param, depth)
{
    if (auto* lineEdit = qobject_cast<QLineEdit*>(editorWidget())) {
        lineEdit->setReadOnly(true);
    }

    browseBtn_ = new QPushButton(QStringLiteral("..."));
    browseBtn_->setFixedWidth(25);
    browseBtn_->setContentsMargins(0, 0, 0, 0);
    QObject::connect(browseBtn_, &QPushButton::clicked, browseBtn_, [this]() { retrieveFileSelection(); });

    if (layoutWidget_ != nullptr) {
        if (auto* layout = qobject_cast<QHBoxLayout*>(layoutWidget_->layout())) {
            layout->insertWidget(2, browseBtn_);
        }
    }

    storedValue_ = param->value();
    writeEditorValue(storedValue_);
    updateDisplayElision();
    updateDefaultBtn();
}

void FileParameterItem::bindEditor(QWidget* /*editor*/)
{
}

QVariant FileParameterItem::readEditorValue() const
{
    return storedValue_;
}

void FileParameterItem::writeEditorValue(const QVariant& val)
{
    storedValue_ = val;
    if (auto* lineEdit = qobject_cast<QLineEdit*>(editorWidget())) {
        const QSignalBlocker blocker(lineEdit);
        if (val.metaType().id() == QMetaType::QStringList) {
            lineEdit->setText(val.toStringList().join(QStringLiteral(", ")));
        } else if (val.isValid() && !val.isNull()) {
            lineEdit->setText(val.toString());
        } else {
            lineEdit->clear();
        }
    }
    updateDisplayElision();
}

void FileParameterItem::updateDisplayLabel(const QVariant& value)
{
    if (displayLabel_ == nullptr) {
        return;
    }
    const QVariant display = value.isValid() ? value : storedValue_;
    if (display.metaType().id() == QMetaType::QStringList) {
        displayLabel_->setText(display.toStringList().join(QStringLiteral(", ")));
    } else if (display.isValid() && !display.isNull()) {
        displayLabel_->setText(display.toString());
    } else {
        displayLabel_->clear();
    }
    updateDisplayElision();
}

void FileParameterItem::updateDisplayElision()
{
    if (displayLabel_ == nullptr) {
        return;
    }
    QString value = displayLabel_->text();
    const QFontMetricsF metrics(displayLabel_->font());
    value = metrics.elidedText(value, Qt::ElideLeft, std::max(0, displayLabel_->width() - 5));
    displayLabel_->setText(value);
}

void FileParameterItem::updateDefaultBtn()
{
    if (defaultBtn_ == nullptr || param_ == nullptr) {
        return;
    }
    defaultBtn_->setEnabled(param_->valueModifiedSinceResetToDefault() && param_->enabled());
    defaultBtn_->setVisible(param_->hasDefault());
}

void FileParameterItem::editorValueChanging(const QVariant& /*value*/)
{
}

void FileParameterItem::widgetValueChanged()
{
}

void FileParameterItem::retrieveFileSelection()
{
    if (param_ == nullptr) {
        return;
    }

    const QVariantMap opts = param_->options();
    QVariant current = param_->hasDefault() || param_->options().contains(QStringLiteral("value")) ? param_->value()
                                                                                                  : QVariant();
    QString useDir;
    if (current.metaType().id() == QMetaType::QStringList && !current.toStringList().isEmpty()) {
        const QString first = current.toStringList().front();
        useDir = QFileInfo(first).isFile() ? QFileInfo(first).absolutePath() : first;
    } else if (current.metaType().id() == QMetaType::QString && !current.toString().isEmpty()) {
        const QString path = current.toString();
        useDir = QFileInfo(path).isFile() ? QFileInfo(path).absolutePath() : path;
    }

    QVariantMap requestOpts = opts;
    if (!useDir.isEmpty()) {
        requestOpts.insert(QStringLiteral("directory"), useDir);
    } else if (!opts.value(QStringLiteral("directory")).toString().isEmpty()) {
        requestOpts.insert(QStringLiteral("directory"), opts.value(QStringLiteral("directory")));
    } else {
        requestOpts.insert(QStringLiteral("directory"), QDir::currentPath());
    }

    FilePickerRequest request = filePickerRequestFromOptions(requestOpts, param_->title());
  if (request.windowTitle.isEmpty()) {
        request.windowTitle = param_->title();
    }

    const FilePickerProvider provider = filePickerProvider();
    if (!provider) {
        return;
    }

    QVariant picked = provider(request);
    if (picked.metaType().id() == QMetaType::QStringList) {
        picked = QVariant::fromValue(normalizePickerPaths(picked.toStringList()));
        picked = QVariant::fromValue(applyRelativeTo(picked.toStringList(), request.relativeTo));
    } else if (picked.metaType().id() == QMetaType::QString) {
        QString path = normalizePickerPath(picked.toString());
        if (!request.relativeTo.isEmpty()) {
            path = QDir(request.relativeTo).relativeFilePath(path);
        }
        picked = path;
    }

    const QVariant shaped = shapeFilePickerResult(picked, request.fileMode);
    if (!shaped.isValid()) {
        return;
    }
    if (request.fileMode == QFileDialog::ExistingFiles && shaped.toStringList().isEmpty()) {
        return;
    }
    param_->setValue(shaped);
}

} // namespace cppqtgraph::parametertree

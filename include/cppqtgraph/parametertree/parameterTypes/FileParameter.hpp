#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/file.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/parameterTypes/StrParameterItem.hpp>

#include <QtWidgets/QFileDialog>

#include <functional>

class QPushButton;

namespace cppqtgraph::parametertree {

struct FilePickerRequest {
    QString windowTitle;
    QString nameFilter;
    QString directory;
    QString selectFile;
    QString relativeTo;
    QFileDialog::AcceptMode acceptMode{QFileDialog::AcceptOpen};
    QFileDialog::FileMode fileMode{QFileDialog::AnyFile};
    QFileDialog::ViewMode viewMode{QFileDialog::Detail};
    QFileDialog::Options options{QFileDialog::Options()};
};

using FilePickerProvider = std::function<QVariant(const FilePickerRequest&)>;

void setFilePickerProvider(FilePickerProvider provider);
[[nodiscard]] FilePickerProvider filePickerProvider();

[[nodiscard]] FilePickerRequest filePickerRequestFromOptions(const QVariantMap& opts, const QString& title);
[[nodiscard]] QVariant shapeFilePickerResult(const QVariant& picked, QFileDialog::FileMode fileMode);

class FileParameterItem final : public StrParameterItem {
public:
    FileParameterItem(Parameter* param, int depth);

    void editorValueChanging(const QVariant& value) override;
    void widgetValueChanged() override;

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void updateDisplayLabel(const QVariant& value = QVariant()) override;
    void updateDefaultBtn() override;

private:
    void retrieveFileSelection();
    void updateDisplayElision();

    QPushButton* browseBtn_ = nullptr;
    QVariant storedValue_;
};

class FileParameter final : public Parameter {
public:
    explicit FileParameter(QVariantMap opts, QObject* parent = nullptr);

    ParameterItem* makeTreeItem(int depth) override;
};

} // namespace cppqtgraph::parametertree

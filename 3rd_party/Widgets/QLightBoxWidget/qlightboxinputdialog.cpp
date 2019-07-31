#include "qlightboxinputdialog.h"

#include <3rd_party/Delegates/TreeViewItemDelegate/TreeViewItemDelegate.h>
#include <3rd_party/Helpers/ScrollerHelper.h>
#include <3rd_party/Widgets/MaterialLineEdit/MaterialLineEdit.h>
#include <3rd_party/Widgets/SimpleTextEditor/SimpleTextEditorWidget.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QDialogButtonBox>
#include <QInputMethod>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QStringListModel>
#include <QVBoxLayout>

namespace {
    const char* focusProperty = "focusedOnExec";
}


QString QLightBoxInputDialog::getText(QWidget* _parent, const QString& _title, const QString& _label, const QString& _text)
{
    QSharedPointer<QLightBoxInputDialog> dialog{textDialog(_parent, _title, _label, _text)};

    QString result;
    if (dialog->exec() == QLightBoxDialog::Accepted) {
        result = dialog->text();
    }
    return result;
}

QLightBoxInputDialog* QLightBoxInputDialog::textDialog(QWidget* _parent, const QString& _title, const QString& _label, const QString& _text)
{
    QLightBoxInputDialog* dialog = new QLightBoxInputDialog(_parent);
    dialog->setWindowTitle(_title);
#ifndef MOBILE_OS
    dialog->m_label->setText(_label);
#else
    dialog->m_lineEdit->setLabel(_label);
    connect(dialog, &QLightBoxInputDialog::showed, QApplication::inputMethod(), &QInputMethod::show);
#endif
    dialog->m_lineEdit->setText(_text);
    dialog->m_lineEdit->setProperty(::focusProperty, true);
    dialog->m_textEdit->hide();
    dialog->m_listWidget->hide();

    return dialog;
}

QString QLightBoxInputDialog::getLongText(QWidget* _parent, const QString& _title, const QString& _label, const QString& _text)
{
    QLightBoxInputDialog dialog(_parent);
    dialog.setWindowTitle(_title);
    dialog.m_label->setText(_label);
    dialog.m_lineEdit->hide();
    dialog.m_textEdit->setPlainText(_text);
    dialog.m_textEdit->setProperty(::focusProperty, true);
    dialog.m_listWidget->hide();

    QString result;
    if (dialog.exec() == QLightBoxDialog::Accepted) {
        result = dialog.m_textEdit->toPlainText();
    }
    return result;
}

QString QLightBoxInputDialog::getItem(QWidget* _parent, const QString& _title, const QStringList& _items, const QString& _selectedItem)
{
    QStringListModel model(_items);
    return getItem(_parent, _title, &model, _selectedItem);
}

QString QLightBoxInputDialog::getItem(QWidget* _parent, const QString& _title, const QAbstractItemModel* _itemsModel, const QString& _selectedItem)
{
    const bool STRETCH_LIST_WIDGET = true;
    QLightBoxInputDialog dialog(_parent, STRETCH_LIST_WIDGET);
    dialog.setWindowTitle(_title);
    dialog.m_label->hide();
    dialog.m_lineEdit->hide();
    dialog.m_textEdit->hide();

    //
    // Наполняем список переключателями
    //
    {
        const int invalidRow = -1;
        int selectedRow = invalidRow;
        for (int row = 0; row < _itemsModel->rowCount(); ++row) {
            const QString itemText = _itemsModel->data(_itemsModel->index(row, 0)).toString();
            dialog.m_listWidget->addItem(itemText);
            if (itemText == _selectedItem) {
                selectedRow = row;
            }
        }

        if (selectedRow != invalidRow) {
            dialog.m_listWidget->setCurrentRow(selectedRow);
        }
    }

    QString result;
    if (dialog.exec() == QLightBoxDialog::Accepted) {
        result = dialog.m_listWidget->currentIndex().data().toString();
    }
    return result;
}

QString QLightBoxInputDialog::text() const
{
    return m_lineEdit->text();
}

QLightBoxInputDialog::~QLightBoxInputDialog()
{
}

QLightBoxInputDialog::QLightBoxInputDialog(QWidget* _parent, bool _isContentStretchable) :
    QLightBoxDialog(_parent, true, _isContentStretchable),
    m_label(new QLabel(this)),
#ifndef MOBILE_OS
    m_lineEdit(new QLineEdit(this)),
#else
    m_lineEdit(new MaterialLineEdit(this)),
#endif

    m_textEdit(new SimpleTextEditorWidget(this)),
    m_listWidget(new QListWidget(this)),
    m_buttons(new QDialogButtonBox(this))
{
}

void QLightBoxInputDialog::initView()
{
    m_lineEdit->setMinimumWidth(500);

    m_textEdit->setToolbarVisible(false);
    m_textEdit->setMinimumWidth(500);
    m_textEdit->setMinimumHeight(400);

    m_listWidget->setProperty("dialog-container", true);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listWidget->setItemDelegate(new TreeViewItemDelegate(m_listWidget));
    m_listWidget->setMinimumWidth(500);
#ifdef MOBILE_OS
    ScrollerHelper::addScroller(m_listWidget);
#endif

    m_buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    foreach (QAbstractButton* button, m_buttons->buttons())	{
        button->setProperty("flat", true);

#ifdef MOBILE_OS
        //
        // Для мобильных делаем кнопки в верхнем регистре и убераем ускорители
        //
        button->setText(button->text().toUpper().remove("&"));
#endif
    }

    QVBoxLayout* layout = new QVBoxLayout;
#ifdef MOBILE_OS
    layout->setContentsMargins(QMargins());
#endif
    layout->addWidget(m_label);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_textEdit);
    layout->addWidget(m_listWidget);
    layout->addWidget(m_buttons);
    setLayout(layout);
}

void QLightBoxInputDialog::initConnections()
{
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QLightBoxInputDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QLightBoxInputDialog::reject);
}

QWidget* QLightBoxInputDialog::focusedOnExec() const
{
    QWidget* focusTarget = m_buttons;
    if (m_lineEdit->property(::focusProperty).toBool()) {
        focusTarget = m_lineEdit;
    } else if (m_textEdit->property(::focusProperty).toBool()) {
        focusTarget = m_textEdit;
    }

    return focusTarget;
}


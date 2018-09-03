#include "ScriptVersionsList.h"
#include "ScriptVersionWidget.h"

#include <Domain/ScriptVersion.h>

#include <3rd_party/Helpers/TextUtils.h>

#include <QAbstractItemModel>
#include <QVBoxLayout>

using Domain::ScriptVersionsTable;
using UserInterface::ScriptVersionsList;
using UserInterface::ScriptVersionWidget;


ScriptVersionsList::ScriptVersionsList(QWidget* _parent)
    : QScrollArea(_parent)
{
    initView();
}

void ScriptVersionsList::setModel(QAbstractItemModel* _model)
{
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widget()->layout());

    //
    // Стираем старый список версий
    //
    while (layout->count() > 0) {
        QLayoutItem* item = layout->takeAt(0);
        if (item != nullptr
            && item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
    }

    //
    // Отключаем старую модель
    //
    if (m_model != nullptr) {
        m_model->disconnect(this);
    }

    //
    // Сохраняем новую модель
    //
    m_model = _model;

    //
    // Строим новый список версий
    //
    if (m_model != nullptr) {
        for (int row = m_model->rowCount() - 1; row >= 0; --row) {
            const bool isFirstDraft = row == 0;
            ScriptVersionWidget* version = new ScriptVersionWidget(isFirstDraft);
            const QString versionName = m_model->index(row, ScriptVersionsTable::kName).data().toString();
            const QString versionDateTime = m_model->index(row, ScriptVersionsTable::kDatetime).data().toDateTime().toString("dd.MM.yyyy hh:mm:ss");
            const QString versionUser = m_model->index(row, ScriptVersionsTable::kUsername).data().toString();
            if (isFirstDraft) {
                version->setTitle(versionName);
            } else {
                version->setTitle(QString("%1 %2 %3 %4")
                                  .arg(versionName)
                                  .arg(TextUtils::directedText(versionDateTime, '[', ']'))
                                  .arg(tr("started by"))
                                  .arg(versionUser));
            }
            const QString versionDescription = m_model->index(row, ScriptVersionsTable::kDescription).data().toString();
            version->setDescription(versionDescription);
            const QColor versionColor = m_model->index(row, ScriptVersionsTable::kColor).data().value<QColor>();
            version->setColor(versionColor);
            //
            connect(version, &ScriptVersionWidget::removeClicked, this, &ScriptVersionsList::handleRemoveClick);

            layout->addWidget(version);
        }

        layout->addStretch(1);

        //
        // FIXME: сделать нормальное управление изменениями модели
        //
        connect(m_model, &QAbstractItemModel::rowsInserted, this, [this] { setModel(m_model); });
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, [this] { setModel(m_model); });
    }
}

void ScriptVersionsList::initView()
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    QWidget* content = new QWidget;
    content->setObjectName("ProjectListsContent");
    content->setLayout(layout);

    setWidget(content);
    setWidgetResizable(true);
}

int ScriptVersionsList::versionRow(ScriptVersionWidget* _version) const
{
    //
    // Инвертируем индекс, т.к. на экране мы отображаем от новых к старым
    //
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widget()->layout());
    return layout->count() - layout->indexOf(_version) - 2; // Отнимаем два т.к. индексы с 0 + одна позиция на спейсер
}

void ScriptVersionsList::handleRemoveClick()
{
    if (ScriptVersionWidget* version = qobject_cast<ScriptVersionWidget*>(sender())) {
        emit removeRequested(m_model->index(versionRow(version), 0));
    }
}

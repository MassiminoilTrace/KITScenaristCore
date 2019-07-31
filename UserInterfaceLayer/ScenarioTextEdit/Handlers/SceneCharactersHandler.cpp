#include "SceneCharactersHandler.h"

#include "../ScenarioTextEdit.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockParsers.h>

#include <Domain/Research.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/ResearchStorage.h>

#include <3rd_party/Helpers/TextEditHelper.h>

#include <QKeyEvent>
#include <QRegularExpression>
#include <QStringListModel>
#include <QTextBlock>

using namespace KeyProcessingLayer;
using namespace DataStorageLayer;
using namespace BusinessLogic;
using UserInterface::ScenarioTextEdit;

SceneCharactersHandler::SceneCharactersHandler(ScenarioTextEdit* _editor) :
	StandardKeyHandler(_editor),
	m_filteredCharactersModel(new QStringListModel(_editor))
{
}

void SceneCharactersHandler::handleEnter(QKeyEvent*)
{
	//
	// Получим необходимые значения
	//
	// ... курсор в текущем положении
	QTextCursor cursor = editor()->textCursor();
	// ... блок текста в котором находится курсор
	QTextBlock currentBlock = cursor.block();
	// ... текст до курсора
	QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
	// ... текст после курсора
	QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());
	// ... префикс и постфикс стиля
	ScenarioBlockStyle style = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::SceneCharacters);
	QString stylePrefix = style.prefix();
	QString stylePostfix = style.postfix();

	//
	// Обработка
	//
	if (editor()->isCompleterVisible()) {
		//! Если открыт подстановщик

		//
		// Вставить выбранный вариант
		//
		editor()->applyCompletion();
	} else {
		//! Подстановщик закрыт

		if (cursor.hasSelection()) {
			//! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addScenarioBlock(ScenarioBlockStyle::SceneCharacters);
		} else {
			//! Нет выделения

			if ((cursorBackwardText.isEmpty() && cursorForwardText.isEmpty())
				|| (cursorBackwardText + cursorForwardText == stylePrefix + stylePostfix)) {
				//! Текст пуст

				//
				// Cменить стиль на описание действия
				//
				editor()->changeScenarioBlockType(changeForEnter(ScenarioBlockStyle::SceneCharacters));
			} else {
				//! Текст не пуст

				//
				// Сохраним персонажей
				//
				storeCharacters();

				if (cursorBackwardText.isEmpty()
					|| cursorBackwardText == stylePrefix) {
					//! В начале блока

					//
					// Ни чего не делаем
					//
				} else if (cursorForwardText.isEmpty()
						   || cursorForwardText == stylePostfix) {
					//! В конце блока

					//
					// Перейдём к блоку действия
					//
					cursor.movePosition(QTextCursor::EndOfBlock);
					editor()->setTextCursor(cursor);
					editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::SceneCharacters));
				} else {
					//! Внутри блока

					//
					// Переместим обрамление в правильное место
					//
					cursor.movePosition(QTextCursor::EndOfBlock);
					if (cursorForwardText.endsWith(stylePostfix)) {
						for (int deleteReplays = stylePostfix.length(); deleteReplays > 0; --deleteReplays) {
							cursor.deletePreviousChar();
						}
					}
					cursor = editor()->textCursor();
					cursor.insertText(stylePostfix);

					//
					// Перейдём к блоку действия
					//
					editor()->setTextCursor(cursor);
					editor()->addScenarioBlock(ScenarioBlockStyle::Action);
				}
			}
		}
	}
}

void SceneCharactersHandler::handleTab(QKeyEvent* _event)
{
	handleEnter(_event);
}

void SceneCharactersHandler::handleOther(QKeyEvent*)
{
	//
	// Получим необходимые значения
	//
	// ... курсор в текущем положении
	QTextCursor cursor = editor()->textCursor();
	// ... блок текста в котором находится курсор
	QTextBlock currentBlock = cursor.block();
	// ... текст блока
	QString currentBlockText = currentBlock.text();
	// ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void SceneCharactersHandler::handleInput(QInputMethodEvent* _event)
{
#ifndef Q_OS_ANDROID
    Q_UNUSED(_event)
#endif
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    const QTextCursor cursor = editor()->textCursor();
    int cursorPosition = cursor.positionInBlock();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
#ifdef Q_OS_ANDROID
    QString stringForInsert;
    if (!_event->preeditString().isEmpty()) {
        stringForInsert = _event->preeditString();
    } else {
        stringForInsert = _event->commitString();
    }
    currentBlockText.insert(cursorPosition, stringForInsert);
    cursorPosition += stringForInsert.length();
#endif
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursorPosition);

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void SceneCharactersHandler::complete(const QString& _currentBlockText, const QString& _cursorBackwardText)
{
    QString cursorBackwardTextToComma = _cursorBackwardText;
    if (!cursorBackwardTextToComma.split(", ").isEmpty()) {
        cursorBackwardTextToComma = cursorBackwardTextToComma.split(", ").last();
    }
    // ... уберём префикс
    ScenarioBlockStyle style = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::SceneCharacters);
    QString stylePrefix = style.prefix();
    if (!stylePrefix.isEmpty()
        && cursorBackwardTextToComma.startsWith(stylePrefix)) {
        cursorBackwardTextToComma.remove(QRegularExpression(QString("^[%1]").arg(stylePrefix)));
    }

    //
    // Получим модель подсказок для текущей секции и выведем пользователю
    //
    QAbstractItemModel* characterModel = StorageFacade::researchStorage()->characters();

    //
    // Убрать из модели уже использованные элементы
    //
    // ... сформируем список уже введённых персонажей
    //
    QStringList enteredCharacters = TextEditHelper::smartToUpper(_currentBlockText).split(", ");
    enteredCharacters.removeOne(cursorBackwardTextToComma);
    //
    // ... скорректируем модель
    //
    QStringList filteredCharacters;
    for (int row = 0; row < characterModel->rowCount(); ++row) {
        const QString characterName =
                TextEditHelper::smartToUpper(characterModel->data(characterModel->index(row, 0)).toString());
        if (!enteredCharacters.contains(characterName)) {
            filteredCharacters.append(characterName);
        }
    }
    m_filteredCharactersModel->setStringList(filteredCharacters);

    //
    // Дополним текст
    //
    editor()->complete(m_filteredCharactersModel, cursorBackwardTextToComma);
}

void SceneCharactersHandler::storeCharacters() const
{
	if (editor()->storeDataWhenEditing()) {
		//
		// Получим необходимые значения
		//
		// ... курсор в текущем положении
		QTextCursor cursor = editor()->textCursor();
		// ... блок текста в котором находится курсор
		QTextBlock currentBlock = cursor.block();
		// ... текст блока
		QString currentBlockText = currentBlock.text();
		// ... персонажы
		QStringList enteredCharacters = SceneCharactersParser::characters(currentBlockText);

		foreach (const QString& character, enteredCharacters) {
			//
			// Сохраняем персонажа
			//
            StorageFacade::researchStorage()->storeCharacter(character);
		}
	}
}

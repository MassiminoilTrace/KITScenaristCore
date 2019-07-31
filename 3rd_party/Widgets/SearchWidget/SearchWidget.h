#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QFrame>

class PageTextEdit;
class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;


/**
 * @brief Виджет - поисковая строка для текстового документа
 */
class SearchWidget : public QFrame
{
	Q_OBJECT

public:
    explicit SearchWidget(QWidget* _parent = 0, bool _showTypesCombo = false);

	/**
	 * @brief Установить документ для поиска
	 */
	void setEditor(PageTextEdit* _editor);

	/**
	 * @brief Выделить искомый текст
	 * @note Применяется при повторном показе, чтобы можно было сразу вводить новый текст
	 */
	void selectText();

	/**
	 * @brief Установить режим только поиска или и замены тоже
	 */
	void setSearchOnly(bool _isSearchOnly);

private slots:
	/**
	 * @brief Перейти к следующему совпадению
	 */
	void aboutFindNext();

	/**
	 * @brief Перейти к предыдущему совпадению
	 */
	void aboutFindPrev();

	/**
	 * @brief Заменить текущий и перейти к следующему
	 */
	void aboutReplaceOne();

	/**
	 * @brief Заменить все
	 */
	void aboutReplaceAll();

private:
    /**
     * @brief Настроить представление
     */
    void initView(bool _showTypesCombo);

    /**
     * @brief Настроить внешний вид
     */
    void initStyleSheet();

    /**
     * @brief Настроить соединения
     */
    void initConnections();

	/**
	 * @brief Поиск текста в заданном направлении
	 */
	void findText(bool _backward);

private:
	/**
	 * @brief Редактор текста, в котором производится поиск
	 */
	PageTextEdit* m_editor;

	/**
	 * @brief Поле для ввода искомого текста
	 */
	QLineEdit* m_searchText;

	/**
	 * @brief Искать с учётом регистра или без
	 */
	QPushButton* m_caseSensitive;

	/**
	 * @brief Перейти к предыдущему совпадению
	 */
	QPushButton* m_prevMatch;

	/**
	 * @brief Перейти к следующему совпадению
	 */
	QPushButton* m_nextMatch;

	/**
	 * @brief Поле для ввода текста замены
	 */
	QLineEdit* m_replaceText;

	/**
	 * @brief Кнопка замены текущего совпадение и перехода к следующему
	 */
	QPushButton* m_replaceOne;

	/**
	 * @brief Кнопка замены всех совпадений
	 */
	QPushButton* m_replaceAll;

    /**
     * @brief В каких блоках искать/заменять
     */
    QComboBox* m_searchIn;

	/**
	 * @brief Последний искомый текст
	 */
	QString m_lastSearchText;
};

#endif // SEARCHWIDGET_H

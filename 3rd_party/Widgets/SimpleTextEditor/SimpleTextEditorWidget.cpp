#include "SimpleTextEditorWidget.h"
#include "SimpleTextEditor.h"

#include <3rd_party/Helpers/ShortcutHelper.h>
#include <3rd_party/Widgets/ColoredToolButton/ColoredToolButton.h>
#include <3rd_party/Widgets/FlatButton/FlatButton.h>
#include <3rd_party/Widgets/ScalableWrapper/ScalableWrapper.h>

#include <QApplication>
#include <QComboBox>
#include <QFontDatabase>
#include <QLabel>
#include <QSettings>
#include <QStringListModel>
#include <QVBoxLayout>

namespace {
    /**
     * @brief Список всех виджетов редакторов
     */
    static QList<SimpleTextEditorWidget*> s_editorWidgets;
}


void SimpleTextEditorWidget::enableSpellCheck(bool _enable, SpellChecker::Language _language)
{
    //
    // Для каждого редактора
    //
    for (SimpleTextEditorWidget* editorWidget : s_editorWidgets) {
        editorWidget->m_editor->setUseSpellChecker(_enable);
        editorWidget->m_editor->setSpellCheckLanguage(_language);
        editorWidget->m_editor->setHighlighterDocument(editorWidget->m_editor->document());
    }
}

SimpleTextEditorWidget::SimpleTextEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_editor(new SimpleTextEditor(this)),
    m_editorWrapper(new ScalableWrapper(m_editor, this)),
    m_toolbar(new QWidget(this)),
    m_undo(new FlatButton(this)),
    m_redo(new FlatButton(this)),
    m_textFont(new QComboBox(this)),
    m_textFontSize(new QComboBox(this)),
    m_textBold(new FlatButton(this)),
    m_textItalic(new FlatButton(this)),
    m_textUnderline(new FlatButton(this)),
    m_textColor(new ColoredToolButton(QIcon(":/Graphics/Iconset/format-color-text.svg"), this)),
    m_textBackgroundColor(new ColoredToolButton(QIcon(":/Graphics/Iconset/format-color-fill.svg"), this)),
    m_clearFormatting(new FlatButton(this)),
    m_toolbarSpace(new QLabel(this)),
    m_isInTextFormatUpdate(false)
{
    initView();
    initConnections();
    initStyleSheet();

    //
    // При создании нового, попробуем настроить его в соответствии со всеми уже настроенными редакторами
    //
    for (SimpleTextEditorWidget* editorWidget : s_editorWidgets) {
        if (editorWidget != this) {
            m_editor->setUseSpellChecker(editorWidget->m_editor->useSpellChecker());
            m_editor->setSpellCheckLanguage(editorWidget->m_editor->spellCheckLanguage());
            m_editor->setHighlighterDocument(m_editor->document());
            break;
        }
    }
    s_editorWidgets.append(this);

    //
    // Установить шрифт умолчальный для системы
    //
    QFont defaultFont;
#ifdef Q_OS_WIN
    defaultFont.setFamily("Calibri");
    defaultFont.setPointSize(11);
#elif defined(Q_OS_MAC)
    defaultFont.setFamily("Helvetica");
    defaultFont.setPointSize(12);
#else
    defaultFont.setPointSize(12);
#endif
    setFont(defaultFont);
}

SimpleTextEditorWidget::~SimpleTextEditorWidget()
{
    s_editorWidgets.removeAll(this);
}

void SimpleTextEditorWidget::setToolbarVisible(bool _visible)
{
    m_toolbar->setVisible(_visible);
}

void SimpleTextEditorWidget::setReadOnly(bool _readOnly)
{
    m_toolbar->setEnabled(!_readOnly);
    m_editor->setReadOnly(_readOnly);
}

void SimpleTextEditorWidget::setUsePageMode(bool _use)
{
    m_editor->setUsePageMode(_use);
    m_editor->setPageMargins(_use ? QMarginsF(20, 20, 20, 20) : QMarginsF(2, 2, 2, 2));
}

void SimpleTextEditorWidget::setPageSettings(QPageSize::PageSizeId _pageSize, const QMarginsF& _margins, Qt::Alignment _numberingAlign)
{
    m_editor->setPageFormat(_pageSize);
    m_editor->setPageMargins(_margins);
    m_editor->setPageNumbersAlignment(_numberingAlign);
}

void SimpleTextEditorWidget::setDefaultFont(const QFont& _font)
{
    m_textFont->setCurrentText(_font.family());
    m_textFontSize->setCurrentText(QString::number(_font.pointSize()));
    m_editor->document()->setDefaultFont(_font);
    m_editor->setTextFont(_font);
}

bool SimpleTextEditorWidget::isEmpty() const
{
    return m_editor->document()->isEmpty();
}

QString SimpleTextEditorWidget::toHtml() const
{
    if (isEmpty()) {
        return QString();
    }

    return m_editor->toHtml();
}

QString SimpleTextEditorWidget::toPlainText() const
{
    if (isEmpty()) {
        return QString();
    }

    return m_editor->toPlainText();
}

void SimpleTextEditorWidget::setHtml(const QString& _html)
{
    m_editor->setHtml(_html);
}

void SimpleTextEditorWidget::setPlainText(const QString& _text)
{
    m_editor->setPlainText(_text);
}

void SimpleTextEditorWidget::clear()
{
    m_editor->clear();
}

SimpleTextEditor* SimpleTextEditorWidget::editor() const
{
    return m_editor;
}

void SimpleTextEditorWidget::initView()
{
    setFocusProxy(m_editorWrapper);

    //
    // Обновить масштаб
    //
    QSettings settings;
    m_editorWrapper->setZoomRange(settings.value("simple-editor/zoom-range", 0).toDouble());
    m_editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    m_undo->setShortcut(QKeySequence::Undo);
    m_undo->setIcons(QIcon(":/Graphics/Iconset/undo.svg"));
    m_undo->setToolTip(ShortcutHelper::makeToolTip(tr("Undo last action"), m_undo->shortcut()));
    m_redo->setShortcut(QKeySequence::Redo);
    m_redo->setIcons(QIcon(":/Graphics/Iconset/redo.svg"));
    m_redo->setToolTip(ShortcutHelper::makeToolTip(tr("Redo previously undone action"), m_redo->shortcut()));
    m_textFont->setModel(new QStringListModel(QFontDatabase().families(), m_textFont));
    m_textFont->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_textFont->setEditable(true);
    m_textFontSize->setModel(new QStringListModel({"8","9","10","11","12","14","18","24","30","36","48","60","72","96"}, m_textFontSize));
    m_textFontSize->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_textFontSize->setEditable(true);
    m_textBold->setCheckable(true);
    m_textBold->setShortcut(QKeySequence::Bold);
    m_textBold->setIcons(QIcon(":/Graphics/Iconset/format-bold.svg"));
    m_textBold->setToolTip(ShortcutHelper::makeToolTip(tr("Make text bold"), m_textBold->shortcut()));
    m_textItalic->setCheckable(true);
    m_textItalic->setShortcut(QKeySequence::Italic);
    m_textItalic->setIcons(QIcon(":/Graphics/Iconset/format-italic.svg"));
    m_textItalic->setToolTip(ShortcutHelper::makeToolTip(tr("Make text italic"), m_textItalic->shortcut()));
    m_textUnderline->setCheckable(true);
    m_textUnderline->setShortcut(QKeySequence::Underline);
    m_textUnderline->setIcons(QIcon(":/Graphics/Iconset/format-underline.svg"));
    m_textUnderline->setToolTip(ShortcutHelper::makeToolTip(tr("Make text underline"), m_textUnderline->shortcut()));
    m_textColor->setIconSize(QSize(20, 20));
    m_textColor->setColorsPane(ColoredToolButton::Google);
    m_textColor->setToolTip(tr("Change text color"));
    m_textBackgroundColor->setIconSize(QSize(20, 20));
    m_textBackgroundColor->setColorsPane(ColoredToolButton::Google);
    m_textBackgroundColor->setToolTip(tr("Change text background color"));
    m_clearFormatting->setIcons(QIcon(":/Graphics/Iconset/format-clear.svg"));
    m_clearFormatting->setToolTip(tr("Clear formatting"));
    m_toolbarSpace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    //
    // Настроим панель инструментов
    //
    QHBoxLayout* toolbarLayout = new QHBoxLayout;
    toolbarLayout->setSpacing(0);
    toolbarLayout->setContentsMargins(QMargins());
    toolbarLayout->addWidget(m_undo);
    toolbarLayout->addWidget(m_redo);
    toolbarLayout->addWidget(m_textFont);
    toolbarLayout->addWidget(m_textFontSize);
    toolbarLayout->addWidget(m_textBold);
    toolbarLayout->addWidget(m_textItalic);
    toolbarLayout->addWidget(m_textUnderline);
    toolbarLayout->addWidget(m_textColor);
    toolbarLayout->addWidget(m_textBackgroundColor);
    toolbarLayout->addWidget(m_clearFormatting);
    toolbarLayout->addWidget(m_toolbarSpace);
    m_toolbar->setLayout(toolbarLayout);
    //
    // Настроим осноную компоновку виджета
    //
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins());
    layout->addWidget(m_toolbar);
    layout->addWidget(m_editorWrapper);
    setLayout(layout);
}

void SimpleTextEditorWidget::initConnections()
{
    connect(m_editor, &SimpleTextEditor::textChanged, this, &SimpleTextEditorWidget::textChanged);

    connect(m_editorWrapper, &ScalableWrapper::zoomRangeChanged,
        [=] (const qreal _zoomRange) {
        //
        // Сохранить значение масштаба
        //
        QSettings settings;
        if (settings.value("simple-editor/zoom-range").toDouble() != _zoomRange) {
            settings.setValue("simple-editor/zoom-range", _zoomRange);
        }

        //
        // Обновить значение для всех простых редакторов
        //
        for (SimpleTextEditorWidget* editorWidget : s_editorWidgets) {
            editorWidget->m_editorWrapper->setZoomRange(_zoomRange);
        }
    });

    //
    // Панель инструментов
    //
    connect(m_editor, &SimpleTextEditor::currentCharFormatChanged,
            [this] (const QTextCharFormat& _format) {
        m_isInTextFormatUpdate = true;
        const QFont font = _format.font();
        m_textFont->setCurrentText(font.family());
        m_textFontSize->setCurrentText(QString::number(font.pointSize()));
        m_textBold->setChecked(font.bold());
        m_textItalic->setChecked(font.italic());
        m_textUnderline->setChecked(font.underline());
        QColor textColor = palette().text().color();
        if (_format.hasProperty(QTextFormat::ForegroundBrush)) {
            textColor = _format.foreground().color();
        }
        m_textColor->setColor(textColor);
        QColor textBackgroundColor = palette().text().color();
        if (_format.hasProperty(QTextFormat::BackgroundBrush)) {
            textBackgroundColor = _format.background().color();
        }
        m_textBackgroundColor->setColor(textBackgroundColor);
        m_isInTextFormatUpdate = false;
    });
    //
    // ... отмена/повтор последнего действия
    //
    connect(m_undo, &FlatButton::clicked, m_editor, &SimpleTextEditor::undo);
    connect(m_redo, &FlatButton::clicked, m_editor, &SimpleTextEditor::redo);
    //
    // ... шрифт
    //
    connect(m_textFont, &QComboBox::currentTextChanged, this, [this] {
        if (!m_isInTextFormatUpdate) {
            QFont font(m_textFont->currentText(), m_textFontSize->currentText().toInt());
            m_editor->setTextFont(font);
        }
    });
    //
    // ... размер шрифта
    //
    connect(m_textFontSize, &QComboBox::currentTextChanged, this, [this] {
        if (!m_isInTextFormatUpdate) {
            QFont font(m_textFont->currentText(), m_textFontSize->currentText().toInt());
            m_editor->setTextFont(font);
        }
    });
    //
    // ... начертания
    //
    connect(m_textBold, &FlatButton::toggled, this, [this] (bool _checked) {
        if (!m_isInTextFormatUpdate) {
            m_editor->setTextBold(_checked);
        }
    });
    connect(m_textItalic, &FlatButton::toggled, this, [this] (bool _checked) {
        if (!m_isInTextFormatUpdate) {
            m_editor->setTextItalic(_checked);
        }
    });
    connect(m_textUnderline, &FlatButton::toggled, this, [this] (bool _checked) {
        if (!m_isInTextFormatUpdate) {
            m_editor->setTextUnderline(_checked);
        }
    });
    //
    // ... цвет текста
    //
    connect(m_textColor, &ColoredToolButton::clicked, this, [this] (const QColor& _color) {
        if (!m_isInTextFormatUpdate) {
            m_editor->setTextColor(_color);
        }
    });
    //
    // ... цвет фона текста
    //
    connect(m_textBackgroundColor, &ColoredToolButton::clicked, this, [this] (const QColor& _color) {
        if (!m_isInTextFormatUpdate) {
            m_editor->setTextBackgroundColor(_color);
        }
    });
    //
    // ... очистить форматирование
    //
    connect(m_clearFormatting, &FlatButton::clicked, this, [this] {
        if (!m_isInTextFormatUpdate) {
            m_editor->clearFormat();
        }
    });
}

void SimpleTextEditorWidget::initStyleSheet()
{
    m_undo->setProperty("inTopPanel", true);
    m_undo->setProperty("topPanelLeftBordered", true);
    m_redo->setProperty("inTopPanel", true);
    m_textFont->setProperty("inTopPanel", true);
    m_textFont->setProperty("topPanelTopBordered", true);
    m_textFont->setProperty("topPanelLeftBordered", true);
    m_textFont->setProperty("topPanelRightBordered", true);
    m_textFontSize->setProperty("inTopPanel", true);
    m_textFontSize->setProperty("topPanelTopBordered", true);
    m_textFontSize->setProperty("topPanelRightBordered", true);
    m_textBold->setProperty("inTopPanel", true);
    m_textItalic->setProperty("inTopPanel", true);
    m_textUnderline->setProperty("inTopPanel", true);
    m_textColor->setProperty("inTopPanel", true);
    m_textBackgroundColor->setProperty("inTopPanel", true);
    m_clearFormatting->setProperty("inTopPanel", true);
    m_toolbarSpace->setProperty("inTopPanel", true);
    m_toolbarSpace->setProperty("topPanelTopBordered", true);
    m_toolbarSpace->setProperty("topPanelRightBordered", true);
}

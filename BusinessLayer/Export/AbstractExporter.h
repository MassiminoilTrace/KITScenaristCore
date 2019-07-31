#ifndef ABSTRACTEXPORTER_H
#define ABSTRACTEXPORTER_H

#include <QString>

class QTextDocument;

namespace BusinessLogic
{
    class ResearchModelCheckableProxy;
    class ScenarioDocument;


    /**
     * @brief Параметры экспорта
     */
    class ExportParameters
    {
    public:
        /**
         * @brief Экспортировать разработку или текст
         */
        bool isResearch = false;

        /**
         * @brief Необходимо ли экспортировать описания сцен
         */
        bool isOutline = false;

        /**
         * @brief Необходимо ли экспортировать текст сценария
         */
        bool isScript = true;

        /**
         * @brief Путь к файлу
         */
        QString filePath;

        /**
         * @brief Проверять ли переносы страниц
         */
        bool checkPageBreaks = false;

        /**
         * @brief Название стиля экспорта
         */
        QString style;

        /**
         * @brief Печатать титульную страницу
         */
        bool printTilte = true;

        /**
         * @brief Информация с титульного листа
         */
        /** @{ */
        QString scriptName;
        QString scriptAdditionalInfo;
        QString scriptGenre;
        QString scriptAuthor;
        QString scriptContacts;
        QString scriptYear;
        /** @} */

        /**
         * @brief Колонтитулы
         */
        /** @{ */
        QString scriptHeader;
        QString scriptFooter;
        /** @} */

        /**
         * @brief Логлайн
         */
        QString logline;

        /**
         * @brief Синопсис
         */
        QString synopsis;

        /**
         * @brief Печатать номера страниц
         */
        bool printPagesNumbers = true;

        /**
         * @brief Печатать номера сцен
         */
        bool printScenesNumbers = true;

        /**
         * @brief Печатать номера реплик
         */
        bool printDialoguesNumbers = false;

        /**
         * @brief Приставка сцен
         */
        QString scenesPrefix;

        /**
         * @brief Сохранять редакторские пометки
         */
        bool saveReviewMarks = true;

        /**
         * @brief Сохранять ли непечатаемые комментарии
         */
        bool printInvisible = false;

        /**
         * @brief Печатать водяной знак
         */
        bool printWatermark = false;

        /**
         * @brief Водяной знак
         */
        QString watermark;
    };


    /**
     * @brief Базовый класс экспортера
     */
    class AbstractExporter
    {
    public:
        virtual ~AbstractExporter() {}

        /**
         * @brief Экспорт заданного документа в файл
         */
        virtual void exportTo(ScenarioDocument* _scenario, const ExportParameters& _exportParameters) const = 0;

        /**
         * @brief Экспорт заданной модели разработки с указанными параметрами
         */
        virtual void exportTo(const ResearchModelCheckableProxy* _researchModel,
                              const ExportParameters& _exportParameters) const  = 0;

    protected:
        /**
         * @brief Сформировать из сценария документ, готовый для экспорта
         * @note Вызывающий получает владение над новым сформированным документом
         */
        static QTextDocument* prepareDocument(const ScenarioDocument* _scenario,
            const ExportParameters& _exportParameters);

        /**
         * @brief Сформировать из разработки документ, готовый для экспорта
         * @note Вызывающий получает владение над новым сформированным документом
         */
        static QTextDocument* prepareDocument(const ResearchModelCheckableProxy* _researchModel,
            const ExportParameters& _exportParameters);

    private:
        /**
         * @brief Подготовить документ, для формирования
         * @note Вызывающий получает владение над новым сформированным документом
         */
        static QTextDocument* prepareDocument();
    };
}

#endif // ABSTRACTEXPORTER_H

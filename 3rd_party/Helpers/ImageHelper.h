#ifndef IMAGEHELPER
#define IMAGEHELPER

#include <QByteArray>
#include <QBuffer>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace {
    /**
     * @brief Максимальные размеры изображения
     */
    /** @{ */
    const int kImageMaxWidth = 1600;
    const int kImageMaxHeight = 1200;
    /** @} */

    /**
     * @brief Используем низкое качество изображения (всё-таки у нас приложение не для фотографов)
     */
    const int kImageFileQuality = 40;

    /**
     * @brief Умолчальный размер для svg-изображения
     */
    const QSize kDefaultSvgIconSize{1024, 1024};
}


/**
 * @brief Вспомогательные функции для работы с изображениями
 */
class ImageHelper
{
public:
    /**
     * @brief Сохранение изображения в массив байт
     */
    static QByteArray bytesFromImage(const QPixmap& _image) {
        //
        // Если необходимо корректируем размер изображения
        //
        QPixmap imageScaled = _image;
        if (imageScaled.width() > kImageMaxWidth
            || imageScaled.height() > kImageMaxHeight) {
            imageScaled = imageScaled.scaled(kImageMaxWidth, kImageMaxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        //
        // Сохраняем изображение
        //
        QByteArray imageData;
        QBuffer imageDataBuffer(&imageData);
        imageDataBuffer.open(QIODevice::WriteOnly);
        const char* imageFormat = imageScaled.hasAlpha() ? "PNG" : "JPG";
        imageScaled.save(&imageDataBuffer, imageFormat, kImageFileQuality);
        return imageData;
    }

    /**
     * @brief Загрузить изображение из массива байт
     */
    static QPixmap imageFromBytes(const QByteArray& _bytes) {
        QPixmap image;
        image.loadFromData(_bytes);
        return image;
    }

    /**
     * @brief Установить цвет иконки
     */
    static void setIconColor(QIcon& _icon, const QSize& _iconSize, const QColor& _color) {
        if (!_icon.isNull() && _iconSize.isValid() && _color.isValid()) {
            QPixmap baseIconPixmap = _icon.pixmap(_iconSize);
            QPixmap newIconPixmap = baseIconPixmap;

            QPainter painter(&newIconPixmap);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(newIconPixmap.rect(), _color);
            painter.end();

            _icon = QIcon(newIconPixmap);
        }
    }

    /**
     * @brief Установить цвет иконки используя максимальный размер иконки
     */
    static void setIconColor(QIcon& _icon, const QColor& _color) {
        if (!_icon.isNull() && !_icon.availableSizes().isEmpty() && _color.isValid()) {
            setIconColor(_icon, _icon.availableSizes().last(), _color);
        } else {
            setIconColor(_icon, kDefaultSvgIconSize, _color);
        }
    }

    /**
     * @brief Сравнить два изображения
     */
    static bool isImagesEqual(const QPixmap& _lhs, const QPixmap& _rhs) {
        return bytesFromImage(_lhs) == bytesFromImage(_rhs);
    }

    /**
     * @brief Сравнить два списка изображений
     */
    static bool isImageListsEqual(const QList<QPixmap>& _lhs, const QList<QPixmap>& _rhs) {
        QList<QByteArray> lhs, rhs;
        foreach (const QPixmap& pixmap, _lhs) {
            lhs.append(bytesFromImage(pixmap));
        }
        foreach (const QPixmap& pixmap, _rhs) {
            rhs.append(bytesFromImage(pixmap));
        }
        return lhs == rhs;
    }
};

#endif // IMAGEHELPER


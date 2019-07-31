#include "ProjectsManager.h"

#include <DataLayer/Database/Database.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Helpers/TextEditHelper.h>

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QXmlStreamReader>

using ManagementLayer::ProjectsManager;
using ManagementLayer::Project;

namespace {
    /**
     * @brief Ключи для доступа к спискам проектов
     */
    const QString RECENT_FILES_LIST_SETTINGS_KEY = "application/recent-files/list";
    const QString RECENT_FILES_USING_SETTINGS_KEY = "application/recent-files/using";

    /**
     * @brief kit scenarist project
     */
    const QString kProjectFleExtension = ".dps";
}


bool ProjectsManager::isCurrentProjectValid()
{
    return s_currentProject.type() != Project::Invalid;
}

const ManagementLayer::Project& ProjectsManager::currentProject()
{
    return s_currentProject;
}

QString ProjectsManager::defaultLocation()
{
#ifdef MOBILE_OS
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
}

Project ProjectsManager::s_currentProject;


// ********


ProjectsManager::ProjectsManager(QObject* _parent) :
    QObject(_parent),
    m_recentProjectsModel(new QStandardItemModel(this)),
    m_remoteProjectsModel(new QStandardItemModel(this))
{
#ifdef Q_OS_IOS
    makeRelativePaths();
#endif
    loadRecentProjects();
    refreshProjects();
}

ProjectsManager::~ProjectsManager()
{
    saveRecentProjects();
}

QAbstractItemModel* ProjectsManager::recentProjects()
{
    m_recentProjectsModel->clear();
    for (const Project& project : m_recentProjects) {
        QStandardItem* item = new QStandardItem;
        item->setData(project.displayName(), Qt::DisplayRole);
        item->setData(project.displayPath(), Qt::StatusTipRole);
        item->setData(project.lastEditDatetime(), Qt::WhatsThisRole);
        item->setData(true, Qt::UserRole + 1);
        m_recentProjectsModel->appendRow(item);
    }

    return m_recentProjectsModel;
}

QAbstractItemModel* ProjectsManager::remoteProjects()
{
    m_remoteProjectsModel->clear();
    for (const Project& project : m_remoteProjects) {
        QStandardItem* item = new QStandardItem;
        item->setData(project.displayName(), Qt::DisplayRole);
        item->setData(project.displayPath(), Qt::StatusTipRole);
        item->setData(project.lastEditDatetime(), Qt::WhatsThisRole);
        item->setData(project.users(), Qt::UserRole);
        item->setData(project.isUserOwner(), Qt::UserRole + 1);
        m_remoteProjectsModel->appendRow(item);
    }

    return m_remoteProjectsModel;
}

bool ProjectsManager::setCurrentProject(const QString& _path, bool _isLocal, bool _forceOpen)
{
    //
    // Приведём путь к нативному виду
    //
    const QString projectPath = QDir::toNativeSeparators(_path);

    //
    // Проверяем можем ли мы открыть файл проекта
    //
    const bool canOpen = _forceOpen ? true : DatabaseLayer::Database::canOpenFile(projectPath, _isLocal);
    if (canOpen) {
        //
        // Делаем проект текущим и загружаем из него БД
        // или создаём, если ранее его не существовало
        //
        DatabaseLayer::Database::setCurrentFile(projectPath);

        Project newCurrentProject;
        //
        // Для локальных файлов делаем обработку списка недавно используемых
        //
        if (_isLocal) {
            //
            // Проверяем находится ли проект в списке недавно используемых
            //
            foreach (const Project& project, m_recentProjects) {
                if (project.path().compare(projectPath) == 0) {
                    newCurrentProject = project;
                    break;
                }
            }

            //
            // Если проект был в списке недавних делаем его первым
            //
            if (newCurrentProject.type() != Project::Invalid) {
                m_recentProjects.removeOne(newCurrentProject);
                newCurrentProject.setLastEditDatetime(QDateTime::currentDateTime());
                m_recentProjects.prepend(newCurrentProject);
            }
            //
            // Если не был добавляем в начало списка ранее используемых
            //
            else {
                //
                // Определим название проекта
                //
                QFileInfo fileInfo(projectPath);
                QString projectName = fileInfo.completeBaseName();
                //
                // Создаём проект
                //
                newCurrentProject = Project(Project::Local, projectName, projectPath, QDateTime::currentDateTime());
                //
                // Добавляем проект в список
                //
                m_recentProjects.prepend(newCurrentProject);
                //
                // Сохраняем список проектов
                //
                saveRecentProjects();
            }

            //
            // Уведомляем об обновлении
            //
            emit recentProjectsUpdated();
        }
        //
        // Для проектов из облака просто определяем сам проект
        //
        else {
            foreach (const Project& project, m_remoteProjects) {
                if (project.path() == projectPath) {
                    newCurrentProject = project;
                    break;
                }
            }
        }

        //
        // Запоминаем проект, как текущий
        //
        s_currentProject = newCurrentProject;
    }

    return canOpen;
}

bool ProjectsManager::setCurrentProject(const QModelIndex& _index, bool _isLocal, bool _forceOpen)
{
    //
    // Определим проект
    //
    QString newCurrentProjectPath;
    if (_isLocal) {
        if (m_recentProjects.size() > _index.row()) {
            newCurrentProjectPath = m_recentProjects.at(_index.row()).path();
        }
    } else {
        if (m_remoteProjects.size() > _index.row()) {
            newCurrentProjectPath = m_remoteProjects.at(_index.row()).path();
        }
    }
    //
    // ... и установим его текущим
    //
    return setCurrentProject(newCurrentProjectPath, _isLocal, _forceOpen);
}

bool ProjectsManager::setCurrentProject(int _id, bool _isLocal, bool _forceOpen)
{
    //
    // Определим проект
    //
    QString newCurrentProjectPath;
    if (!_isLocal) {
        for (const Project& project : m_remoteProjects) {
            if (project.id() == _id) {
                newCurrentProjectPath = project.path();
                break;
            }
        }
    } else {
        Q_ASSERT_X(0, "Can't open local project by id", Q_FUNC_INFO);
    }
    //
    // ... и установим его текущим
    //
    return setCurrentProject(newCurrentProjectPath, _isLocal, _forceOpen);
}

void ProjectsManager::setCurrentProjectName(const QString& _projectName)
{
    //
    // Если имя не задано, то по умолчанию используется название файла, нам ни чего делать не надо
    //
    if (!_projectName.isEmpty()) {
        //
        // Определим источник хранения проекта
        //
        QMutableListIterator<ManagementLayer::Project> projectsIterator(s_currentProject.isLocal()
                                                                        ? m_recentProjects
                                                                        : m_remoteProjects);
        //
        // Обновим название проекта
        //
        while (projectsIterator.hasNext()) {
            Project& project = projectsIterator.next();
            if (project == s_currentProject) {
                s_currentProject.setName(_projectName);
                projectsIterator.setValue(s_currentProject);
                break;
            }
        }
        //
        // Уведомим клиентов об обновлении проекта
        //
        if (s_currentProject.isLocal()) {
            if (m_recentProjects.contains(s_currentProject)) {
                emit recentProjectNameChanged(m_recentProjects.indexOf(s_currentProject), _projectName);
            }
        } else {
            if (m_remoteProjects.contains(s_currentProject)) {
                emit remoteProjectNameChanged(m_remoteProjects.indexOf(s_currentProject), _projectName);
            }
        }
    }
}

void ProjectsManager::setCurrentProjectSyncAvailable(bool _syncAvailable, int _errorCode)
{
    //
    // Определим источник хранения проекта
    //
    QMutableListIterator<ManagementLayer::Project> projectsIterator(m_recentProjects);
    if (s_currentProject.isRemote()) {
        projectsIterator = QMutableListIterator<ManagementLayer::Project>(m_remoteProjects);
    }
    //
    // Обновим флаг доступности синхронизации
    //
    while (projectsIterator.hasNext()) {
        Project& project = projectsIterator.next();
        if (project == s_currentProject) {
            s_currentProject.setSyncAvailable(_syncAvailable, _errorCode);
            projectsIterator.setValue(s_currentProject);
            break;
        }
    }

    //
    // Уведомляем об обновлении
    //
    if (s_currentProject.isRemote()) {
        emit remoteProjectsUpdated();
    } else {
        emit recentProjectsUpdated();
    }
}

void ProjectsManager::closeCurrentProject()
{
    s_currentProject = Project();
}

ManagementLayer::Project ProjectsManager::project(const QModelIndex& _index, bool _isLocal) const
{
    return
            _isLocal
            ? m_recentProjects.value(_index.row())
            : m_remoteProjects.value(_index.row());
}

ManagementLayer::Project& ProjectsManager::project(const QModelIndex& _index, bool _isLocal)
{
    return
            _isLocal
            ? m_recentProjects[_index.row()]
            : m_remoteProjects[_index.row()];
}

void ProjectsManager::hideProjectFromLocal(const QModelIndex& _index)
{
    m_recentProjects.removeAt(_index.row());
    emit recentProjectsUpdated();
}

void ProjectsManager::refreshProjects()
{
    //
    // Обновляем локальные
    //
    {
        //
        // Удаляем все несуществующие файлы
        //
        QMutableListIterator<Project> checker(m_recentProjects);
        while (checker.hasNext()) {
            const Project& project = checker.next();
            if (!QFile::exists(project.path())) {
                checker.remove();
            }
        }
    }

    //
    // Уведомляем об обновлении
    //
    emit recentProjectsUpdated();
}

void ProjectsManager::setRemoteProjects(const QString& _xml)
{
    m_remoteProjects.clear();
    m_remoteProjectsModel->clear();

    //
    // Считываем список проектов из xml
    //
    QXmlStreamReader projectsReader(_xml);
    while (!projectsReader.atEnd()) {
        projectsReader.readNext();
        if (projectsReader.tokenType() == QXmlStreamReader::StartElement
            && projectsReader.name().toString() == "project") {
            const QString name = TextEditHelper::fromHtmlEscaped(projectsReader.attributes().value("name").toString());
            const QString path = QString();
            const QString lastEditDatetimeText = projectsReader.attributes().value("modified_at").toString();
            const QDateTime lastEditDatetime = QDateTime::fromString(lastEditDatetimeText, "yyyy-MM-dd hh:mm:ss");
            const int id = projectsReader.attributes().value("id").toInt();
            const QString owner = projectsReader.attributes().value("owner").toString();
            const QString roleText = projectsReader.attributes().value("role").toString();
            Project::Role role = Project::roleFromString(roleText);

            QStringList users;
            projectsReader.readNextStartElement();
            while (projectsReader.name().toString() != "project") {
                if (projectsReader.tokenType() == QXmlStreamReader::StartElement
                    && projectsReader.name().toString() == "user") {
                    users << projectsReader.readElementText();
                }
                projectsReader.readNextStartElement();
            }

            m_remoteProjects.append(Project(Project::Remote, name, path, lastEditDatetime, id, owner, role, users));
        }
    }

    //
    // Удаляем все старые проекты
    //
    for (const auto& fileInfo : QDir(Project::remoteProjectsDirPath()).entryInfoList(QDir::Files)) {
        bool needRemoveProject = true;
        for (const auto& project : m_remoteProjects) {
            if (project.path() == QDir::toNativeSeparators(fileInfo.absoluteFilePath())) {
                needRemoveProject = false;
                break;
            }
        }

        if (needRemoveProject) {
            QFile::remove(fileInfo.absoluteFilePath());
        }
    }

    //
    // Уведомляем об обновлении
    //
    emit remoteProjectsUpdated();
}

void ProjectsManager::setRemoteProjectsSyncAvailable(bool _syncAvailable)
{
    for (int projectIndex = 0; projectIndex < m_remoteProjects.size(); ++projectIndex) {
        if (m_remoteProjects[projectIndex].isUserOwner()) {
            m_remoteProjects[projectIndex].setSyncAvailable(_syncAvailable);
        }
    }
}

void ProjectsManager::loadRecentProjects()
{
    //
    // Загрузим список недавних файлов из настроек
    //
    QMap<QString, QString> recentFiles =
            DataStorageLayer::StorageFacade::settingsStorage()->values(
                RECENT_FILES_LIST_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );
    QMap<QString, QString> recentFilesUsing =
            DataStorageLayer::StorageFacade::settingsStorage()->values(
                RECENT_FILES_USING_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );

    m_recentProjects.clear();
    m_recentProjectsModel->clear();

    //
    // Формируем список недавно используемых проектов в порядке убывания даты изменения
    //
    QStringList usingDates = recentFilesUsing.values();
    qSort(usingDates.begin(), usingDates.end(), qGreater<QString>());
    for (const QString& usingDate : usingDates) {
        //
        // Путь к проекту
        //
        QString path = recentFilesUsing.key(usingDate);

        //
        // Название проекта
        //
        const QString name = recentFiles.value(path);

#ifdef Q_OS_IOS
        path = QDir(defaultLocation()).filePath(path);
#endif

        //
        // Сам проект
        //
        m_recentProjects.append(
            Project(Project::Local, name, path, QDateTime::fromString(usingDate, "yyyy-MM-dd hh:mm:ss")));
    }

#ifdef MOBILE_OS
    //
    // Загружаем список "потерянных проектов"
    //
    const QDir projectsDir(defaultLocation());
    for (const QFileInfo& fileInfo : projectsDir.entryInfoList()) {
        //
        // ... если такого файла проекта нет в списке недавних, значит от "потерялся"
        //
#ifdef Q_OS_IOS
        const QString filePath = fileInfo.fileName();
#else
        const QString filePath = fileInfo.absoluteFilePath();
#endif
        if (fileInfo.fileName().endsWith(kProjectFleExtension)
            && !recentFiles.contains(filePath)) {
            //
            // Путь к проекту
            //
            const QString path = fileInfo.absoluteFilePath();

            //
            // Название проекта
            //
            const QString name = fileInfo.baseName();

            //
            // Сам проект
            //
            m_recentProjects.append(Project(Project::Local, name, path, fileInfo.lastRead()));
        }
    }

#endif

    //
    // Уведомляем об обновлении
    //
    emit recentProjectsUpdated();
}

void ProjectsManager::saveRecentProjects()
{
    //
    // Формируем список недавно используемых файлов для сохранения
    //

    /*
     * @brief Недавно используемые файлы проектов
     *
     * key - путь к файлу проекта
     * value - название проекта
     */
    QMap<QString, QString> recentFiles;
    /*
     * @brief Порядок использования недавних файлов
     *
     * key - путь к файлу проекта
     * value - последнее использование
     */
    QMap<QString, QString> recentFilesUsing;

    for (const Project& project : m_recentProjects) {
#ifdef Q_OS_IOS
        const QString path = QDir(defaultLocation()).relativeFilePath(project.path());
#else
        const QString path = project.path();
#endif
        recentFiles.insert(path, project.name());
        recentFilesUsing.insert(path, project.lastEditDatetime().toString("yyyy-MM-dd hh:mm:ss"));
    }

    //
    // Сохраняем
    //
    DataStorageLayer::StorageFacade::settingsStorage()->setValues(
                recentFiles,
                RECENT_FILES_LIST_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );
    DataStorageLayer::StorageFacade::settingsStorage()->setValues(
                recentFilesUsing,
                RECENT_FILES_USING_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );
}

void ProjectsManager::makeRelativePaths()
{
    //
    // Загрузим списки проектов
    //
    QMap<QString, QString> recentFiles =
            DataStorageLayer::StorageFacade::settingsStorage()->values(
                RECENT_FILES_LIST_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );

    QMap<QString, QString> recentFilesUsing =
            DataStorageLayer::StorageFacade::settingsStorage()->values(
                RECENT_FILES_USING_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );

    for (const QString& fileName : recentFiles.keys()) {
        //
        // Если путь абсолютный
        //
        if (fileName.startsWith(QDir::separator())) {
            //
            // Получим имя файла вместе с расширением
            //
            const QString newFileName = fileName.split(QDir::separator()).back();

            //
            // И уберем оставшийся путь
            recentFiles[newFileName] = recentFiles[fileName];
            recentFilesUsing[newFileName] = recentFilesUsing[fileName];
            recentFiles.remove(fileName);
            recentFilesUsing.remove(fileName);
        }
    }

    //
    // Сохраним все, что сделали
    //
    DataStorageLayer::StorageFacade::settingsStorage()->setValues(
                recentFiles,
                RECENT_FILES_LIST_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );
    DataStorageLayer::StorageFacade::settingsStorage()->setValues(
                recentFilesUsing,
                RECENT_FILES_USING_SETTINGS_KEY,
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );
}

#include "ScenarioChangeMapper.h"

#include <Domain/ScenarioChange.h>

#include <DataLayer/Database/Database.h>

#include <QSet>
#include <QSqlQuery>

using namespace DataMappingLayer;


namespace {
    const QString kColumns = " id, uuid, datetime, username, undo_patch, redo_patch, is_draft ";
    const QString kTableName = " scenario_changes ";
}

ScenarioChange* ScenarioChangeMapper::find(const Identifier& _id)
{
    return dynamic_cast<ScenarioChange*>(abstractFind(_id));
}

ScenarioChangesTable* ScenarioChangeMapper::findLast(int _count)
{
    QString queryFilter = QString("WHERE id IN (SELECT id FROM %1 ORDER BY datetime DESC LIMIT %2)").arg(kTableName).arg(_count);
    return findAll(queryFilter);
}

ScenarioChangesTable* ScenarioChangeMapper::findAll(const QString& _queryFilter)
{
    return qobject_cast<ScenarioChangesTable*>(abstractFindAll(_queryFilter));
}

void ScenarioChangeMapper::insert(ScenarioChange* _change)
{
    abstractInsert(_change);
}

void ScenarioChangeMapper::update(ScenarioChange* _change)
{
    abstractUpdate(_change);
}

void ScenarioChangeMapper::remove(ScenarioChange* _change)
{
    abstractDelete(_change);
}

bool ScenarioChangeMapper::containsUuid(const QString& _uuid)
{
    QSqlQuery checker = DatabaseLayer::Database::query();
    checker.prepare("SELECT COUNT(id) FROM " + kTableName + " WHERE uuid = ?");
    checker.addBindValue(_uuid);
    checker.exec();
    checker.next();
    return checker.value(0).toInt();
}

QList<QString> ScenarioChangeMapper::uuids() const
{
    QSqlQuery loader = DatabaseLayer::Database::query();
    loader.exec("SELECT uuid FROM " + kTableName);
    QList<QString> uuids;
    while (loader.next()) {
        uuids.append(loader.value(0).toString());
    }
    return uuids;
}

ScenarioChange ScenarioChangeMapper::change(const QString& _uuid) const
{
    QSqlQuery loader = DatabaseLayer::Database::query();
    loader.prepare("SELECT " + kColumns + " FROM " + kTableName + " WHERE uuid = ? ");
    loader.addBindValue(_uuid);
    loader.exec();
    loader.next();
    return
            ScenarioChange(Identifier(), _uuid, loader.value("datetime").toDateTime(),
                loader.value("username").toString(), loader.value("undo_patch").toString(),
                loader.value("redo_patch").toString(), loader.value("is_draft").toInt());
}

QString ScenarioChangeMapper::findStatement(const Identifier& _id) const
{
    QString findStatement =
            QString("SELECT " + kColumns +
                    " FROM " + kTableName +
                    " WHERE id = %1 "
                    )
            .arg(_id.value());
    return findStatement;
}

QString ScenarioChangeMapper::findAllStatement() const
{
    return "SELECT " + kColumns + " FROM  " + kTableName;
}

QString ScenarioChangeMapper::insertStatement(DomainObject* _subject, QVariantList& _insertValues) const
{
    QString insertStatement =
            QString("INSERT INTO " + kTableName +
                    " (" + kColumns + ") "
                    " VALUES(?, ?, ?, ?, ?, ?, ?) "
                    );

    ScenarioChange* change = dynamic_cast<ScenarioChange*>(_subject );
    _insertValues.clear();
    _insertValues.append(change->id().value());
    _insertValues.append(change->uuid().toString());
    _insertValues.append(change->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz"));
    _insertValues.append(change->user());
    _insertValues.append(change->undoPatch());
    _insertValues.append(change->redoPatch());
    _insertValues.append(change->isDraft() ? "1" : "0");

    return insertStatement;
}

QString ScenarioChangeMapper::updateStatement(DomainObject* _subject, QVariantList& _updateValues) const
{
    QString updateStatement =
            QString("UPDATE " + kTableName +
                    " SET uuid = ?, "
                    " datetime = ?, "
                    " username = ?, "
                    " undo_patch = ?, "
                    " redo_patch = ?, "
                    " is_draft = ? "
                    " WHERE id = ? "
                    );

    ScenarioChange* change = dynamic_cast<ScenarioChange*>(_subject);
    _updateValues.clear();
    _updateValues.append(change->uuid().toString());
    _updateValues.append(change->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz"));
    _updateValues.append(change->user());
    _updateValues.append(change->undoPatch());
    _updateValues.append(change->redoPatch());
    _updateValues.append(change->isDraft() ? "1" : "0");
    _updateValues.append(change->id().value());

    return updateStatement;
}

QString ScenarioChangeMapper::deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const
{
    QString deleteStatement = "DELETE FROM " + kTableName + " WHERE id = ?";

    _deleteValues.clear();
    _deleteValues.append(_subject->id().value());

    return deleteStatement;
}

DomainObject* ScenarioChangeMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
    const QUuid uuid = QUuid(_record.value("uuid").toString());
    const QDateTime datetime = QDateTime::fromString(_record.value("datetime").toString(), "yyyy-MM-dd hh:mm:ss:zzz");
    const QString user = _record.value("username").toString();
    const QString undoPatch = _record.value("undo_patch").toString();
    const QString redoPatch = _record.value("redo_patch").toString();
    const bool isDraft = _record.value("is_draft").toInt();

    return new ScenarioChange(_id, uuid, datetime, user, undoPatch, redoPatch, isDraft);
}

void ScenarioChangeMapper::doLoad(DomainObject* _domainObject, const QSqlRecord& _record)
{
    if (ScenarioChange* change = dynamic_cast<ScenarioChange*>(_domainObject)) {
        const QUuid uuid = QUuid(_record.value("uuid").toString());
        change->setUuid(uuid);

        const QDateTime datetime = QDateTime::fromString(_record.value("datetime").toString(), "yyyy-MM-dd hh:mm:ss:zzz");
        change->setDatetime(datetime);

        const QString user = _record.value("username").toString();
        change->setUser(user);

        const QString undoPatch = _record.value("undo_patch").toString();
        change->setUndoPatch(undoPatch);

        const QString redoPatch = _record.value("redo_patch").toString();
        change->setRedoPatch(redoPatch);

        const bool isDraft = _record.value("is_draft").toInt();
        change->setIsDraft(isDraft);
    }
}

DomainObjectsItemModel* ScenarioChangeMapper::modelInstance()
{
    return new ScenarioChangesTable;
}

ScenarioChangeMapper::ScenarioChangeMapper()
{
}

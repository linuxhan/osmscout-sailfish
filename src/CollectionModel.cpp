/*
  OSMScout for SFOS
  Copyright (C) 2018 Lukas Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "CollectionModel.h"
#include "QVariantConverters.h"

#include <QDebug>
#include <QtCore/QStandardPaths>
#include <QStorageInfo>

namespace {
QString safeFileName(QString name)
{
  return name.replace(QRegExp("[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]"), QString("_"));
}
}

CollectionModel::CollectionModel()
{
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &CollectionModel::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::initialisationError,
          this, &CollectionModel::storageInitialisationError,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::collectionDetailRequest,
          storage, &Storage::loadCollectionDetails,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionDetailsLoaded,
          this, &CollectionModel::onCollectionDetailsLoaded,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::deleteWaypointRequest,
          storage, &Storage::deleteWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::deleteTrackRequest,
          storage, &Storage::deleteTrack,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::createWaypointRequest,
          storage, &Storage::createWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::editWaypointRequest,
          storage, &Storage::editWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::editTrackRequest,
          storage, &Storage::editTrack,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::exportCollectionRequest,
          storage, &Storage::exportCollection,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionExported,
          this, &CollectionModel::onCollectionExported,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::exportTrackRequest,
          storage, &Storage::exportTrack,
          Qt::QueuedConnection);

  connect(storage, &Storage::trackExported,
          this, &CollectionModel::onTrackExported,
          Qt::QueuedConnection);

  connect(storage, &Storage::error,
          this, &CollectionModel::error,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::moveWaypointRequest,
          storage, &Storage::moveWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::moveTrackRequest,
          storage, &Storage::moveTrack,
          Qt::QueuedConnection);

}

CollectionModel::~CollectionModel()
{
}

void CollectionModel::storageInitialised()
{
  beginResetModel();
  collectionLoaded = false;
  endResetModel();
  if (collection.id > 0) {
    emit collectionDetailRequest(collection);
  }
}

void CollectionModel::storageInitialisationError(QString)
{
  storageInitialised();
}

void CollectionModel::onCollectionDetailsLoaded(Collection collection, bool ok)
{
  if (this->collection.id != collection.id){
    return;
  }
  collectionLoaded = true;
  this->collection = collection;

  handleChanges<Waypoint>(0, waypoints, collection.waypoints ? *(collection.waypoints): std::vector<Waypoint>());
  handleChanges<Track>(waypoints.size(), tracks, collection.tracks ? *(collection.tracks): std::vector<Track>());

  if (!ok){
    qWarning() << "Collection load fails";
  }
  emit loadingChanged();
}

int CollectionModel::rowCount([[maybe_unused]] const QModelIndex &parentIndex) const
{
  return tracks.size() + waypoints.size();
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
  using namespace converters;
  using namespace std::string_literals;

  if(index.row() < 0) {
    return QVariant();
  }
  int row = index.row();

  if (row < waypoints.size()){
    const Waypoint &waypoint = waypoints.at(row);
    switch(role){
      case TypeRole: return "waypoint";
      case NameRole: return QString::fromStdString(waypoint.data.name.value_or(""s));
      case FilesystemNameRole: return safeFileName(QString::fromStdString(waypoint.data.name.value_or(""s)));
      case DescriptionRole: return QString::fromStdString(waypoint.data.description.value_or(""s));
      case IdRole: return QString::number(waypoint.id);

      case SymbolRole: return QString::fromStdString(waypoint.data.symbol.value_or(""s));
      case LatitudeRole: return waypoint.data.coord.GetLat();
      case LongitudeRole: return waypoint.data.coord.GetLon();
      case ElevationRole: return waypoint.data.elevation.has_value() ? *(waypoint.data.elevation) : QVariant();

      case TimeRole: return timestampToDateTime(waypoint.data.time);
      case LastModificationRole: return waypoint.lastModification;
      default: return QVariant();
    }
  } else {
    row -= waypoints.size();
  }
  if (row < tracks.size()){
    const Track &track = tracks.at(row);
    switch(role){
      case TypeRole: return "track";
      case NameRole: return track.name;
      case FilesystemNameRole: return safeFileName(track.name);
      case DescriptionRole: return track.description;
      case IdRole: return QString::number(track.id);

      case TimeRole: return track.statistics.from;
      case LastModificationRole: return track.lastModification;
      case DistanceRole: return track.statistics.distance.AsMeter();
      default: return QVariant();
    }
  }

  return QVariant();
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole] = "name";
  roles[FilesystemNameRole] = "filesystemName";
  roles[DescriptionRole] = "description";
  roles[TypeRole] = "type";
  roles[IdRole] = "id";
  roles[TimeRole] = "time";
  roles[LastModificationRole] = "lastModification";

  // waypoint
  roles[SymbolRole] = "symbol";
  roles[LatitudeRole] = "latitude";
  roles[LongitudeRole] = "longitude";
  roles[ElevationRole] = "elevation";

  // track
  roles[DistanceRole] = "distance";

  return roles;
}

Qt::ItemFlags CollectionModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void CollectionModel::setCollectionId(QString id)
{
  bool ok;
  collection.id = id.toLongLong(&ok);
  if (!ok)
    collection.id = -1;

  emit collectionDetailRequest(collection);
}

bool CollectionModel::isLoading() const
{
  return !collectionLoaded;
}

bool CollectionModel::isExporting()
{
  return collectionExporting;
}

QString CollectionModel::getCollectionName() const
{
  return collectionLoaded? collection.name : "";
}

QString CollectionModel::getCollectionFilesystemName() const
{
  return safeFileName(getCollectionName());
}

QString CollectionModel::getCollectionDescription() const
{
  return collectionLoaded? collection.description : "";
}

bool CollectionModel::isVisible() const
{
  return collectionLoaded? collection.visible : false;
}

void CollectionModel::createWaypoint(double lat, double lon, QString name, QString description)
{
  collectionLoaded = true;
  emit loadingChanged();
  emit createWaypointRequest(collection.id, lat, lon, name, description);
}

void CollectionModel::deleteWaypoint(QString idStr)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit deleteWaypointRequest(collection.id, id);
}

void CollectionModel::deleteTrack(QString idStr)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit deleteTrackRequest(collection.id, id);
}

void CollectionModel::editWaypoint(QString idStr, QString name, QString description)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit editWaypointRequest(collection.id, id, name, description);
}

void CollectionModel::editTrack(QString idStr, QString name, QString description)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit editTrackRequest(collection.id, id, name, description);
}

void CollectionModel::exportToFile(QString fileName, QString directory)
{
  QFileInfo dir(directory);
  if (!dir.isDir() || !dir.isWritable()){
    qWarning() << "Invalid export directory:" << dir.absoluteFilePath();
    emit error(tr("Invalid export directory"));
    return;
  }
  QString safeName = safeFileName(fileName);
  if (safeName.isEmpty()){
    qWarning() << "Invalid file name:" << fileName << "/" << safeName;
    emit error(tr("Invalid file name"));
    return;
  }
  QFileInfo file(QDir(dir.absoluteFilePath()), safeName + ".gpx");
  collectionExporting = true;
  emit exportingChanged();
  emit exportCollectionRequest(collection.id, file.absoluteFilePath());
}

void CollectionModel::exportTrackToFile(QString trackIdStr, QString fileName, QString directory)
{
  bool ok;
  qint64 trackId = trackIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << trackIdStr << "to number";
    return;
  }

  QFileInfo dir(directory);
  if (!dir.isDir() || !dir.isWritable()){
    qWarning() << "Invalid export directory:" << dir.absoluteFilePath();
    emit error(tr("Invalid export directory"));
    return;
  }
  QString safeName = safeFileName(fileName);
  if (safeName.isEmpty()){
    qWarning() << "Invalid file name:" << fileName << "/" << safeName;
    emit error(tr("Invalid file name"));
    return;
  }
  QFileInfo file(QDir(dir.absoluteFilePath()), safeName + ".gpx");
  collectionExporting = true;
  emit exportingChanged();
  emit exportTrackRequest(collection.id, trackId, file.absoluteFilePath());
}

void CollectionModel::onCollectionExported(bool)
{
  collectionExporting = false;
  emit exportingChanged();
}

void CollectionModel::onTrackExported(bool)
{
  collectionExporting = false;
  emit exportingChanged();
}

QStringList CollectionModel::getExportSuggestedDirectories()
{
  QStringList result;
  result << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  result << QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

  for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
      if (storage.isValid() &&
          storage.isReady() &&
          !storage.isReadOnly() &&
          ( // Sailfish OS specific mount point base for SD cards!
            storage.rootPath().startsWith("/media") ||
            storage.rootPath().startsWith("/run/media/") /* SFOS >= 2.2 */
            )) {
        result << storage.rootPath();
      }
  }

  return result;
}

void CollectionModel::moveWaypoint(QString waypointIdStr, QString collectionIdStr)
{
  bool ok;
  qint64 waypointId = waypointIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << waypointIdStr << "to number";
    return;
  }
  qint64 collectionId = collectionIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << collectionIdStr << "to number";
    return;
  }

  collectionLoaded = false;
  emit loadingChanged();
  emit moveWaypointRequest(waypointId, collectionId);
}

void CollectionModel::moveTrack(QString trackIdStr, QString collectionIdStr)
{
  bool ok;
  qint64 trackId = trackIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << trackIdStr << "to number";
    return;
  }
  qint64 collectionId = collectionIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << collectionIdStr << "to number";
    return;
  }

  collectionLoaded = false;
  emit loadingChanged();
  emit moveTrackRequest(trackId, collectionId);
}

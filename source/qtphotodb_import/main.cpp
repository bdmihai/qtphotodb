/****************************************************************************
**
** Copyright (C) 2010-2015 B.D. Mihai.
**
** This file is part of qtphotodb.
**
** qtphotodb  is  free  software:  you  can redistribute it and/or modify it
** under the terms of the GNU Lesser Public License as published by the Free
** Software Foundation, either version 3 of the License, or (at your option)
** any later version.
**
** qtphotodb  is  distributed  in  the  hope  that  it  will be  useful,  but
** WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
** or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser Public License for
** more details.
**
** You should have received a copy of the GNU Lesser Public License along
** with qtphotodb.  If not, see http://www.gnu.org/licenses/.
**
****************************************************************************/

#include "stable.h"
#include "defines.h"
#include "options.h"
#include "exif.h"

QTextStream cout(stdout, QIODevice::WriteOnly);
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream clog;

bool importFile    (const QString &rootPath,
                    const QString &importPath,
                    const QString &filePath);
bool importInPhotos(const QString &filePath,
                    quint32 &photo_id,
                    QString &photo_name,
                    bool    &photo_dupe);
bool importInExif  (const QString &filePath,
                    const quint32 &photo_id);
bool importInTags  (const QString &importPath,
                    const QString &filePath,
                    const quint32 &photo_id);
bool importInAlbums(const QString &importPath,
                    const quint32 &photo_id);

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  Options options;

  bool noLogo = false;
  QString rootPath;
  QString importPath;

  // set the application info
  app.setApplicationName(APP_NAME);
  app.setOrganizationName(APP_COMPANY);
  app.setOrganizationDomain(APP_DOMAIN);
  app.setApplicationVersion(APP_VERSION);

  // add the application options
  options.add(&rootPath,   "rootPath",              "directory where the db shall be created", true );
  options.add(&importPath, "importPath", "-i"     , "directory where the db shall be created", true );
  options.add(&noLogo,     "",           "-nologo", "do not show logo",                        false);

  // set the application options values
  if (!options.set())
  {
    cout << options.logo()  << endl;
    cout << options.usage() << endl;
    return 1;
  }
  else if (!noLogo)
  {
    // print copyright logo
    cout << options.logo() << endl;
  }

  cout << "Initial check";
  // prepare and check the root directory
  QDir rootDir(rootPath);
  if (!rootDir.exists())
  {
    cerr << "ERROR: Directory " << rootPath << " not found!" << endl;
    return 1;
  }
  if (!rootDir.isReadable())
  {
    cerr << "ERROR: Directory " << rootPath << " not readable!" << endl;
    return 1;
  }
  rootPath.replace('\\', '/');
  if (rootPath.endsWith('/')) rootPath.remove(rootPath.length() - 1, 1);
  cout << ".";

  // check for a database in the root path
  if (!QFileInfo::exists(rootPath + "/database.s3db"))
  {
    cerr << "ERROR: Directory " << rootPath << " has no database!" << endl;
  }
  cout << ".";

  // prepare and check the import directory
  QDir importDir(importPath);
  if (!importDir.exists())
  {
    cerr << "ERROR: Directory " << importPath << " not found!" << endl;
    return 1;
  }
  if (!importDir.isReadable())
  {
    cerr << "ERROR: Directory " << importPath << " not readable!" << endl;
    return 1;
  }
  importPath.replace('\\', '/');
  if (importPath.endsWith('/')) importPath.remove(importPath.length() - 1, 1);
  cout << ".";

  // create the application database
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(rootPath + "/database.s3db");
  if (!db.open())
  {
    cerr << "ERROR: Database " << rootPath + "/database.s3db" << " cannot be opened!" << endl;
    return 2;
  }
  cout << ".done" << endl;

  // create a log file
  QDateTime logTime = QDateTime::currentDateTime();
  QFile logFile(rootPath + "/log/" + QString("%1-%2-%3-%4-%5-%6.import.log")
                                     .arg(logTime.date().year())
                                     .arg(logTime.date().month(),  2, 10, QChar('0'))
                                     .arg(logTime.date().day(),    2, 10, QChar('0'))
                                     .arg(logTime.time().hour(),   2, 10, QChar('0'))
                                     .arg(logTime.time().minute(), 2, 10, QChar('0'))
                                     .arg(logTime.time().second(), 2, 10, QChar('0')));
  logFile.open(QIODevice::WriteOnly);
  clog.setDevice(&logFile);

  // parse the import path for pictures
  cout << "Importing photos";
  QStringList filter; int cnt = 0;
  filter << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.tiff";
  QDirIterator it(importPath, filter, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
    QString filePath = it.next();
    importFile(rootPath, importPath, filePath);
    cnt++; cout << "."; cout.flush(); if (cnt % 50 == 0) cout << cnt << endl;
  }
  cout << cnt << " done." << endl;

  logFile.close();
  return 0;
}

bool importFile(const QString &rootPath, const QString &importPath, const QString &filePath)
{
  quint32   photo_id   = 0;
  QString   photo_name = "";
  bool      photo_dupe = false;

  // start transaction for one photo import
  QSqlDatabase::database().transaction();

  // import all photo details into the database - rollback if it does not work
  if (!importInPhotos(filePath, photo_id, photo_name, photo_dupe))
  {
     QSqlDatabase::database().rollback();
     return false;
  }

  // copy photo to bulk directory - rollback if it does not work
  if (!photo_dupe)
  {
    if (!QFile::copy(filePath, rootPath + "/bulk/" + photo_name))
    {
      QSqlDatabase::database().rollback();
      cerr << "ERROR: File " << filePath << " cannot be copied!" << endl;
      return false;
    }
    clog << photo_name << " : " << filePath << endl;

    // store exif data into the database
    importInExif(filePath, photo_id);
  }

  // store information about location of th imported photo in tags and albums
  // album : top level import directory
  // tag   : each sub-directory splitted by '-' sign
  importInAlbums(importPath, photo_id);
  importInTags(importPath, filePath, photo_id);

  // commit all changes to the database
  QSqlDatabase::database().commit();

  return true;
}

bool importInPhotos(const QString &filePath, quint32 &photo_id, QString &photo_name, bool &photo_dupe)
{
  QFile file(filePath);

  if (!file.open(QIODevice::ReadOnly))
  {
    cerr << "ERROR: " << filePath << " cannot be opened!" << endl;
    return false;
  }

  // make a MD5 hash of the picture to be able to compare
  QCryptographicHash hash(QCryptographicHash::Md5);
  hash.addData(file.readAll());
  file.close();

  QSqlQuery q(QSqlDatabase::database());
  if (!q.exec("SELECT max(Photos.Id) FROM Photos"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }

  QDateTime photo_date = QFileInfo(filePath).lastModified();
            photo_id   = q.next() ? (q.value(0).toUInt() + 1) : 1;
  QString   photo_hash = hash.result().toHex().toUpper();
  qint64    photo_size = QFileInfo(filePath).size();
            photo_name = QString("%1-%2-%3-%4.%5")
                         .arg(photo_date.date().year())
                         .arg(photo_date.date().month(), 2, 10, QChar('0'))
                         .arg(photo_date.date().day(), 2, 10, QChar('0'))
                         .arg(photo_id, 6, 16, QChar('0'))
                         .arg(QFileInfo(filePath).suffix())
                         .toUpper();

  // check for same size/hash
  if (!q.prepare("SELECT Photos.Id,Photos.Name FROM Photos WHERE Photos.Hash=? AND Photos.Size=? AND Photos.Date=?"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  q.bindValue(0, photo_hash);
  q.bindValue(1, photo_size);
  q.bindValue(2, photo_date);
  if (!q.exec())
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }

  if (q.next())
  {
    // the photo is already in the database
    photo_dupe = true;
    photo_id   = q.value(0).toUInt();
    photo_name = q.value(1).toString();
    clog << photo_name << " [dupe]: " << filePath << endl;
    return true;
  }

  // insert the photo in the database
  photo_dupe = false;
  if (!q.prepare("INSERT INTO Photos (Id,Name,Hash,Size,Date) VALUES(?,?,?,?,?)"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  q.bindValue(0, photo_id);
  q.bindValue(1, photo_name);
  q.bindValue(2, photo_hash);
  q.bindValue(3, photo_size);
  q.bindValue(4, photo_date);
  if (!q.exec())
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }

  return true;
}

bool importInExif(const QString &filePath, const quint32 &photo_id)
{
  // Read the JPEG file into a buffer
  FILE *fp = fopen(filePath.toStdString().c_str(), "rb");
  if (!fp) {
    clog << "EXIF ERROR [open]: " << filePath << endl;
    return false;
  }
  fseek(fp, 0, SEEK_END);
  unsigned long fsize = ftell(fp);
  rewind(fp);
  unsigned char *buf = new unsigned char[fsize];
  if (fread(buf, 1, fsize, fp) != fsize) {
    clog << "EXIF ERROR [read]: " << filePath << endl;
    delete[] buf;
    return false;
  }
  fclose(fp);

  // Parse EXIF
  easyexif::EXIFInfo result;
  int code = result.parseFrom(buf, fsize);
  delete[] buf;
  if (code) {
    clog << "EXIF ERROR [" << code << "]:" << filePath << endl;
    return false;
  }

  QSqlQuery q(QSqlDatabase::database());
  if (!q.prepare("INSERT INTO Exif (ImageDescription,Make,Model,Software,DateTime,ImageWidth,ImageHeight,Latitude,Longitude,Altitude,PhotoId)"
                 "VALUES(?,?,?,?,?,?,?,?,?,?,?)"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  q.bindValue(0,  result.ImageDescription.c_str());
  q.bindValue(1,  result.Make.c_str());
  q.bindValue(2,  result.Model.c_str());
  q.bindValue(3,  result.Software.c_str());
  q.bindValue(4,  result.DateTime.c_str());
  q.bindValue(5,  result.ImageWidth);
  q.bindValue(6,  result.ImageHeight);
  q.bindValue(7,  result.GeoLocation.Latitude);
  q.bindValue(8,  result.GeoLocation.Longitude);
  q.bindValue(9,  result.GeoLocation.Altitude);
  q.bindValue(10, photo_id);
  if (!q.exec())
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }

  return true;
}

bool importInTags(const QString &importPath, const QString &filePath, const quint32 &photo_id)
{
  QString folderPath = QFileInfo(filePath).absolutePath();
  folderPath.replace(importPath, "", Qt::CaseInsensitive);
  folderPath = folderPath.toLower();

  QStringList folders = folderPath.split('/', QString::SkipEmptyParts);
  QStringList labels;
  for (int i = 0; i < folders.count(); i++)
  {
    labels.append(folders[i].split('-', QString::SkipEmptyParts));
  }
  labels.removeDuplicates();

  QSqlQuery q(QSqlDatabase::database());

  // check for same label/photoID
  QStringList tags;
  if (!q.prepare("SELECT Tags.Name,Tags.PhotoId FROM Tags WHERE Tags.Name=? AND Tags.PhotoId=?"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  for (int i = 0; i < labels.count(); i++)
  {
    q.bindValue(0, labels[i].trimmed());
    q.bindValue(1, photo_id);
    if (!q.exec())
    {
      cerr << "ERROR: " << q.lastError().text() << endl;
      return false;
    }
    if (!q.next())
    {
      tags.append(labels[i]);
    }
  }

  if (!q.prepare("INSERT INTO Tags (Name,PhotoId) VALUES(?,?)"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  for (int i = 0; i < tags.count(); i++)
  {
    q.bindValue(0, tags[i].trimmed());
    q.bindValue(1, photo_id);
    if (!q.exec())
    {
      cerr << "ERROR: " << q.lastError().text() << endl;
      return false;
    }
  }

  return true;
}

bool importInAlbums(const QString &importPath, const quint32 &photo_id)
{
  QString album = QDir(importPath).dirName();
  QSqlQuery q(QSqlDatabase::database());

  // check for same album/photoID
  if (!q.prepare("SELECT Albums.Name,Albums.PhotoId FROM Albums WHERE Albums.Name=? AND Albums.PhotoId=?"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  q.bindValue(0, album);
  q.bindValue(1, photo_id);
  if (!q.exec())
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  if (q.next())
  {
    return true;
  }

  if (!q.prepare("INSERT INTO Albums (Name,PhotoId) VALUES(?,?)"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  q.bindValue(0, album);
  q.bindValue(1, photo_id);
  if (!q.exec())
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }

  return true;
}

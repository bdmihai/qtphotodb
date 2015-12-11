/****************************************************************************
**
** Copyright (C) 2010-2011 B.D. Mihai.
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

bool importFile(QFileInfo fileInfo,
                QSqlDatabase db,
                QString rootPath,
                QString importPath);

bool importInPhotos(QFileInfo fileInfo,
                    QSqlDatabase db,
                    quint32 &photo_id,
                    QString &photo_name,
                    QString &photo_hash,
                    quint32 &photo_size,
                    QDateTime &photo_date,
                    bool &photo_dupe);

bool importInExif(QFileInfo fileInfo,
                  QSqlDatabase db,
                  quint32 &photo_id);

bool importInTags(QFileInfo fileInfo,
                  QSqlDatabase db,
                  quint32 &photo_id,
                  QString importPath);

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

  QDir rootDir(rootPath);

  if (!rootDir.exists())
  {
    cerr << "Directory " << rootPath << " not found!" << endl;
    return 1;
  }

  if (!rootDir.isReadable())
  {
    cerr << "Directory " << rootPath << " not readable!" << endl;
    return 1;
  }
  rootPath.replace('\\', '/');

  QDir importDir(importPath);

  if (!importDir.exists())
  {
    cerr << "Directory " << importPath << " not found!" << endl;
    return 1;
  }

  if (!importDir.isReadable())
  {
    cerr << "Directory " << importPath << " not readable!" << endl;
    return 1;
  }
  importPath.replace('\\', '/');

  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(rootPath + "/database.s3db");

  if (!db.open())
  {
    cerr << "Database " << rootPath + "/database.s3db" << " cannot be opened!" << endl;
    return 2;
  }

  QFile logFile("qtphotodb_import.log");
  logFile.open(QIODevice::WriteOnly);
  clog.setDevice(&logFile);

  QStringList filter;
  filter << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.tiff";
  QDirIterator it(importPath, filter, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
    it.next();
    if (!importFile(it.fileInfo(), db, rootPath, importPath))
    {
      cerr << "File " << it.filePath() << " cannot be imported!" << endl;
    }
  }

  logFile.close();
  return 0;
}

bool importFile(QFileInfo fileInfo,
                QSqlDatabase db,
                QString rootPath,
                QString importPath)
{
  quint32   photo_id   = 0;
  QString   photo_name = "";
  QString   photo_hash = "";
  quint32   photo_size = 0;
  bool      photo_dupe = false;
  QDateTime photo_date;

  db.transaction();
  if (!importInPhotos(fileInfo, db, photo_id, photo_name, photo_hash, photo_size, photo_date, photo_dupe))
  {
     db.rollback();
     return false;
  }

  if (!importInTags(fileInfo, db, photo_id, importPath))
  {
     db.rollback();
     return false;
  }

  if (!photo_dupe)
  {
    if (!importInExif(fileInfo, db, photo_id))
    {
       db.rollback();
       return false;
    }

    if (!QFile::copy(fileInfo.filePath(), rootPath + "/bulk/" + photo_name))
    {
      db.rollback();
      cerr << "File " << fileInfo.filePath() << " cannot be copied!" << endl;
      return false;
    }
    else
    {
      cout << photo_id << "..." << fileInfo.filePath() <<endl;
    }
  }
  else
  {
    cout << "dupe..." << fileInfo.filePath() <<endl;
  }

  db.commit();

  // log photo details
  clog << "[" << photo_name << "]" << endl;
  clog << "PHOTO_ID   = " << QString("%1").arg(photo_id, 6, 16, QChar('0')).toUpper() << endl;
  clog << "PHOTO_PATH = " << fileInfo.filePath() << endl;
  clog << "PHOTO_HASH = " << photo_hash << endl;
  clog << "PHOTO_SIZE = " << photo_size << endl;
  clog << "PHOTO_DATE = " << photo_date.toString() << endl;
  clog << "PHOTO_DUPE = " << photo_dupe << endl;
  return true;
}

bool importInPhotos(QFileInfo fileInfo,
                    QSqlDatabase db,
                    quint32 &photo_id,
                    QString &photo_name,
                    QString &photo_hash,
                    quint32 &photo_size,
                    QDateTime &photo_date,
                    bool &photo_dupe)
{
  QFile file(fileInfo.filePath());

  if (!file.open(QIODevice::ReadOnly))
  {
    cerr << "importInPhotos: " << fileInfo.filePath() << " cannot be opened!" << endl;
    return false;
  }

  QCryptographicHash hash(QCryptographicHash::Md5);
  hash.addData(file.readAll());

  QSqlQuery q(db);
  q.exec("SELECT max(Id) FROM Photos");

  photo_date = fileInfo.lastModified();
  photo_id   = q.next() ? (q.value(0).toUInt() + 1) : 1;
  photo_hash = hash.result().toHex().toUpper();
  photo_size = fileInfo.size();
  photo_name = QString("%1-%2-%3-%4.%5")
               .arg(photo_date.date().year())
               .arg(photo_date.date().month(), 2, 10, QChar('0'))
               .arg(photo_date.date().day(), 2, 10, QChar('0'))
               .arg(photo_id, 6, 16, QChar('0'))
               .arg(fileInfo.suffix())
               .toUpper();

  file.close();

  // check for same size/hash
  q.prepare("SELECT Id,Name FROM Photos WHERE Hash=? AND Size=? AND Date=?");
  q.bindValue(0, photo_hash);
  q.bindValue(1, photo_size);
  q.bindValue(2, photo_date);
  q.exec();

  if (q.next())
  {
    photo_id   = q.value(0).toUInt();
    photo_name = q.value(1).toString();
    photo_dupe = true;
  }
  else
  {
    photo_dupe = false;
    q.prepare("INSERT INTO Photos (Id,Name,Hash,Size,Date) VALUES(?,?,?,?,?)");
    q.bindValue(0, photo_id);
    q.bindValue(1, photo_name);
    q.bindValue(2, photo_hash);
    q.bindValue(3, photo_size);
    q.bindValue(4, photo_date);
    q.exec();
  }

  return true;
}

bool importInExif(QFileInfo fileInfo,
                  QSqlDatabase db,
                  quint32 &photo_id)
{
  // Read the JPEG file into a buffer
  FILE *fp = fopen(fileInfo.filePath().toStdString().c_str(), "rb");
  if (!fp) {
    cerr << "importInExif: Can't open file." << endl;
    return true;
  }
  fseek(fp, 0, SEEK_END);
  unsigned long fsize = ftell(fp);
  rewind(fp);
  unsigned char *buf = new unsigned char[fsize];
  if (fread(buf, 1, fsize, fp) != fsize) {
    cerr << "importInExif: Can't read file." << endl;
    delete[] buf;
    return true;
  }
  fclose(fp);

  // Parse EXIF
  easyexif::EXIFInfo result;
  int code = result.parseFrom(buf, fsize);
  delete[] buf;
  if (code) {
    cerr << "Error parsing EXIF: code " << code << endl;
    return true;
  }

  QSqlQuery q(db);

  q.prepare("INSERT INTO Exif (ImageDescription,Make,Model,Software,DateTime,ImageWidth,ImageHeight,Latitude,Longitude,Altitude,PhotoId)"
            "VALUES(?,?,?,?,?,?,?,?,?,?,?)");

  q.bindValue(0, result.ImageDescription.c_str());
  q.bindValue(1, result.Make.c_str());
  q.bindValue(2, result.Model.c_str());
  q.bindValue(3, result.Software.c_str());
  q.bindValue(4, result.DateTime.c_str());
  q.bindValue(5, result.ImageWidth);
  q.bindValue(6, result.ImageHeight);
  q.bindValue(7, result.GeoLocation.Latitude);
  q.bindValue(8, result.GeoLocation.Longitude);
  q.bindValue(9, result.GeoLocation.Altitude);
  q.bindValue(10, photo_id);
  q.exec();

  return true;
}

bool importInTags(QFileInfo fileInfo,
                  QSqlDatabase db,
                  quint32 &photo_id,
                  QString importPath)
{
  QString folderPath = fileInfo.dir().path();
  folderPath.replace(importPath, "", Qt::CaseInsensitive);

  QStringList labels = folderPath.split('/');

  QSqlQuery q(db);

  q.prepare("INSERT INTO Tags (Name,PhotoId)"
            "VALUES(?,?)");

  for (int i = 0; i < labels.count(); i++)
  {
    q.bindValue(0, labels[i].trimmed());
    q.bindValue(1, photo_id);
    q.exec();
  }



  return true;
}


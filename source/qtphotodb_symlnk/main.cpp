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

QTextStream cout(stdout, QIODevice::WriteOnly);
QTextStream cerr(stderr, QIODevice::WriteOnly);

bool linkByDate (QDir sortDir);
bool linkByTag  (QDir sortDir);
bool linkByAlbum(QDir sortDir);
bool linkBySize (QDir sortDir);

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  Options options;

  bool noLogo = false;
  QString rootPath;
  QString linkBy;

  // set the application info
  app.setApplicationName(APP_NAME);
  app.setOrganizationName(APP_COMPANY);
  app.setOrganizationDomain(APP_DOMAIN);
  app.setApplicationVersion(APP_VERSION);

  // add the application options
  options.add(&rootPath, "rootPath",             "directory where the db shall be created", true );
  options.add(&linkBy,   "linkBy",   "-link_by", "lynk by (date, tag, size, album)",        true );
  options.add(&noLogo,   "",         "-nologo" , "do not show logo",                        false);

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

  // prepare and check the sort directory
  QDir sortDir(rootDir.path() + "/sort");
  if (!sortDir.exists())
  {
    cerr << "Directory " << sortDir.path() << " not found!" << endl;
    return 1;
  }
  if (!sortDir.isReadable())
  {
    cerr << "ERROR: Directory " << sortDir.path() << " not readable!" << endl;
    return 1;
  }

  // check for a database in the root path
  if (!QFileInfo::exists(rootPath + "/database.s3db"))
  {
    cerr << "ERROR: Directory " << rootPath << " has no database!" << endl;
  }
  cout << ".";

  // create the application database
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(rootPath + "/database.s3db");
  if (!db.open())
  {
    cerr << "Database " << rootPath + "/database.s3db" << " canot be opened!" << endl;
    return 2;
  }
  cout << "done" << endl;

  QStringList linkByList = linkBy.split(',', QString::SkipEmptyParts);
  for (int i = 0; i < linkByList.count(); i++)
  {
    cout << "Linking by " << linkByList[i]; cout.flush();
    if (linkByList[i] == "date")
    {
      linkByDate(sortDir);
    }
    else if ( linkByList[i] == "tag" )
    {
      linkByTag(sortDir);
    }
    else if ( linkByList[i] == "album" )
    {
      linkByAlbum(sortDir);
    }
    else if ( linkByList[i] == "size" )
    {
      linkBySize(sortDir);
    }
    cout << "done" << endl;
  }

  return 0;
}

bool linkByDate(QDir sortDir)
{
  QDir linkRoot(sortDir.path() + "/by_date");
  if (!linkRoot.exists(sortDir.path() + "/by_date"))
  {
    sortDir.mkdir("by_date");
  }

  QSqlQuery q(QSqlDatabase::database());
  if (!q.exec("SELECT Photos.Name,Photos.Date FROM Photos"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  int cnt = 0;
  while (q.next())
  {
    QString   name  = q.value(0).toString();
    QDateTime tstmp = q.value(1).toDateTime();

    // absolute link directory path
    QString linkDirPath = QString("%1/%2/%3/%4")
                          .arg(sortDir.path() + "/by_date")
                          .arg(tstmp.date().year())
                          .arg(tstmp.date().month(), 2, 10, QChar('0'))
                          .arg(tstmp.date().day()  , 2, 10, QChar('0'));

    // absolute link file path
    QString linkFilePath = QString("%1/%2")
                           .arg(linkDirPath)
                           .arg(name);

    // relative target file path
    QString targetFilePath = QString("../../../../../bulk/%1")
                             .arg(name);

    // if link file exists continue with next item
    if (QFileInfo(linkFilePath).exists()) continue;

    // create the directory and make sure it exists
    QDir linkDir(linkDirPath); linkDir.mkpath(linkDirPath);

    // create the link
    QFile::link(targetFilePath, linkFilePath);
    cnt++; cout << "."; cout.flush(); if (cnt % 50 == 0) cout << cnt << endl;
  }

  return true;
}

bool linkByTag(QDir sortDir)
{
  QDir linkRoot(sortDir.path() + "/by_tag");
  if (!linkRoot.exists(sortDir.path() + "/by_tag"))
  {
    sortDir.mkdir("by_tag");
  }

  QSqlQuery q(QSqlDatabase::database());
  if (!q.exec("SELECT Photos.Name,Photos.date,Tags.Name FROM Photos INNER JOIN Tags ON Photos.Id = Tags.PhotoId"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  int cnt = 0;
  while (q.next())
  {
    QString   name   = q.value(0).toString();
    QDateTime tstmp  = q.value(1).toDateTime();
    QString   tag    = q.value(2).toString();

    // absolute link directory path
    QString linkDirPath = QString("%1/%2/%3-%4")
                          .arg(sortDir.path() + "/by_tag")
                          .arg(tag)
                          .arg(tstmp.date().year())
                          .arg(tstmp.date().month(), 2, 10, QChar('0'));

    // absolute link file path
    QString linkFilePath = QString("%1/%2")
                           .arg(linkDirPath)
                           .arg(name);

    // relative target file path
    QString targetFilePath = QString("../../../../bulk/%1")
                             .arg(name);

    // if link file exists continue with next item
    if (QFileInfo(linkFilePath).exists()) continue;

    // create the directory and make sure it exists
    QDir linkDir(linkDirPath); linkDir.mkpath(linkDirPath);

    // create the link
    QFile::link(targetFilePath, linkFilePath);
    cnt++; cout << "."; cout.flush(); if (cnt % 50 == 0) cout << cnt << endl;
  }

  return true;
}

bool linkByAlbum(QDir sortDir)
{
  QDir linkRoot(sortDir.path() + "/by_album");
  if (!linkRoot.exists(sortDir.path() + "/by_album"))
  {
    sortDir.mkdir("by_album");
  }

  QSqlQuery q(QSqlDatabase::database());
  if (!q.exec("SELECT Photos.Name,Photos.date,Albums.Name FROM Photos INNER JOIN Albums ON Photos.Id = Albums.PhotoId"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  int cnt = 0;
  while (q.next())
  {
    QString   name   = q.value(0).toString();
    QDateTime tstmp  = q.value(1).toDateTime();
    QString   album  = q.value(2).toString();

    // absolute link directory path
    QString linkDirPath = QString("%1/%2/%3-%4")
                          .arg(sortDir.path() + "/by_album")
                          .arg(album)
                          .arg(tstmp.date().year())
                          .arg(tstmp.date().month(), 2, 10, QChar('0'));

    // absolute link file path
    QString linkFilePath = QString("%1/%2")
                           .arg(linkDirPath)
                           .arg(name);

    // relative target file path
    QString targetFilePath = QString("../../../../bulk/%1")
                             .arg(name);

    // if link file exists continue with next item
    if (QFileInfo(linkFilePath).exists()) continue;

    // create the directory and make sure it exists
    QDir linkDir(linkDirPath); linkDir.mkpath(linkDirPath);

    // create the link
    QFile::link(targetFilePath, linkFilePath);
    cnt++; cout << "."; cout.flush(); if (cnt % 50 == 0) cout << cnt << endl;
  }

  return true;
}

bool linkBySize(QDir sortDir)
{
  QDir linkRoot(sortDir.path() + "/by_size");
  if (!linkRoot.exists(sortDir.path() + "/by_size"))
  {
    sortDir.mkdir("by_size");
  }

  QSqlQuery q(QSqlDatabase::database());
  if (!q.exec("SELECT Photos.Name,Photos.Date,Exif.ImageWidth,Exif.ImageHeight FROM Photos INNER JOIN Exif ON Photos.Id = Exif.PhotoId"))
  {
    cerr << "ERROR: " << q.lastError().text() << endl;
    return false;
  }
  int cnt = 0;
  while (q.next())
  {
    QString   name   = q.value(0).toString();
    QDateTime tstmp  = q.value(1).toDateTime();
    int       width  = q.value(2).toInt();
    int       height = q.value(3).toInt();

    // absolute link directory path
    QString linkDirPath = QString("%1/%2x%3/%4-%5")
                          .arg(sortDir.path() + "/by_size")
                          .arg(width)
                          .arg(height)
                          .arg(tstmp.date().year())
                          .arg(tstmp.date().month(), 2, 10, QChar('0'));

    // absolute link file path
    QString linkFilePath = QString("%1/%2")
                           .arg(linkDirPath)
                           .arg(name);

    // relative target file path
    QString targetFilePath = QString("../../../../bulk/%1")
                             .arg(name);

    // if link file exists continue with next item
    if (QFileInfo(linkFilePath).exists()) continue;

    // create the directory and make sure it exists
    QDir linkDir(linkDirPath); linkDir.mkpath(linkDirPath);

    // create the link
    QFile::link(targetFilePath, linkFilePath);
    cnt++; cout << "."; cout.flush(); if (cnt % 50 == 0) cout << cnt << endl;
  }

  return true;
}

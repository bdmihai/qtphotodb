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
QTextStream clog;

bool linkByDate(QDir sortDir, QSqlDatabase db);

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
  options.add(&linkBy,   "linkBy",   "-link_by", "lynk by (date, tag, album, camera)",      true );
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

  QDir rootDir = QDir(rootPath);

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

  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(rootPath + "/database.s3db");

  if (!db.open())
  {
    cerr << "Database " << rootPath + "/database.s3db" << " canot be opened!" << endl;
    return 2;
  }

  QFile logFile("qtphotodb_symlink.log");
  logFile.open(QIODevice::WriteOnly);
  clog.setDevice(&logFile);

  QDir sortDir(rootDir.path() + "/sort");
  if (!sortDir.exists())
  {
    cerr << "Directory " << sortDir.path() << " not found!" << endl;
    return 1;
  }

  if (!sortDir.isReadable())
  {
    cerr << "Directory " << sortDir.path() << " not readable!" << endl;
    return 1;
  }

  if (linkBy == "date")
  {
    linkByDate(sortDir, db);
  }


  logFile.close();
  return 0;
}

bool linkByDate(QDir sortDir, QSqlDatabase db)
{
  QDir linkRoot(sortDir.path() + "/by_date");

  if (linkRoot.exists(sortDir.path() + "/by_date"))
  {
    QDir(sortDir.path() + "/by_date").removeRecursively();
  }
  sortDir.mkdir("by_date");

  QSqlQuery q1(db);
  q1.exec("SELECT Date FROM Photos");
  while (q1.next())
  {
    QDateTime ts = q1.value(0).toDateTime();
    QString datePath = QString("/%1/%2/%3")
                       .arg(ts.date().year())
                       .arg(ts.date().month(), 2, 10, QChar('0'))
                       .arg(ts.date().day(), 2, 10, QChar('0'));

    QDir dateDir(linkRoot.path() + datePath);
    if (!dateDir.exists())
    {
      dateDir.mkpath(dateDir.path());
    }

    QSqlQuery q2(db);
    q2.exec(QString("SELECT Name FROM Photos WHERE Name LIKE '%1-%2-%3%'")
            .arg(ts.date().year())
            .arg(ts.date().month(), 2, 10, QChar('0'))
            .arg(ts.date().day(), 2, 10, QChar('0')));
    while (q2.next())
    {
      QString linkName = QString(dateDir.path() + "/%1").arg(q2.value(0).toString());

      if (QFileInfo(linkName).exists())
      {
        continue;
      }

      QString fileName = QString("../../../../../bulk/%1").arg(q2.value(0).toString());
      QFile::link(fileName, linkName);
    }
  }

  return true;
}

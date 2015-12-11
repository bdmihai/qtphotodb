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

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  Options options;

  bool noLogo = false;
  QString rootPath;

  // set the application info
  app.setApplicationName(APP_NAME);
  app.setOrganizationName(APP_COMPANY);
  app.setOrganizationDomain(APP_DOMAIN);
  app.setApplicationVersion(APP_VERSION);

  // add the application options
  options.add(&rootPath, "rootPath",            "directory where the db shall be created", true );
  options.add(&noLogo,   "",         "-nologo", "do not show logo",                        false);

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

  if (rootDir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot).count() != 0)
  {
    cerr << "Directory " << rootPath << " is not empty! Please provide a empty directory." << endl;
    return 1;
  }

  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(rootPath + "/database.s3db");

  if (!db.open())
  {
    cerr << "Database " << rootPath + "/database.s3db" << " canot be opened!" << endl;
    return 2;
  }

  QFile logFile("qtphotodb_create.log");
  logFile.open(QIODevice::WriteOnly);
  clog.setDevice(&logFile);

  QSqlQuery query;
  clog << "[database.s3db]" << endl;

  query.exec("CREATE TABLE [Albums] (                            " \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL," \
             "  [Name] VARCHAR(1024)  NOT NULL,                  " \
             "  [PhotoId] INTEGER  NOT NULL                      " \
             ");                                                 ");
  clog << "Albums = " << query.lastQuery().simplified() << endl;

  query.exec("CREATE TABLE [Exif] (                              " \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL," \
             "  [ImageDescription] VARCHAR(1024)  NULL,          " \
             "  [Make] VARCHAR(1024)  NULL,                      " \
             "  [Model] VARCHAR(1024)  NULL,                     " \
             "  [Software] VARCHAR(1024)  NULL,                  " \
             "  [DateTime] VARCHAR(1024)  NULL,                  " \
             "  [ImageWidth] INTEGER  NULL,                      " \
             "  [ImageHeight] INTEGER  NULL,                     " \
             "  [Latitude] FLOAT  NULL,                          " \
             "  [Longitude] FLOAT  NULL,                         " \
             "  [Altitude] FLOAT  NULL,                          " \
             "  [PhotoId] INTEGER  NOT NULL                      " \
             ");                                                 ");
  clog << "Exif = " << query.lastQuery().simplified() << endl;

  query.exec("CREATE TABLE [Photos] (                            " \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL," \
             "  [Name] VARCHAR(32)  UNIQUE NOT NULL,             " \
             "  [Hash] VARCHAR(32)  NOT NULL,                    " \
             "  [Size] INTEGER  NOT NULL,                        " \
             "  [Date] TIMESTAMP  NOT NULL                       " \
             ");                                                 ");
  clog << "Photos = " << query.lastQuery().simplified() << endl;

  query.exec("CREATE TABLE [Tags] (                              " \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL," \
             "  [Name] VARCHAR(1024)  NOT NULL,                  " \
             "  [PhotoId] INTEGER  NOT NULL                      " \
             ");                                                 ");
  clog << "Tags = " << query.lastQuery().simplified() << endl;

  rootDir.mkdir("bulk");
  clog << "BulkDirectory = bulk" << endl;

  rootDir.mkdir("sort");
  clog << "SortDirectory = sort" << endl;

  logFile.close();
  return 0;
}
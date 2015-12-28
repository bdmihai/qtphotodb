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

  // check that the directory is empty
  if (rootDir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot).count() != 0)
  {
    cerr << "ERROR: Directory " << rootPath << " is not empty! Please provide a empty directory." << endl;
    return 1;
  }

  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(rootPath + "/database.s3db");
  if (!db.open())
  {
    cerr << "ERROR: Database " << rootPath + "/database.s3db" << " canot be opened!" << endl;
    return 2;
  }

  // create directories
  cout << "Creating directories";
  rootDir.mkdir("bulk");   cout << ".";
  rootDir.mkdir("sort");   cout << ".";
  rootDir.mkdir("log");    cout << ".";
  rootDir.mkdir("backup"); cout << ".";
  rootDir.mkdir("cache");  cout << ".";
  cout << "done" << endl;

  // create a log file
  QDateTime logTime = QDateTime::currentDateTime();
  QFile logFile(rootPath + "/log/" + QString("%1-%2-%3-%4-%5-%6.create.log")
                                     .arg(logTime.date().year())
                                     .arg(logTime.date().month(),  2, 10, QChar('0'))
                                     .arg(logTime.date().day(),    2, 10, QChar('0'))
                                     .arg(logTime.time().hour(),   2, 10, QChar('0'))
                                     .arg(logTime.time().minute(), 2, 10, QChar('0'))
                                     .arg(logTime.time().second(), 2, 10, QChar('0')));
  logFile.open(QIODevice::WriteOnly);
  clog.setDevice(&logFile);

  cout << "Creating database";
  QSqlQuery query;
  clog << "Database : " << rootPath + "/database.s3db" << endl;

  query.exec("CREATE TABLE [Albums] (                            \n" \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\n" \
             "  [Name] VARCHAR(1024)  NOT NULL,                  \n" \
             "  [PhotoId] INTEGER  NOT NULL                      \n" \
             ");                                                 \n");
  clog << "Albums : " << query.lastQuery().simplified() << endl; cout << ".";

  query.exec("CREATE TABLE [Exif] (                              \n" \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\n" \
             "  [ImageDescription] VARCHAR(1024)  NULL,          \n" \
             "  [Make] VARCHAR(1024)  NULL,                      \n" \
             "  [Model] VARCHAR(1024)  NULL,                     \n" \
             "  [Software] VARCHAR(1024)  NULL,                  \n" \
             "  [DateTime] VARCHAR(1024)  NULL,                  \n" \
             "  [ImageWidth] INTEGER  NULL,                      \n" \
             "  [ImageHeight] INTEGER  NULL,                     \n" \
             "  [Latitude] FLOAT  NULL,                          \n" \
             "  [Longitude] FLOAT  NULL,                         \n" \
             "  [Altitude] FLOAT  NULL,                          \n" \
             "  [PhotoId] INTEGER  NOT NULL                      \n" \
             ");                                                 \n");
  clog << "Exif : " << query.lastQuery().simplified() << endl; cout << ".";

  query.exec("CREATE TABLE [Photos] (                            \n" \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\n" \
             "  [Name] VARCHAR(32)  UNIQUE NOT NULL,             \n" \
             "  [Hash] VARCHAR(32)  NOT NULL,                    \n" \
             "  [Size] INTEGER  NOT NULL,                        \n" \
             "  [Date] TIMESTAMP  NOT NULL                       \n" \
             ");                                                 \n");
  clog << "Photos : " << query.lastQuery().simplified() << endl; cout << ".";

  query.exec("CREATE TABLE [Tags] (                              \n" \
             "  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\n" \
             "  [Name] VARCHAR(1024)  NOT NULL,                  \n" \
             "  [PhotoId] INTEGER  NOT NULL                      \n" \
             ");                                                 \n");
  clog << "Tags : " << query.lastQuery().simplified() << endl; cout << ".";
  cout << "done" << endl;

  logFile.close();
  return 0;
}

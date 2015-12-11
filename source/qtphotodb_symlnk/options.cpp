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
#include "options.h"

struct Option
{
  enum OptionType { string, boolean, stringList };
  void *var;
  OptionType type;
  QString tag, name, desc;
  bool mandatory;
};

Options::Options()
{
  defaultOption = 0;
}

Options::~Options()
{
  qDeleteAll(optionList);
  delete defaultOption;
}

void Options::add(QString *var, const QString &name, const QString &tag, const QString &desc, bool mandatory)
{
  Option *option = new Option();

  option->var = var;
  option->name = name;
  option->type = Option::string;
  option->tag = tag;
  option->desc = desc;
  option->mandatory = mandatory;

  optionList.append(option);
}

void Options::add(QStringList *var, const QString &name, const QString &tag, const QString &desc, bool mandatory)
{
  Option *option = new Option();

  option->var = var;
  option->name = name;
  option->type = Option::stringList;
  option->tag = tag;
  option->desc = desc;
  option->mandatory = mandatory;

  optionList.append(option);
}

void Options::add(bool *var, const QString &name, const QString &tag, const QString &desc, bool mandatory)
{
  Option *option = new Option();

  option->var = var;
  option->name = name;
  option->type = Option::boolean;
  option->tag = tag;
  option->desc = desc;
  option->mandatory = mandatory;

  optionList.append(option);
}

void Options::add(QString *var, const QString &name, const QString &desc, bool mandatory)
{
  defaultOption = new Option();

  defaultOption->var = var;
  defaultOption->name = name;
  defaultOption->type = Option::string;
  defaultOption->tag = "";
  defaultOption->desc = desc;
  defaultOption->mandatory = mandatory;
}

bool Options::set()
{
  QStringList arguments = qApp->arguments();

  // make a list with all mandatory options
  QList<Option*> mandatoryOptions;
  for (int i = 0; i < optionList.count(); i++)
  {
    Option *option = optionList[i];
    if (option->mandatory)
    {
      mandatoryOptions.append(option);
    }
  }
  if (defaultOption)
  {
    if (defaultOption->mandatory)
    {
      mandatoryOptions.append(defaultOption);
    }
  }

  // remove the application path from the arguments
  arguments.removeFirst();

  // parse the arguments
  while (arguments.count())
  {
    bool tagFound = false;
    for (int i = 0; i < optionList.count(); i++)
    {
      Option *option = optionList[i];
      if (arguments.first().compare(option->tag, Qt::CaseInsensitive) == 0)
      {
        tagFound = true;
        mandatoryOptions.removeAll(option);
        arguments.removeFirst();
        setValue(option, arguments);
        break;
      }
    }

    // no tag -> default option if defined
    if (!tagFound && defaultOption)
    {
      mandatoryOptions.removeAll(defaultOption);
      setValue(defaultOption, arguments);
    }
  }
  
  // all mandatory options have been provided
  return (mandatoryOptions.count() == 0);
}

void Options::setValue(Option *option, QStringList &arguments)
{
  switch (option->type)
  {
    case Option::string:      { *((QString*)(option->var)) = arguments.first(); arguments.removeFirst();           break; }
    case Option::stringList:  { ((QStringList*)(option->var))->append(arguments.first()); arguments.removeFirst(); break; }
    case Option::boolean:     { *((bool*)   (option->var)) = true;                                                 break; }
  }
}

QString Options::usage()
{
  QString usageString;
  QTextStream out(&usageString);


  // create the usage path with options mandatory/optional
  out << "usage:" << endl;

  QString mandatoryOpt, optionalOpt;
  for (int i = 0; i < optionList.count(); i++)
  {
    Option *option = optionList[i];

    if (option->mandatory)
      if (option->name != "")
        mandatoryOpt += option->tag + " <" + option->name + "> ";
      else
        mandatoryOpt += option->tag + " ";
    else
      if (option->name != "")
        optionalOpt += option->tag + " <" + option->name + "> ";
      else
        optionalOpt += option->tag + " ";
  }

  out << "  " << QFileInfo(qApp->applicationFilePath()).fileName()     << 
    ((defaultOption != 0) ? (" <" + defaultOption->name + "> ") : " ") <<
    ((mandatoryOpt != "") ? (mandatoryOpt                     ) : "")  <<
    ((optionalOpt != "")  ? ("[ " + optionalOpt + "]"         ) : "")  << endl;


  // add details about the options
  if (optionList.count() > 0)
  {
    out << endl;
    out <<"options:" << endl;

    for (int i = 0; i < optionList.count(); i++)
    {
      Option *option = optionList[i];

      if (option->name != "")
        out << "  " + QString("%1").arg(option->tag + " <" + option->name + ">", -20, QChar(' ')) + "- " + option->desc << endl;
      else
        out << "  " + QString("%1").arg(option->tag                            , -20, QChar(' ')) + "- " + option->desc << endl;
    }
  }

  return usageString;
}

QString Options::logo()
{
  QString logoString;
  QTextStream out(&logoString);

  out << qApp->applicationName() << " Version " << qApp->applicationVersion() << endl;
  out << "Copyright (C) " << qApp->organizationName() << ". All rights reserved." << endl;
  out << endl;

  return logoString;
}

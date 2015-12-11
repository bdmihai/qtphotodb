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

#ifndef OPTIONS_H
#define OPTIONS_H

struct Option;
class Options
{
  public:
    Options();
    virtual ~Options();

    void add(QString     *var, const QString &name, const QString &tag, const QString &desc, bool mandatory);
    void add(bool        *var, const QString &name, const QString &tag, const QString &desc, bool mandatory);
    void add(QStringList *var, const QString &name, const QString &tag, const QString &desc, bool mandatory);
    void add(QString     *var, const QString &name, const QString &desc, bool mandatory);

    bool set();

    QString usage();
    QString logo();

  private:
    void setValue(Option *option, QStringList &arguments);

  private:
    QList<Option*> optionList;
    Option* defaultOption;
};

#endif // OPTIONS_H

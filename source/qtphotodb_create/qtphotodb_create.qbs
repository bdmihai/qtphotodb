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

import qbs

Product {
  name: "qtphotodb_create"
  type: "application"
  consoleApplication: true

  // dependencies
  Depends { name: "cpp" }
  Depends { name: "Qt.core" }
  Depends { name: "Qt.sql" }

  files: [
          "stable.h",
          "defines.h",
          "main.cpp",
          "options.h",
          "options.cpp"
  ]

  // cpp module configuration
  cpp.cxxPrecompiledHeader: "stable.h"

  // properties for the produced executable
  Group {
    qbs.install: true
    qbs.installDir: "bin"
    fileTagsFilter: product.type
  }
}

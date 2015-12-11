CREATE TABLE [Albums] (
  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,
  [Name] VARCHAR(1024)  NOT NULL,
  [PhotoId] INTEGER  NOT NULL
);

CREATE TABLE [Exif] (
  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,
  [ImageDescription] VARCHAR(1024)  NULL,
  [Make] VARCHAR(1024)  NULL,
  [Model] VARCHAR(1024)  NULL,
  [Software] VARCHAR(1024)  NULL,
  [DateTime] TIMESTAMP  NULL,
  [ImageWidth] INTEGER  NULL,
  [ImageHeight] INTEGER  NULL,
  [Latitude] FLOAT  NULL,
  [Longitude] FLOAT  NULL,
  [Altitude] FLOAT  NULL,
  [PhotoId] INTEGER  NOT NULL
);

CREATE TABLE [Photos] (
  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,
  [Name] VARCHAR(1024)  UNIQUE NOT NULL,
  [Hash] VARCHAR(1024)  NOT NULL,
  [Size] INTEGER  NOT NULL
);

CREATE TABLE [Tags] (
  [Id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,
  [Name] VARCHAR(1024)  NOT NULL,
  [PhotoId] INTEGER  NOT NULL
);

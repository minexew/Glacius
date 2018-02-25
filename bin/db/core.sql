CREATE TABLE `Accounts`
(
  `AccountID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `Name` varchar(40) UNIQUE NOT NULL,
  `Pass` INTEGER NOT NULL
);

CREATE TABLE `Characters`
(
  `CharID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `AccountID` INTEGER NOT NULL,
  `Name` varchar(25) NOT NULL,
  `Race` TINYINT NOT NULL,
  `Class` TINYINT NOT NULL DEFAULT '0',
  `Level` SMALLINT NOT NULL DEFAULT '1',
  `Zone` SMALLINT NOT NULL DEFAULT '800',
  `x` FLOAT NOT NULL DEFAULT '300',
  `y` FLOAT NOT NULL DEFAULT '300',
  `z` FLOAT NOT NULL DEFAULT '0',
  `orientation` FLOAT NOT NULL DEFAULT '0',
  `Rights` TINYINT NOT NULL DEFAULT '0',
  `Gold` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE `RealmConfig`
(
  `Option` varchar(16) PRIMARY KEY NOT NULL,
  `Value` text NOT NULL
);

CREATE TABLE `RealmNews`
(
  `Date` date PRIMARY KEY NOT NULL,
  `Text` text NOT NULL
);

CREATE TABLE `ZoneNames`
(
  `Zone` INTEGER PRIMARY KEY NOT NULL,
  `AreaName` varchar(32) NOT NULL,
  `ZoneName` varchar(32) DEFAULT NULL
);

INSERT INTO `RealmConfig` (`Option`, `Value`) VALUES ('RealmName', 'Lanthaia Alpha');
INSERT INTO `RealmConfig` (`Option`, `Value`) VALUES ('RealmMotd', 'Welcome to the Tales of Lanthaia test server!');
INSERT INTO `RealmConfig` (`Option`, `Value`) VALUES ('RealmWebsite', 'http://lanthaia.net/');

INSERT INTO `ZoneNames` (`Zone`, `AreaName`, `ZoneName`) VALUES (800, 'Devel Island', NULL);
INSERT INTO `ZoneNames` (`Zone`, `AreaName`, `ZoneName`) VALUES (11000, 'Nael''othe', NULL);
INSERT INTO `ZoneNames` (`Zone`, `AreaName`, `ZoneName`) VALUES (11001, 'Nael''othe', 'Nei''othe');
INSERT INTO `ZoneNames` (`Zone`, `AreaName`, `ZoneName`) VALUES (11002, 'Nael''othe', 'Khai Nael''othe');
INSERT INTO `ZoneNames` (`Zone`, `AreaName`, `ZoneName`) VALUES (11003, 'Nael''othe', 'Laai''othe');
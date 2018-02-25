SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;


CREATE TABLE `Accounts` (
  `AccountID` int(11) NOT NULL AUTO_INCREMENT,
  `Name` varchar(40) CHARACTER SET utf8 COLLATE utf8_czech_ci NOT NULL,
  `Pass` char(32) CHARACTER SET utf8 COLLATE utf8_czech_ci NOT NULL,
  PRIMARY KEY (`AccountID`),
  UNIQUE KEY `Name` (`Name`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 ;

CREATE TABLE `Characters` (
  `CharID` int NOT NULL AUTO_INCREMENT,
  `AccountID` int NOT NULL,
  `Name` varchar(25) CHARACTER SET utf8 COLLATE utf8_czech_ci NOT NULL,
  `Race` tinyint NOT NULL,
  `Class` tinyint NOT NULL DEFAULT '0',
  `Level` smallint NOT NULL DEFAULT '1',
  `Zone` smallint NOT NULL DEFAULT '800',
  `x` float NOT NULL DEFAULT '300',
  `y` float NOT NULL DEFAULT '300',
  `z` float NOT NULL DEFAULT '0',
  `orientation` double NOT NULL DEFAULT '0',
  `Rights` tinyint(4) NOT NULL DEFAULT '0',
  `Gold` bigint(20) NOT NULL DEFAULT '0',
  PRIMARY KEY (`CharID`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 ;

CREATE TABLE `RealmConfig` (
  `Option` varchar(16) COLLATE utf8_czech_ci NOT NULL,
  `Value` text COLLATE utf8_czech_ci NOT NULL,
  PRIMARY KEY (`Option`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_czech_ci;

CREATE TABLE `RealmNews` (
  `Date` date NOT NULL,
  `Text` text COLLATE utf8_czech_ci NOT NULL,
  PRIMARY KEY (`Date`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_czech_ci;

CREATE TABLE `ZoneNames` (
  `Zone` int(11) NOT NULL,
  `AreaName` varchar(32) COLLATE utf8_czech_ci NOT NULL,
  `ZoneName` varchar(32) COLLATE utf8_czech_ci DEFAULT NULL,
  PRIMARY KEY (`Zone`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_czech_ci;

INSERT INTO `RealmConfig` (`Option`, `Value`) VALUES ('RealmName', 'Lanthaia Alpha');
INSERT INTO `RealmConfig` (`Option`, `Value`) VALUES ('RealmMotd', 'Welcome to the Tales of Lanthaia test server!');

INSERT INTO `ZoneNames` (`Zone`, `AreaName`, `ZoneName`) VALUES (800, 'Devel Island', NULL);
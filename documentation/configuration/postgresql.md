---
layout: page
title: Spectrum 2
---

## Editing the configuration file

To configure Spectrum 2 to use PostgreSQL database, you have to edit following options in database section:

Section | Key | Type | Change to value | Description
--------|-----|------|-----------------|------------
database| type | string | pqxx | Database type - "none", "mysql", "sqlite3", "pqxx".
database| database | string | Name of the already create empty database | Database used to store data.
database| server | string | Database server | Database server.
database| user | string | PostgreSQL user. | PostgreSQL user.
database| password | string | PostgreSQL Password. | PostgreSQL Password.
database| prefix | string | | Prefix of tables in database.

## Creating the database

Spectrum 2 will create the database on the first execution. Once the database is created, you can remove the CREATE TABLE permissions to the PostgreSQL database user you use to connect the SQL.

#pragma once

/*
  BlueShine: Open-Source Database Interface
    Copyright 2009, 2010 Minexew

    This file is part of BlueShine.

    BlueShine is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BlueShine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BlueShine.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ShineResult.hpp"

#ifndef __mysql_hpp__
struct MYSQL;
#endif

namespace ShineDB
{
    const char* getLastError();
    void setLastError( const char* );

    class Session
    {
        MYSQL* mySQL;
        unsigned affectedRows;

        Session( MYSQL* sess );

        public:
            /* create a new Session instance */
            static Session* create( const char* host, const char* username, const char* password, const char* dbName );
            ~Session();

            /* perform a query */
            bool query( const char* query, QueryOutput*& output );

            /* the number of affected rows in latest query */
            unsigned getAffectedRows() const { return affectedRows; }
            unsigned lastInsertID();
    };
}

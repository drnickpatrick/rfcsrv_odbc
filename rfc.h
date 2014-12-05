/*
 * Copyright (C) 2014 Nick Patrick (http://scn.sap.com/people/dr.nick.patrick)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _RFCH_
#define _RFCH_

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sapnwrfc.h>

#include "dbc.h"


void errorHandling(RFC_RC rc, SAP_UC* description, RFC_ERROR_INFO* errorInfo, RFC_CONNECTION_HANDLE connection);

RFC_RC SAP_API zSQLCommitImplementation(RFC_CONNECTION_HANDLE rfcHandle, RFC_FUNCTION_HANDLE funcHandle, RFC_ERROR_INFO* errorInfoP);

RFC_RC SAP_API zSQLRollbackImplementation(RFC_CONNECTION_HANDLE rfcHandle, RFC_FUNCTION_HANDLE funcHandle, RFC_ERROR_INFO* errorInfoP);

RFC_RC SAP_API zSQLExecImplementation(RFC_CONNECTION_HANDLE rfcHandle, RFC_FUNCTION_HANDLE funcHandle, RFC_ERROR_INFO* errorInfoP);


#endif

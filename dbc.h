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

#ifndef _DBCH_
#define _DBCH_

#include <iostream>
#include <windows.h>
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>
#include <io.h>
#include <fcntl.h>
#include <vector>
#include <list>
#include <string>
#include <sapnwrfc.h>

using namespace std;

//#define __DEBUG__


// this is due to vs 2008 lack of functionality, comment it if use recent version of vs ----+
#define _WLLFMT	L"%I64"

inline wstring to_wstring(_Longlong _Val)
	{	// convert long long to wstring
	wchar_t _Buf[2 * _MAX_INT_DIG];

	_CSTD swprintf(_Buf, sizeof (_Buf) / sizeof (_Buf[0]),
		_WLLFMT L"d", _Val);
	return (wstring(_Buf));
	}

inline wstring to_wstring(_ULonglong _Val)
	{	// convert unsigned long long to wstring
	wchar_t _Buf[2 * _MAX_INT_DIG];

	_CSTD swprintf(_Buf, sizeof (_Buf) / sizeof (_Buf[0]),
		_WLLFMT L"u", _Val);
	return (wstring(_Buf));
	}

inline wstring to_wstring(long double _Val)
	{	// convert long double to wstring
	wchar_t _Buf[_MAX_EXP_DIG + _MAX_SIG_DIG + 64];

	_CSTD swprintf(_Buf,sizeof (_Buf) / sizeof (_Buf[0]),
		L"%Lg", _Val);
	return (wstring(_Buf));
	}
// -----------------------------------------------------------------------------------------+


class sql_param{
public:
  SQLUSMALLINT    ParameterNumber;
  SQLSMALLINT     InputOutputType;
  SQLSMALLINT     ParameterType; // SQL data type
  SQLLEN out_len;
  wstring sap_datatype;

  wstring v_sqlwchar;
  string v_sqlchar;
  long int v_sqlinteger;
  long double v_sqldouble;
  DATE_STRUCT v_date;
  TIME_STRUCT v_time;
  TIMESTAMP_STRUCT v_timestamp;

  sql_param(const SQLUSMALLINT i_ParameterNumber, const SQLSMALLINT i_InputOutputType, const wstring i_val, const wstring i_sap_dt=wstring()){
    ParameterNumber = i_ParameterNumber;
    InputOutputType = i_InputOutputType;
    ParameterType = SQL_WCHAR;
    v_sqlwchar = i_val;
    out_len = 0;
    sap_datatype = i_sap_dt;
  }
  sql_param(const SQLUSMALLINT i_ParameterNumber, const SQLSMALLINT i_InputOutputType, const string i_val, const wstring i_sap_dt=wstring()){
    ParameterNumber = i_ParameterNumber;
    InputOutputType = i_InputOutputType;
    ParameterType = SQL_CHAR;
    v_sqlchar = i_val;
    out_len = 0;
    sap_datatype = i_sap_dt;
  }
  sql_param(const SQLUSMALLINT i_ParameterNumber, const SQLSMALLINT i_InputOutputType, const long int i_val, const wstring i_sap_dt=wstring()){
    ParameterNumber = i_ParameterNumber;
    InputOutputType = i_InputOutputType;
    ParameterType = SQL_INTEGER;
    v_sqlinteger = i_val;
    out_len = 0;
    sap_datatype = i_sap_dt;
  }
  sql_param(const SQLUSMALLINT i_ParameterNumber, const SQLSMALLINT i_InputOutputType, const double i_val, const wstring i_sap_dt=wstring()){
    ParameterNumber = i_ParameterNumber;
    InputOutputType = i_InputOutputType;
    ParameterType = SQL_DOUBLE;
    v_sqldouble = i_val;
    out_len = 0;
    sap_datatype = i_sap_dt;
  }
  sql_param(const SQLUSMALLINT i_ParameterNumber, const SQLSMALLINT i_InputOutputType, const DATE_STRUCT & i_val, const wstring i_sap_dt=wstring()){
    ParameterNumber = i_ParameterNumber;
    InputOutputType = i_InputOutputType;
    ParameterType = SQL_TYPE_DATE;
    v_date = i_val;
    out_len = 0;
    sap_datatype = i_sap_dt;
  }

  sql_param(const SQLUSMALLINT i_ParameterNumber, const SQLSMALLINT i_InputOutputType, const TIME_STRUCT & i_val, const wstring i_sap_dt=wstring()){
    ParameterNumber = i_ParameterNumber;
    InputOutputType = i_InputOutputType;
    ParameterType = SQL_TYPE_TIME;
    v_time = i_val;
    out_len = 0;
    sap_datatype = i_sap_dt;
  }

  sql_param(const SQLUSMALLINT i_ParameterNumber, const SQLSMALLINT i_InputOutputType, const TIMESTAMP_STRUCT & i_val, const wstring i_sap_dt=wstring()){
    ParameterNumber = i_ParameterNumber;
    InputOutputType = i_InputOutputType;
    ParameterType = SQL_TYPE_TIMESTAMP;
    v_timestamp = i_val;
    out_len = 0;
    sap_datatype = i_sap_dt;
  }

  SQLLEN * get_out_ptr(){
    if(InputOutputType == SQL_PARAM_OUTPUT || InputOutputType == SQL_PARAM_INPUT_OUTPUT){
      return &out_len; 
    }
    else{
      return NULL;
    }
  }

  void * get_value_ptr_and_len(SQLLEN & o_len, SQLUSMALLINT & c_value_type)const{ 
    void * result = NULL;
    o_len = 0;
    switch(ParameterType){
    case SQL_WCHAR:
      result = (void*)v_sqlwchar.c_str();
      o_len = v_sqlwchar.length();
      c_value_type = SQL_C_WCHAR;
      break;
    case SQL_CHAR:
      result = (void*)v_sqlchar.c_str();
      o_len = v_sqlchar.length();
      c_value_type = SQL_C_CHAR;
      break;
    case SQL_INTEGER:
      result = (void*)&v_sqlinteger;
      c_value_type = SQL_C_SLONG;
      break;
    case SQL_DOUBLE:
      result = (void*) &v_sqldouble;
      c_value_type = SQL_C_DOUBLE;
      break;
    case SQL_TYPE_DATE:
      result = (void*) &v_date;
      c_value_type = SQL_C_TYPE_DATE;
      break;
    case SQL_TYPE_TIME:
      result = (void*) &v_time;
      c_value_type = SQL_C_TYPE_TIME;
      break;
    case SQL_TYPE_TIMESTAMP:
      result = (void*) &v_timestamp;
      c_value_type = SQL_C_TYPE_TIMESTAMP;
      break;
    default:
      break;
    }
    return result;
  }

  wstring to_sap_value();

};



void errorHandling2(RFC_RC rc, RFC_ERROR_INFO* errorInfo);

bool init_connection(const wstring i_conn_str);
void close_connection();

bool exec_sql(const wstring sql, const unsigned int max_rows, const RFC_TABLE_HANDLE rfc_table, const RFC_TABLE_HANDLE  fi_table, list<sql_param> & sql_params,
  list<wstring> & sql_messages, unsigned int & rows);

SQLRETURN do_commit(wstring & sql_message);
SQLRETURN do_rollback(wstring & sql_mess);

#if defined __DEBUG__
#define LOG(__var){ __var }
#endif

#if !defined __DEBUG__
#define LOG(__var)
#endif


#endif

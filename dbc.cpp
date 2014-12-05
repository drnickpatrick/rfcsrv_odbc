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

#include <iostream>
#include <windows.h>
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>
#include <io.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <sapnwrfc.h>

#include "dbc.h"

using namespace std;

SQLHANDLE sqlconnectionhandle = SQL_NULL_HANDLE;
SQLHANDLE sqlenvhandle = SQL_NULL_HANDLE;

void errorHandling2(RFC_RC rc, RFC_ERROR_INFO* errorInfo)
{
  if(rc != RFC_OK){
    LOG(printfU(cU("error info: %s: %s\n"), errorInfo->key, errorInfo->message);)
  }
}

void show_error(unsigned int handletype, const SQLHANDLE& handle, const SQLRETURN rc, list<wstring> * o_sql_messages_ptr=NULL){
  if(handle){
    SQLWCHAR sqlstate[1024];
    SQLWCHAR message[1024];
//	_setmode(_fileno(stdout), _O_U16TEXT);
    if(SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, sqlstate, NULL, message, 1024, NULL)){
      printfU(cU("%s:%d\n"), message, sqlstate);
      if(o_sql_messages_ptr) o_sql_messages_ptr->push_back(to_wstring(_Longlong(rc))+wstring(L":")+wstring(message));
    }
    else{
      printfU(cU("SQLGetDiagRec fail!\n"));
      if(o_sql_messages_ptr) o_sql_messages_ptr->push_back(to_wstring(_Longlong(rc))+wstring(L":")+wstring(L"SQLGetDiagRec fail!"));
    }
  }
  else{
    printfU(cU("show_error: handle=0, unable to get error info!\n"));
    if(o_sql_messages_ptr) o_sql_messages_ptr->push_back(to_wstring(_Longlong(rc))+wstring(L":")+wstring(L"show_error: handle=0, unable to get error info!"));
  }
}

bool init_connection(const wstring i_conn_str)
{
  if(sqlconnectionhandle != SQL_NULL_HANDLE) return true; // connection already opened

  SQLRETURN rc;
  rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlenvhandle);
  if(rc != SQL_SUCCESS){
    show_error(SQL_HANDLE_DBC, sqlenvhandle, rc);
    return false;
  }

  rc = SQLSetEnvAttr(sqlenvhandle,SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
  if(rc != SQL_SUCCESS){
    show_error(SQL_HANDLE_DBC, sqlenvhandle, rc);
    return false;
  }

   
  rc = SQLAllocHandle(SQL_HANDLE_DBC, sqlenvhandle, &sqlconnectionhandle);
  if(rc != SQL_SUCCESS){
    show_error(SQL_HANDLE_DBC, sqlconnectionhandle, rc);
    return false;
  }

  if(i_conn_str.length() > 0){
    rc = SQLDriverConnect (sqlconnectionhandle, NULL, (SQLWCHAR*)i_conn_str.c_str(), i_conn_str.length(), NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
  }
  else{ // prompt for connection string
    HWND desktopHandle = GetDesktopWindow();

    SQLWCHAR retconstring[2048] = L"";
    rc = SQLDriverConnect (sqlconnectionhandle, desktopHandle, NULL, 0, retconstring, 2048, NULL, SQL_DRIVER_PROMPT);
    printfU(cU("conn_string:%s\n"), retconstring);
  }

  rc = SQLSetConnectAttr(sqlconnectionhandle, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF, 0);
  if(rc != SQL_SUCCESS){
    show_error(SQL_HANDLE_DBC, sqlconnectionhandle, rc);
    return false;
  }


  bool res=false;
  switch(rc){
    case SQL_SUCCESS:
      res = true;
      break;
    case SQL_SUCCESS_WITH_INFO:
      show_error(SQL_HANDLE_DBC, sqlconnectionhandle, rc);
      res = true;
      break;
    case SQL_INVALID_HANDLE:
    case SQL_ERROR:
      show_error(SQL_HANDLE_DBC, sqlconnectionhandle, rc);
      break;
    default:
      break;
  }

  return res;
}

void close_connection()
{
    SQLFreeHandle(SQL_HANDLE_DBC, sqlconnectionhandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlenvhandle);
}


int SQL2SAP_colsz(const SQLSMALLINT sql_datatype, const SQLULEN colsz)
{
  SQLULEN result = colsz;
  switch(sql_datatype){
  case SQL_FLOAT:
    {
      if(colsz > 20){
        result = 20;
      }
    }
    break;

  case SQL_TYPE_TIMESTAMP:
    {
      result = 14;
    }
    break;

  default:
    break;
  }
  return result;
}


// convert sql datatype to ABAP-dictionary data type
wstring SQLDT2SAPDT(const SQLSMALLINT sql_datatype)
{
  wstring result;
  switch(sql_datatype){
  case SQL_CHAR:
  case SQL_VARCHAR:
  case SQL_WCHAR:
  case SQL_WVARCHAR:
    result = L"CHAR";
    break;

  case SQL_SMALLINT:
  case SQL_INTEGER:
    result = L"INT4";
    break;

  case SQL_FLOAT:
    result = L"FLTP";
    break;

  case SQL_TYPE_TIMESTAMP:
    result = L"DTTM";
    break;

  case SQL_DECIMAL:
    result = L"DEC ";
    break;

  default:
    result = L"UNKN";
    break;
  }

  return result;
}



// convert sql datatype to SAP string representation
//int, nvarchar, float, datetime 
wstring SQL2SAP(const SQLHANDLE sqlh, const SQLUSMALLINT c, const SQLSMALLINT sql_datatype, const SQLULEN i_colsz, const SQLSMALLINT digits)
{
  SQLRETURN rc;
  LOG(printfU(cU("SQL2SAP:c=%d, datatype=%d, i_colsz=%d\n"), c, sql_datatype, i_colsz);)
  SQLULEN colsz_out = SQL2SAP_colsz(sql_datatype, i_colsz);
  LOG(printfU(cU("colsz_out:%d\n"), colsz_out);)
  size_t buf_len = colsz_out+1;
  wchar_t * buf = new wchar_t[buf_len];
  wmemset(buf,0,buf_len);
  wstring res;
  wchar_t * fmt = new wchar_t[256];
  wmemset(fmt,0,256);

  switch(sql_datatype){
    case SQL_CHAR:
    case SQL_VARCHAR:
    {
      swprintf(fmt, L"%%-%ds", colsz_out);
      char * ss = new char[buf_len];
      memset(ss,0,buf_len);
      rc = SQLGetData(sqlh, c, SQL_C_CHAR, ss, colsz_out, NULL);
      if(rc != SQL_SUCCESS){
        memset(ss,0,buf_len);
      }

      wchar_t * ssw = new wchar_t[buf_len];
      int mbret = MultiByteToWideChar(CP_ACP, 0, ss, -1, ssw, buf_len);
      if(mbret == 0){
        LOG(printfU(cU("MultiByteToWideChar fail!\n"));)
      }

      swprintf(buf, fmt, ssw);
      delete [] ss;
      delete [] ssw;
      LOG(printfU(cU("string:buf=%s\n"), buf);)
    }
    break;

    case SQL_WCHAR:
    case SQL_WVARCHAR:
    {
      swprintf(fmt, L"%%-%ds", colsz_out);
      wchar_t * ss = new wchar_t[buf_len];
      wmemset(ss,0,buf_len);
      rc = SQLGetData(sqlh, c, SQL_C_WCHAR, ss, colsz_out, NULL);
      LOG(printfU(cU("ss:%s\n"), ss);)
      if(rc != SQL_SUCCESS){
        wmemset(ss,0,buf_len);
      }
      swprintf(buf, fmt, ss);
      delete [] ss;
      LOG(printfU(cU("string:buf=%s\n"), buf);)
    }
    break;

    case SQL_SMALLINT:
    case SQL_INTEGER:
    {
      swprintf(fmt, L"%%0%dd", colsz_out);
      SQLINTEGER ires=0;
      rc = SQLGetData(sqlh, c, SQL_C_SLONG, &ires, sizeof(SQLINTEGER), NULL);
      if(rc != SQL_SUCCESS){
        ires=0;
      }
      swprintf(buf, fmt, ires);
      LOG(printfU(cU("integer:buf=%s\n"), buf);)
    }
    break;

    case SQL_FLOAT:
    {
      swprintf(fmt, L"%%0%df", colsz_out);
      SQLDOUBLE fres=0;
      rc = SQLGetData(sqlh, c, SQL_C_DOUBLE, &fres, sizeof(SQLDOUBLE), NULL);
      if(rc != SQL_SUCCESS){
        fres=0;
      }
      swprintf(buf, fmt, fres);
      LOG(printfU(cU("float:buf=%s\n"), buf);)
    }
    break;

    case SQL_DECIMAL:
    {
      swprintf(fmt, L"%%0%d.%df", colsz_out, digits);
      LOG(printfU(cU("decimal:fmt=%s\n"), fmt);)
      SQLDOUBLE fres=0;
      rc = SQLGetData(sqlh, c, SQL_C_DOUBLE, &fres, sizeof(SQLDOUBLE), NULL);
      if(rc != SQL_SUCCESS){
        fres=0;
      }
      LOG(printfU(cU("decimal:fres=%f\n"), fres);)
      swprintf(buf, fmt, fres);
      LOG(printfU(cU("decimal:buf=%s\n"), buf);)
    }
    break;

    case SQL_TYPE_TIMESTAMP:
    {
      SQL_TIMESTAMP_STRUCT ts;
      memset(&ts,0,sizeof(ts));
      rc = SQLGetData(sqlh, c, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts), NULL);
      if(rc != SQL_SUCCESS){
        memset(&ts,0,sizeof(ts));
      }
      swprintf(buf, L"%04d%02d%02d%02d%02d%02d", ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second);
      LOG(printfU(cU("timestamp:buf=%s\n"), buf);)
    }
    break;

    default:
    swprintf(fmt, L"%%%ds", colsz_out);
    swprintf(buf, fmt, L"unknown");
    LOG(printfU(cU("nuknown:buf=%s\n"), buf);)
    break;
  }

  res = wstring(buf);
  delete [] buf;
  delete [] fmt;
  return res;
}



bool enum_and_bind_params(const SQLHANDLE sqlstatementhandle, list<sql_param> & sql_params, SQLSMALLINT & pcount, list<wstring> & sql_messages)
{
  SQLRETURN rc = SQLNumParams(sqlstatementhandle, &pcount);
  if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO){
    show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
    return false;
  }
  if(rc == SQL_SUCCESS_WITH_INFO){
    show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
  }

  SQLINTEGER v=0;
  SQLLEN vb = 0;

  bool bind_suc = true;
  if(pcount > 0){
    for(SQLUSMALLINT pn = 1; pn <= pcount; pn++){
      SQLSMALLINT datatype, digits, nullable;
      SQLULEN plen;
      rc = SQLDescribeParam(sqlstatementhandle, pn, &datatype, &plen, &digits, &nullable);
      if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO){
        show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
        return false;
      }
      if(rc == SQL_SUCCESS_WITH_INFO){
        show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
      }

      list<sql_param>::iterator pi=sql_params.end();
      bool found=false;
      for(pi = sql_params.begin(); pi != sql_params.end(); pi++){
        if(pi->ParameterNumber == pn){
          found=true;
          break;
        }
      }
      if(found){
        SQLUSMALLINT c_type=0;
        SQLLEN param_len=0;
        void * val_ptr = pi->get_value_ptr_and_len(param_len, c_type);
        rc = SQLBindParameter(sqlstatementhandle, pn, pi->InputOutputType, c_type, pi->ParameterType, plen, digits, val_ptr, param_len, pi->get_out_ptr());
        if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO){
          show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
          bind_suc = false;
        }
        if(rc == SQL_SUCCESS_WITH_INFO){
          show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
        }
      	LOG(printfU(cU("bind parameter %d, value=%s\n"), pn, pi->to_sap_value().c_str());)
      }
      else{
      	LOG(printfU(cU("unable to find parameter with number=%d\n"), pn );)
        bind_suc=false;
        break;
      }
    }
  }
  return bind_suc;
}


bool exec_sql(const wstring sql, const unsigned int max_rows, const RFC_TABLE_HANDLE rfc_table, const RFC_TABLE_HANDLE  fi_table, list<sql_param> & sql_params,
  list<wstring> & sql_messages, unsigned int & rows)
{
  bool result=false;

  LOG(printfU(cU("call exec_sql: %s\n"), sql.c_str());)
  SQLRETURN rc = 0;
  RFC_RC rcf = RFC_OK;
  SQLHANDLE sqlstatementhandle;

  // allocate handle
  LOG(printfU(cU("call SQLAllocHandle...\n"));)
  rc = SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle);
  if(rc != SQL_SUCCESS){
    LOG(printfU(cU("rc=: %d\n"), rc);)
    show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages); 
    return false;
  }
  LOG(printfU(cU("Ok!\n"), sql.c_str());)

  // prepare
  rc = SQLPrepareW(sqlstatementhandle, (SQLWCHAR*) sql.c_str(), SQL_NTS);
  if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO){
    show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle);
    return false;
  }
  if(rc == SQL_SUCCESS_WITH_INFO){
    show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
  }

  // bind parameters
  SQLSMALLINT pcount;
  bool bind_suc = enum_and_bind_params(sqlstatementhandle, sql_params, pcount, sql_messages);


  if(bind_suc){
    rc=SQLExecute(sqlstatementhandle);
    if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO){
      show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
      result = false;
    }
    else{
      result = true;
    }
    if(rc == SQL_SUCCESS_WITH_INFO){
      show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
    }
    if(result){  // fetching...

      RFC_STRUCTURE_HANDLE tabLine = 0;
      RFC_ERROR_INFO errorInfo;

      vector<SQLSMALLINT> vdt;
      vector<SQLULEN> vfz;
      vector<SQLSMALLINT> vdi;
      SQLSMALLINT num_cols=0;
      rc = SQLNumResultCols(sqlstatementhandle,  &num_cols);
      if(rc == SQL_SUCCESS){
      	LOG(printfU(cU("number of columns: %d\n"), num_cols);)
      	vdt.resize(num_cols);
        vfz.resize(num_cols);
        vdi.resize(num_cols);
        _ULonglong f_offset=0;
	for(SQLSMALLINT c = 1; c <= num_cols; c++){
          SQLWCHAR name[1024];
	  SQLSMALLINT nlen, datatype, digits, nullable;
	  SQLULEN colsz;
          wmemset(name,0,1024);
	  rc = SQLDescribeCol(sqlstatementhandle, c, name, 1024, &nlen, &datatype, &colsz, &digits, &nullable);
          if(rc == SQL_SUCCESS){
            SQLULEN colsz_out = SQL2SAP_colsz(datatype, colsz);
            vdt[c-1]=datatype;
            vfz[c-1]=colsz_out;
            vdi[c-1]=digits;
            LOG(printfU(cU("%s\t%d\t%d\t%d\n"), name, datatype, digits, colsz);)

            tabLine = 0;
            tabLine = RfcAppendNewRow(fi_table, &errorInfo);
            if(tabLine){

              rcf = RfcSetString(tabLine, cU("FIELDNAME"), name, 1024, &errorInfo);
              errorHandling2(rcf, &errorInfo);

              wstring cs = to_wstring(_ULonglong(c));
              rcf = RfcSetString(tabLine, cU("POSITION"), cs.c_str(), cs.length(), &errorInfo);
              errorHandling2(rcf, &errorInfo);

              cs = to_wstring(f_offset);
              rcf = RfcSetString(tabLine, cU("OFFSET"), cs.c_str(), cs.length(), &errorInfo);
              errorHandling2(rcf, &errorInfo);

              cs = to_wstring(_ULonglong(colsz_out));
              rcf = RfcSetString(tabLine, cU("LENG"), cs.c_str(), cs.length(), &errorInfo);
              errorHandling2(rcf, &errorInfo);

              wstring sap_dtype = SQLDT2SAPDT(datatype);
              rcf = RfcSetString(tabLine, cU("DATATYPE"), sap_dtype.c_str(), sap_dtype.length(), &errorInfo);
              errorHandling2(rcf, &errorInfo);

              cs = to_wstring(_ULonglong(digits));
              rcf = RfcSetString(tabLine, cU("DECIMALS"), cs.c_str(), cs.length(), &errorInfo);
              errorHandling2(rcf, &errorInfo);

            }
            else{
              errorHandling2(rcf, &errorInfo);
            }
            f_offset += colsz_out;
    			}
          else{
            show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
            SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle );
            return false;
          }
	}
      }
      else{
        show_error(SQL_HANDLE_STMT, sqlstatementhandle, rc, &sql_messages);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle );
        return false;
      }

      rows = 0;
      LOG(printfU(cU("Fetching data...\n"));)
      while(SQLFetch(sqlstatementhandle)==SQL_SUCCESS){
        wstring line;
	for(SQLSMALLINT c = 1; c <= num_cols; c++){
          wstring fs = SQL2SAP(sqlstatementhandle, c, vdt[c-1], vfz[c-1], vdi[c-1]);
          line += fs;
          LOG(printfU(cU("%s\n"),line.c_str());)
        }
        tabLine = 0;
        tabLine = RfcAppendNewRow(rfc_table, &errorInfo);
        if(tabLine){
          LOG(printfU(cU("Append new row ok!\n"));)
          rcf = RfcSetString(tabLine, cU(""), line.c_str(), line.length(), &errorInfo);
          errorHandling2(rcf, &errorInfo);
        }
        else{
          LOG(printfU(cU("Append new fail!\n"));)
          errorHandling2(rcf, &errorInfo);
        }
        rows++;
        if(max_rows > 0 && rows >= max_rows) break;
      }

      // get parameter's values
      if(pcount > 0){
        for(;;){
          rc = SQLMoreResults(sqlstatementhandle);
          if(rc == SQL_NO_DATA)break;
        }
      }
    }
  }
  else{
    LOG(printfU(cU("parameters binding fail!\n"));)
    sql_messages.push_back(wstring(L"parameters binding fail!"));
    result = false;
  }

  // print out output params
  LOG(printfU(cU("Output parms:\n"));)
  for(list<sql_param>::iterator pi = sql_params.begin(); pi != sql_params.end(); pi++){
    if(pi->InputOutputType == SQL_PARAM_OUTPUT){
      LOG(printfU(cU("\t%d:%s\n"), pi->ParameterNumber, pi->to_sap_value().c_str());)
    }
  }

  SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle );

  return result;
}




wstring sql_param::to_sap_value()
{
  wchar_t * buf = new wchar_t[255];
  memset(buf,0,255*sizeof(wchar_t));
  wstring res;
  wchar_t * fmt = new wchar_t[256];
  memset(fmt,0,256*sizeof(wchar_t));

  switch(ParameterType){

    case SQL_CHAR:
    case SQL_VARCHAR:
      res = wstring(v_sqlchar.begin(), v_sqlchar.end());
      break;

    case SQL_WCHAR:
    case SQL_WVARCHAR:
      res = v_sqlwchar;
      break;

    case SQL_SMALLINT:
    case SQL_INTEGER:
      res = to_wstring(_Longlong(v_sqlinteger));
      break;
    case SQL_FLOAT:
      res = to_wstring(v_sqldouble);
      break;

    case SQL_TYPE_TIMESTAMP:
    {
      swprintf(buf, L"%04d%02d%02d%02d%02d%02d", v_timestamp.year, v_timestamp.month, v_timestamp.day, v_timestamp.hour, v_timestamp.minute, v_timestamp.second);
      res = wstring(buf);
      LOG(printfU(cU("timestamp:buf=%s\n"), buf);)
    }
    break;

	default:
    break;
	}

  delete [] buf;
  delete [] fmt;
  return res;
}


SQLRETURN do_commit(wstring & sql_mess)
{
  SQLRETURN sql_rc = SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_COMMIT);
  LOG(printfU(cU("\tdo_commit:rc=%d\n"), sql_rc);)

  if(sql_rc != SQL_SUCCESS && sql_rc != SQL_SUCCESS_WITH_INFO){
    list<wstring> sql_messages;
    show_error(SQL_HANDLE_DBC, sqlconnectionhandle, sql_rc, &sql_messages);
    if(sql_messages.size() > 0){
      sql_mess = sql_messages.front();
    }
  }
  else{
    sql_mess = L"commit ok!";
  }
  return sql_rc;
}



SQLRETURN do_rollback(wstring & sql_mess)
{
  SQLRETURN sql_rc = SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_ROLLBACK);
  LOG(printfU(cU("\tdo_rollback:rc=%d\n"), sql_rc);)

  if(sql_rc != SQL_SUCCESS && sql_rc != SQL_SUCCESS_WITH_INFO){
    list<wstring> sql_messages;
    show_error(SQL_HANDLE_DBC, sqlconnectionhandle, sql_rc, &sql_messages);
    if(sql_messages.size() > 0){
      sql_mess = sql_messages.front();
    }
  }
  else{
    sql_mess = L"rollback ok!";
  }
  return sql_rc;
}

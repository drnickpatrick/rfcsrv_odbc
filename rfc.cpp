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

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sapnwrfc.h>

#include "dbc.h"


void errorHandling(RFC_RC rc, SAP_UC* description, RFC_ERROR_INFO* errorInfo, RFC_CONNECTION_HANDLE connection)
{
  printfU(cU("%s: %d\n"), description, rc);
  printfU(cU("%s: %s\n"), errorInfo->key, errorInfo->message);
  if (connection != NULL) RfcCloseConnection(connection, errorInfo);
  exit(1);
}


RFC_RC SAP_API zSQLCommitImplementation(RFC_CONNECTION_HANDLE rfcHandle, RFC_FUNCTION_HANDLE funcHandle, RFC_ERROR_INFO* errorInfoP)
{
  RFC_ERROR_INFO errorInfo ;
  wstring sql_message;
  RFC_RC ret = RFC_OK;  
  SQLRETURN sql_rc = do_commit(sql_message);
  RFC_RC rc = RfcSetInt2(funcHandle, cU("RC"), sql_rc, &errorInfo);
  if(rc != RFC_OK){
    ret = RFC_INVALID_PARAMETER;
    errorHandling2(rc, &errorInfo);
  }
  rc = RfcSetString(funcHandle, cU("SQL_MESSAGE"), sql_message.c_str(), sql_message.length(), &errorInfo);
  if(rc != RFC_OK){
    ret = RFC_INVALID_PARAMETER;
    errorHandling2(rc, &errorInfo);
  }
  return ret;
}

RFC_RC SAP_API zSQLRollbackImplementation(RFC_CONNECTION_HANDLE rfcHandle, RFC_FUNCTION_HANDLE funcHandle, RFC_ERROR_INFO* errorInfoP)
{
  RFC_ERROR_INFO errorInfo ;
  wstring sql_message;
  RFC_RC ret = RFC_OK;  
  SQLRETURN sql_rc = do_rollback(sql_message);
  RFC_RC rc = RfcSetInt2(funcHandle, cU("RC"), sql_rc, &errorInfo);
  if(rc != RFC_OK){
    ret = RFC_INVALID_PARAMETER;
    errorHandling2(rc, &errorInfo);
  }
  rc = RfcSetString(funcHandle, cU("SQL_MESSAGE"), sql_message.c_str(), sql_message.length(), &errorInfo);
  if(rc != RFC_OK){
    ret = RFC_INVALID_PARAMETER;
    errorHandling2(rc, &errorInfo);
  }
  return ret;
}


RFC_RC SAP_API zSQLExecImplementation(RFC_CONNECTION_HANDLE rfcHandle, RFC_FUNCTION_HANDLE funcHandle, RFC_ERROR_INFO* errorInfoP)
{

  RFC_ATTRIBUTES attributes;
  RFC_TABLE_HANDLE importTab = 0;
  RFC_STRUCTURE_HANDLE tabLine = 0;
  RFC_TABLE_HANDLE exportTab = 0;
  RFC_ERROR_INFO errorInfo ;
  RFC_CHAR buffer[1024]; //One for the terminating zero
//  RFC_INT intValue;
  RFC_RC rc;
  RFC_RC ret = RFC_UNKNOWN_ERROR;
  RFC_TABLE_HANDLE sql_mess_tab = 0;
  RFC_TABLE_HANDLE params_tab = 0;
  RFC_TABLE_HANDLE o_params_tab = 0;

  list<sql_param> sql_params;
  list<wstring> sql_messages;

  unsigned int rows=0;
  RFC_INT2 max_rows=0;
  unsigned tabLen = 0, strLen;
  unsigned  i = 0;
  memset(buffer, 0, sizeof(buffer)/sizeof(RFC_CHAR));
  

  printfU(cU("\n*** Got request for zSQLExec from the following system: ***\n"));

  RfcGetConnectionAttributes(rfcHandle, &attributes, &errorInfo);
  printfU(cU("System ID: %s\n"), attributes.sysId);
  printfU(cU("System No: %s\n"), attributes.sysNumber);
  printfU(cU("Mandant  : %s\n"), attributes.client);
  printfU(cU("Host     : %s\n"), attributes.partnerHost);
  printfU(cU("User     : %s\n"), attributes.user);


//CHAR INT4 FLTP DATS TIMS DTTM supported

  rc = RfcGetTable(funcHandle, cU("RES_TAB"), &exportTab, &errorInfo);
  if(rc != RFC_OK){
    errorHandling2(rc, &errorInfo);
    return RFC_INVALID_PARAMETER;
  }

  RFC_TABLE_HANDLE fi_tab = 0;
  rc = RfcGetTable(funcHandle, cU("FIELD_INFOS"), &fi_tab, &errorInfo);
  if(rc != RFC_OK){
    errorHandling2(rc, &errorInfo);
    return RFC_INVALID_PARAMETER;
  }

  rc = RfcGetTable(funcHandle, cU("SQL_MESSAGES"), &sql_mess_tab, &errorInfo);
  if(rc != RFC_OK){
    errorHandling2(rc, &errorInfo);
    return RFC_INVALID_PARAMETER;
  }

  rc = RfcGetTable(funcHandle, cU("PARAMS"), &params_tab, &errorInfo);
  if(rc != RFC_OK){
    errorHandling2(rc, &errorInfo);
    return RFC_INVALID_PARAMETER;
  }

  rc = RfcGetTable(funcHandle, cU("O_PARAMS"), &o_params_tab, &errorInfo);
  if(rc != RFC_OK){
    errorHandling2(rc, &errorInfo);
    return RFC_INVALID_PARAMETER;
  }

  rc = RfcGetInt2(funcHandle, cU("MAX_ROWS"), &max_rows, &errorInfo);
  if(rc != RFC_OK){
    errorHandling2(rc, &errorInfo);
    return RFC_INVALID_PARAMETER;
  }

  // load parameters list
  unsigned int pcount;
  rc = RfcGetRowCount(params_tab, &pcount, &errorInfo);
  if(rc != RFC_OK){
    ret = RFC_INVALID_PARAMETER;
    errorHandling2(rc, &errorInfo);
  }

  LOG(printfU(cU("params_tab (%u lines)\n"), pcount);)

  for (unsigned int pi = 0; pi < pcount; pi++){
    rc = RfcMoveTo(params_tab, pi, &errorInfo);
    if(rc != RFC_OK){
      errorHandling2(rc, &errorInfo);
      return RFC_INVALID_PARAMETER;
    }

    LOG(printfU(cU("\t\t-line %u\n"), pi);)

    unsigned int strLen;
    RFC_INT num;
    RFC_CHAR iotype[2] = cU("");
    RFC_CHAR datatype[5] = cU("");
    
    rc = RfcGetInt(params_tab, cU("NUM"), &num, &errorInfo);
    if(rc != RFC_OK){
      errorHandling2(rc, &errorInfo);
      return RFC_INVALID_PARAMETER;
    }
    LOG(printfU(cU("\t\t-num %d\n"), num);)
    
    if(num == 0)continue;
    
    rc = RfcGetString(params_tab, cU("IOTYPE"), iotype, 2, &strLen, &errorInfo);
    if(rc != RFC_OK){
      errorHandling2(rc, &errorInfo);
      return RFC_INVALID_PARAMETER;
    }
    LOG(printfU(cU("\t\t-iotype %s\n"), iotype);)
    
    rc = RfcGetString(params_tab, cU("DATATYPE"), datatype, 5, &strLen, &errorInfo);
    if(rc != RFC_OK){
      errorHandling2(rc, &errorInfo);
      return RFC_INVALID_PARAMETER;
    }
    LOG(printfU(cU("\t\t-datatype %s\n"), datatype);)
    
    size_t buf_sz = 65535;
    RFC_CHAR* buf = new RFC_CHAR[buf_sz];
    wmemset(buf,0,buf_sz);
    rc = RfcGetString(params_tab, cU("VALUE"), buf, buf_sz, &strLen, &errorInfo);
    if(rc != RFC_OK){
      errorHandling2(rc, &errorInfo);
      delete [] buf;
      return RFC_INVALID_PARAMETER;
    }
    LOG(printfU(cU("\t\t-value %s\n"), buf);)
    
    wstring p_val(buf);
    
    delete [] buf;
    
    SQLSMALLINT param_io_type=0;
    if(wstring(iotype)==wstring(L"I")){
      param_io_type = SQL_PARAM_INPUT;
    }
    else
    if(wstring(iotype)==wstring(L"O")){
      param_io_type = SQL_PARAM_OUTPUT;
    }
    else
    if(wstring(iotype)==wstring(L"B")){
      param_io_type = SQL_PARAM_INPUT_OUTPUT;
    }
    else{
      return RFC_INVALID_PARAMETER;
    }
    
    LOG(printfU(cU("\tparam_io_type set ok, p_val=%s\n"), p_val.c_str());)
    
      
    if(wstring(datatype)==wstring(L"CHAR")){
      sql_params.push_back(sql_param(num, param_io_type, p_val, wstring(datatype) ));
    }
    else
    if(wstring(datatype)==wstring(L"INT4")){
      long int p_val_int4=0;
      try{
        p_val_int4=wcstol(p_val.c_str(),NULL,0);
      }
      catch(...){}
      sql_params.push_back(sql_param(num, param_io_type, p_val_int4, wstring(datatype) ));
    }
    else
    if(wstring(datatype)==wstring(L"FLTP")){
      double p_val_fltp=0;
      try{
        p_val_fltp=wcstod(p_val.c_str(),NULL);
      }
      catch(...){}
      sql_params.push_back(sql_param(num, param_io_type, p_val_fltp, wstring(datatype) ));
    }
    else
    if(wstring(datatype)==wstring(L"DATS")){
      DATE_STRUCT ds;
      memset(&ds, 0, sizeof(SQL_DATE_STRUCT));
      if(p_val.length() >= 8){
        try{
          ds.year  = _wtoi(p_val.substr(0,4).c_str());
          ds.month = _wtoi(p_val.substr(4,2).c_str());
          ds.day   = _wtoi(p_val.substr(6,2).c_str());
        }catch(...){}
      }
      sql_params.push_back(sql_param(num, param_io_type, ds, wstring(datatype)));
    }
    else
    if(wstring(datatype)==wstring(L"TIMS")){
      TIME_STRUCT ts;
      memset(&ts, 0, sizeof(SQL_TIME_STRUCT));
      if(p_val.length() >= 6){
        try{
          ts.hour   = _wtoi(p_val.substr(0,2).c_str());
          ts.minute = _wtoi(p_val.substr(2,2).c_str());
          ts.second = _wtoi(p_val.substr(4,2).c_str());
        }catch(...){}
      }
      sql_params.push_back(sql_param(num, param_io_type, ts, wstring(datatype)));
    }
    else
    if(wstring(datatype)==wstring(L"DTTM")){
      TIMESTAMP_STRUCT tss;
      memset(&tss, 0, sizeof(SQL_TIMESTAMP_STRUCT));
      tss.year  = _wtoi(p_val.substr(0,4).c_str());
      if(p_val.length() >= 14){
        try{
          tss.month = _wtoi(p_val.substr(4,2).c_str());
          tss.day   = _wtoi(p_val.substr(6,2).c_str());
////20141231235959
////01234567890123   
          tss.hour   = _wtoi(p_val.substr(8,2).c_str());
          tss.minute = _wtoi(p_val.substr(10,2).c_str());
          tss.second = _wtoi(p_val.substr(12,2).c_str());
        }catch(...){}
      }
      sql_params.push_back(sql_param(num, param_io_type, tss, wstring(datatype)));
    }
    else{
      return RFC_INVALID_PARAMETER;
    }
    
    printfU(cU("%d	%s	%s\n"), num, iotype, datatype);
  }// end for parameters cycle


  size_t sql_buf_sz = 65535;
  RFC_CHAR* sql_buf = new RFC_CHAR[sql_buf_sz];
  wmemset(sql_buf,0,sql_buf_sz);

  rc = RfcGetString(funcHandle, cU("SQL_TEXT"), sql_buf, sql_buf_sz, &strLen, &errorInfo);
  if(rc != RFC_OK){
    delete [] sql_buf;
    return RFC_INVALID_PARAMETER;
  }

  printfU(cU("%s\n"), sql_buf);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if(!exec_sql(sql_buf, max_rows, exportTab, fi_tab, sql_params, sql_messages, rows)){
    printfU(cU("fail call exec_sql!\n"));
    rc = RfcSetInt(funcHandle, cU("RC"), 4, &errorInfo);
    if(rc != RFC_OK){
      ret = RFC_INVALID_PARAMETER;
      errorHandling2(rc, &errorInfo);
    }

  }
  else{
    rc = RfcSetInt(funcHandle, cU("RC"), 0, &errorInfo);
    if(rc != RFC_OK){
      ret = RFC_INVALID_PARAMETER;
      errorHandling2(rc, &errorInfo);
    }
  }

  ret=RFC_OK;

  // fill o_params_tab
  for(list<sql_param>::iterator pi = sql_params.begin(); pi != sql_params.end(); pi++){
    printfU(cU("output param:%d\n"),pi->ParameterNumber);
    if(pi->InputOutputType == SQL_PARAM_OUTPUT || pi->InputOutputType == SQL_PARAM_INPUT_OUTPUT){
      RFC_STRUCTURE_HANDLE tabline = 0;
      RFC_RC rcf = RFC_OK;
      tabline = RfcAppendNewRow(o_params_tab, &errorInfo);
      if(tabline){
        rcf = RfcSetInt(tabline, cU("NUM"), pi->ParameterNumber, &errorInfo);
        if(rcf != RFC_OK){
          errorHandling2(rcf, &errorInfo);
          ret = RFC_INVALID_PARAMETER;
          break;
        }
        wstring v_iotype;
        if(pi->InputOutputType == SQL_PARAM_OUTPUT) v_iotype = L"O";
        else v_iotype = L"B";
        rcf = RfcSetString(tabline, cU("IOTYPE"), v_iotype.c_str(), v_iotype.length(), &errorInfo);
        if(rcf != RFC_OK){
          errorHandling2(rcf, &errorInfo);
          ret = RFC_INVALID_PARAMETER;
          break;
        }

        rcf = RfcSetString(tabline, cU("DATATYPE"), pi->sap_datatype.c_str(), pi->sap_datatype.length(), &errorInfo);
        if(rcf != RFC_OK){
          errorHandling2(rcf, &errorInfo);
          ret =  RFC_INVALID_PARAMETER;
          break;
        }
        wstring sap_val = pi->to_sap_value();
        rcf = RfcSetString(tabline, cU("VALUE"), sap_val.c_str(), sap_val.length(), &errorInfo);
        if(rcf != RFC_OK){
          errorHandling2(rcf, &errorInfo);
          ret = RFC_INVALID_PARAMETER;
          break;
        }

      }
      else{
        LOG(printfU(cU("unable to append new row to o_params_tab\n"));)
        sql_messages.push_back(wstring(L"unable to append new row to o_params_tab"));
        ret = RFC_INVALID_PARAMETER;
        break;
      }

    }
  }
  LOG(printfU(cU("sql_params list loop ok\n"));)



  RFC_RC rcf = RFC_OK;
  RFC_STRUCTURE_HANDLE sql_mess_tabline = 0;

  for(list<wstring>::const_iterator i = sql_messages.begin(); i != sql_messages.end(); i++){
    LOG(printfU(cU("sql_message:%s\n"), i->c_str());)
    sql_mess_tabline = 0;
    sql_mess_tabline = RfcAppendNewRow(sql_mess_tab, &errorInfo);
    if(sql_mess_tabline){
      rcf = RfcSetString(sql_mess_tabline, cU(""), i->c_str(), i->length(), &errorInfo);
      if(rcf != RFC_OK){
        errorHandling2(rcf, &errorInfo);
        ret = RFC_INVALID_PARAMETER;
        break;
      }
    }
    else{
      LOG(printfU(cU("unable to append new row to o_sql_mess_tab\n"));)
      sql_messages.push_back(wstring(L"unable to append new row to sql_mess_tab"));
      ret = RFC_INVALID_PARAMETER;
      break;
    }
  }
  LOG(printfU(cU("sql_messages fill ok\n"));)

  rc = RfcSetInt2(funcHandle, cU("ROWS"), rows, &errorInfo);
  if(rc != RFC_OK){
    ret = RFC_INVALID_PARAMETER;
    errorHandling2(rc, &errorInfo);
  }

  LOG(printfU(cU("set ROWS ok\n"));)

  delete [] sql_buf;

  return ret;
}

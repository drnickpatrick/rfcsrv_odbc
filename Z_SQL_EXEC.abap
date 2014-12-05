function Z_SQL_EXEC.
*"----------------------------------------------------------------------
*"  IMPORTING
*"     VALUE(SQL_TEXT) TYPE  STRING
*"     VALUE(PARAMS) TYPE  Z_SQL_PARAM_TT OPTIONAL
*"     VALUE(MAX_ROWS) TYPE  I OPTIONAL
*"  EXPORTING
*"     VALUE(RC) TYPE  I
*"     VALUE(SQL_MESSAGES) TYPE  /SDF/RBE_NATSQL_RESTAB_TT
*"     VALUE(ROWS) TYPE  I
*"     VALUE(FIELD_INFOS) TYPE  /SDF/RBE_NATSQL_SDD_FIELD_TT
*"     VALUE(RES_TAB) TYPE  /SDF/RBE_NATSQL_RESTAB_TT
*"     VALUE(O_PARAMS) TYPE  Z_SQL_PARAM_TT
*"----------------------------------------------------------------------

append 'This is a stub' to sql_messages.
rows = 1.


endfunction.

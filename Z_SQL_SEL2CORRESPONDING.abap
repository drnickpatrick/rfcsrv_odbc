FUNCTION Z_SQL_SEL2CORRESPONDING.
*"----------------------------------------------------------------------
*"  IMPORTING
*"     VALUE(SQL) TYPE  STRING
*"     VALUE(RFC_DEST) TYPE  STRING
*"     VALUE(PARAMS) TYPE  Z_SQL_PARAM_TT OPTIONAL
*"  EXPORTING
*"     VALUE(RC) TYPE  I
*"     VALUE(SQL_MESSAGES) TYPE  /SDF/RBE_NATSQL_RESTAB_TT
*"     VALUE(O_PARAMS) TYPE  Z_SQL_PARAM_TT
*"  TABLES
*"      T TYPE  STANDARD TABLE
*"----------------------------------------------------------------------

  data: res type /SDF/RBE_NATSQL_RESTAB_TT,
        fi TYPE /SDF/RBE_NATSQL_SDD_FIELD_TT,
        fi_wa TYPE /SDF/RBE_NATSQL_SDD_FIELD,
        s TYPE string.

  FIELD-SYMBOLS: <fs> TYPE any,
                 <wa> TYPE any,
                 <fs2> TYPE any.

  DATA: lv_line     TYPE REF TO data.
  CREATE DATA lv_line LIKE LINE OF t.
  ASSIGN lv_line->* to <wa>.
  CLEAR t[].
  CALL FUNCTION 'Z_SQL_EXEC' DESTINATION RFC_DEST
    EXPORTING
      PARAMS            = PARAMS
      SQL_TEXT          = sql
      MAX_ROWS          = 0
   IMPORTING
       RC                 = rc
       SQL_MESSAGES       = sql_messages
*       ROWS              =
*       RUNTIME           =
     O_PARAMS          = O_PARAMS
     FIELD_INFOS       = fi
     RES_TAB           = res.

  if rc <> 0.
    exit.
  endif.

  LOOP at fi INTO fi_wa.
    TRANSLATE fi_wa-fieldname TO UPPER CASE.
    MODIFY fi FROM fi_wa.
  ENDLOOP.

  LOOP at res INTO s.
    clear <wa>.
    UNASSIGN <fs>.
    LOOP at fi INTO fi_wa.
      case fi_wa-datatype.
        when 'DTTM'.
          data fn2 LIKE fi_wa-fieldname.
          CONCATENATE fi_wa-fieldname '_DT' INTO fn2.
          ASSIGN COMPONENT fn2 OF STRUCTURE <wa> to <fs>.
          if <fs> is assigned.
            <fs> = s+fi_wa-offset(8).
            UNASSIGN <fs>.
          endif.
          CONCATENATE fi_wa-fieldname '_TM' INTO fn2.
          ASSIGN COMPONENT fn2 OF STRUCTURE <wa> to <fs>.
          if <fs> is assigned.
            data offs2 LIKE fi_wa-offset.
            offs2 = fi_wa-offset + 8.
            <fs> = s+offs2(6).
            UNASSIGN <fs>.
          endif.

        when 'INT4'.
          ASSIGN COMPONENT fi_wa-fieldname OF STRUCTURE <wa> to <fs>.
          data: v_i4 TYPE i.
          if <fs> is assigned.
            v_i4 = s+fi_wa-offset(fi_wa-leng).
            <fs> = v_i4.
          endif.

        when 'FLTP'.
          ASSIGN COMPONENT fi_wa-fieldname OF STRUCTURE <wa> to <fs>.
          data: v_f TYPE f.
          if <fs> is assigned.
            v_f = s+fi_wa-offset(fi_wa-leng).
            <fs> = v_f.
          endif.

        when 'DEC '.
          DATA: p_ref  TYPE REF TO data.
          CREATE DATA p_ref TYPE P DECIMALS fi_wa-decimals.
          ASSIGN p_ref->* to <fs2>.
          ASSIGN COMPONENT fi_wa-fieldname OF STRUCTURE <wa> to <fs>.
          if <fs> is assigned.
            <fs2> = s+fi_wa-offset(fi_wa-leng).
            <fs> = <fs2>.
          endif.
          UNASSIGN <fs2>.

        when 'CHAR'.
          ASSIGN COMPONENT fi_wa-fieldname OF STRUCTURE <wa> to <fs>.
          if <fs> is assigned.
            <fs> = s+fi_wa-offset(fi_wa-leng).
          endif.
      ENDCASE.
      UNASSIGN <fs>.
*          ASSIGN COMPONENT fi_wa-fieldname OF STRUCTURE <wa> to <fs>.
*          if <fs> is assigned.
*            <fs> = s+fi_wa-offset(fi_wa-leng).
*          endif.

    ENDLOOP.
    append <wa> to t.
  ENDLOOP.


ENDFUNCTION.

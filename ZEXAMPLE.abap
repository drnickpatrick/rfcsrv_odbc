* example usage of rfcsrv_odbc for northwind dtabase
* we assume that RFC destination to communicate with RFC server is ZRFCDEST (see README for details)
report zexample.

  data: 
  begin of gt_orders occurs 0.
    OrderID type i,
    CustomerID(5) type c,
    EmployeeID type i,
    OrderDate_DT type d, " << in database there is only one column OrderDate of type datetime, but there is no datetime type in ABAP, so, we split it
    OrderDate_TM type t, " FM Z_SQL_SEL2CORRESPONDING automatically recognizes suffixes _DT & _TM.
    Freight(18) type P decimals 2.
  end of gt_orders.


  data: sql_rc TYPE i, sql_mess_t TYPE /SDF/RBE_NATSQL_RESTAB_TT, sql_s TYPE string.


* 1 simple select 
  CALL FUNCTION 'Z_SQL_SEL2CORRESPONDING'
    EXPORTING
      rfc_dest           = 'ZRFCDEST'
      SQL                = 'select * from orders'
   IMPORTING
     RC                 = sql_rc
     SQL_MESSAGES       = sql_mess_t
    TABLES
      T                  = gt_orders.

  if sql_rc <> 0.
    READ TABLE sql_mess_t INTO sql_s INDEX 1.
    MESSAGE sql_s TYPE 'E'.
    exit.
  endif.


  data: wa like line of gt_orders.
  loop at gt_SFLIGHT into wa.
    write:/ OrderID, CustomerID, EmployeeID, OrderDate_DT, OrderDate_TM, Freight.
  endloop.
  
    
* 2 select with parameter
  data: params type table of Z_SQL_PARAM, p_wa type Z_SQL_PARAM.

  p_wa-NUM = '1'. " <-number of parameter
  p_wa-IOTYPE = 'I'. " <-type of parameter (I-in, O-out, B-both input and output)
  p_wa-DATATYPE = 'INT4'. " <- sap datatype of parameter
  p_wa-VALUE = 1. " <-- parameter's value
  APPEND p_wa to params.

  CALL FUNCTION 'Z_SQL_SEL2CORRESPONDING'
    EXPORTING
      rfc_dest           = 'ZRFCDEST'
      SQL                = 'select * from orders where OrderID=?'
   IMPORTING
     RC                 = sql_rc
     SQL_MESSAGES       = sql_mess_t
    TABLES
      T                  = gt_orders.

  if sql_rc <> 0.
    READ TABLE sql_mess_t INTO sql_s INDEX 1.
    MESSAGE sql_s TYPE 'E'.
    exit.
  endif.


  loop at gt_SFLIGHT into wa.
    write:/ OrderID, CustomerID, EmployeeID, OrderDate_DT, OrderDate_TM, Freight.
  endloop.


* 3 execute stored procedure
*CREATE PROCEDURE [dbo].[CustOrdersDetail] @OrderID int
*AS
*SELECT ProductName,
*    UnitPrice=ROUND(Od.UnitPrice, 2),
*    Quantity,
*    Discount=CONVERT(int, Discount * 100), 
*    ExtendedPrice=ROUND(CONVERT(money, Quantity * (1 - Discount) * Od.UnitPrice), 2)
*FROM Products P, [Order Details] Od
*WHERE Od.ProductID = P.ProductID and Od.OrderID = @OrderID


  data: 
  begin of gt_CustOrdersDetail occurs 0.
    ProductName type string,
    UnitPrice(18) type P decimals 2.
    Quantity(10) type p decimals 3,
    Discount type i,
    ExtendedPrice(18) type P decimals 2.
  end of gt_CustOrdersDetail.

  data: wa2 like line of gt_CustOrdersDetail.

  CALL FUNCTION 'Z_SQL_SEL2CORRESPONDING'
    EXPORTING
      PARAMS             = params " <- we use the same params as for example 2
      rfc_dest           = 'ZRFCDEST'
      SQL                = '{call CustOrdersDetail(?)}'
   IMPORTING
     RC                 = sql_rc
     SQL_MESSAGES       = sql_mess_t
    TABLES
      T                  = gt_orders.

  if sql_rc <> 0.
    READ TABLE sql_mess_t INTO sql_s INDEX 1.
    MESSAGE sql_s TYPE 'E'.
    exit.
  endif.

  loop at gt_CustOrdersDetail into wa2.
    write:/ ProductName, UnitPrice(18), Quantity(10), Discount, ExtendedPrice(18).
  endloop.
    
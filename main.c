#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sapnwrfc.h>

#include "dbc.h"
#include "rfc.h"


static int listening = 1;

#if defined(SAPonOS400) || defined(SAPwithPASE400)
	/* o4fprintfU, o4fgetsU
	 * calling o4xxxU instead of xxxU produces much smaller code,
	 * because it directly expands to xxxU16, while xxxU expands to
	 * as4_xxx  which links against the whole pipe check and handling for ILE.
     * This creates an executable containing almost the whole platform 
 	 * specific kernel and needs the ILE O4PRTLIB in a special library.
	 * Because we have no pipe usage of fxxxU here I use o4xxxU.
	 * ATTENTION:
	 * In any case the above mentioned functions are efectively used for 
	 * pipes, the redefinition below corrupts this functionality
	 * because than the pipe handling is not called for our platform.
	 */
	#undef fprintfU
	#define fprintfU o4fprintfU
	#undef fgetsU
	#define fgetsU o4fgetsU
#endif

/*
Unfortunately SE37 does not yet allow to set complex inputs, so in order to test
this example, you will probably need to write a little ABAP report, which sets a few
input lines for IMPORT_TAB and then does a

CALL FUNCTION 'STFC_DEEP_TABLE' DESTINATION 'MY_SERVER'
  EXPORTING
    import_tab       = imtab
  IMPORTING
    export_tab       = extab
    resptext         = text
  EXCEPTIONS
    system_failure          = 1  MESSAGE mes.
    communication_failure   = 2  MESSAGE mes.

This also allows to catch the detail texts for SYSTEM_FAILURES.
Note: STFC_DEEP_TABLE exists only from SAP_BASIS release 6.20 on.
*/

/*void errorHandling2(RFC_RC rc, RFC_ERROR_INFO* errorInfo){
  printfU(cU("%s: %s\n"), errorInfo->key, errorInfo->message);
} */



int mainU(int argc, SAP_UC** argv)
{
  if(argc > 2){
    if(!init_connection(argv[2])){
      return -1;
    }
  }
  else{
    if(argc > 1){
      if(!init_connection(wstring())){
        return -1;
      }
    }
    else{
      printfU(cU("usage: frcsrv_odbc.exe <DEST> [CONN_STR]\n"));
      printfU(cU("\tDEST - destination label in sapnwrfc.ini\n"));
      printfU(cU("\tCONN_STR - connection string"));
      return -1;
    }
  }
  wstring dest = argv[1];

	RFC_RC rc;
	RFC_FUNCTION_DESC_HANDLE zSQLExec, zSQLCommit, zSQLRollback;
//	RFC_CONNECTION_PARAMETER repoCon[8], serverCon[3];
	RFC_CONNECTION_PARAMETER repoCon[4], serverCon[3];
	RFC_CONNECTION_HANDLE repoHandle, serverHandle;
	RFC_ERROR_INFO errorInfo;

/*	serverCon[0].name = cU("program_id");	serverCon[0].value = cU("MY_SERVER");
	serverCon[1].name = cU("gwhost");		serverCon[1].value = cU("hostname");
	serverCon[2].name = cU("gwserv");		serverCon[2].value = cU("sapgw53");*/

	serverCon[0].name = cU("DEST");	serverCon[0].value = dest.c_str();// cU("ATD");

/*
	repoCon[0].name = cU("client");	repoCon[0].value = cU("000");
	repoCon[1].name = cU("user");		repoCon[1].value = cU("user");
	repoCon[2].name = cU("passwd");	repoCon[2].value = cU("****");
	repoCon[3].name = cU("lang");		repoCon[3].value = cU("DE");
	repoCon[4].name = cU("ashost");	repoCon[4].value = cU("hostname");
	repoCon[5].name = cU("sysnr");	repoCon[5].value = cU("53");*/


	repoCon[0].name = cU("DEST");	repoCon[0].value = dest.c_str();// cU("ATD");
//	repoCon[1].name = cU("passwd");	repoCon[1].value = cU("AtlantTS");

	printfU(cU("Logging in..."));
//	repoHandle = RfcOpenConnection (repoCon, 6, &errorInfo);
	repoHandle = RfcOpenConnection (repoCon, 1, &errorInfo);
	if (repoHandle == NULL) errorHandling(errorInfo.code, cU("Error in RfcOpenConnection()"), &errorInfo, NULL);
	printfU(cU(" ...done\n"));

	printfU(cU("Fetching metadata..."));
	zSQLExec = RfcGetFunctionDesc(repoHandle, cU("Z_SQL_EXEC"), &errorInfo);
	// Note: STFC_DEEP_TABLE exists only from SAP_BASIS release 6.20 on
	if (zSQLExec == NULL) errorHandling(errorInfo.code, cU("Error in Repository Lookup Z_SQL_EXEC"), &errorInfo, repoHandle);

	zSQLCommit = RfcGetFunctionDesc(repoHandle, cU("Z_SQL_COMMIT"), &errorInfo);
	if (zSQLCommit == NULL) errorHandling(errorInfo.code, cU("Error in Repository Lookup Z_SQL_COMMIT"), &errorInfo, repoHandle);

	zSQLRollback = RfcGetFunctionDesc(repoHandle, cU("Z_SQL_ROLLBACK"), &errorInfo);
	if (zSQLRollback == NULL) errorHandling(errorInfo.code, cU("Error in Repository Lookup Z_SQL_ROLLBACK"), &errorInfo, repoHandle);

	printfU(cU(" ...done\n"));

	printfU(cU("Logging out..."));
	RfcCloseConnection(repoHandle, &errorInfo);
	printfU(cU(" ...done\n"));

	rc = RfcInstallServerFunction(NULL, zSQLExec, zSQLExecImplementation, &errorInfo);
	if (rc != RFC_OK) errorHandling(rc, cU("Error Setting zSQLExecImplementation "), &errorInfo, repoHandle);

	rc = RfcInstallServerFunction(NULL, zSQLCommit, zSQLCommitImplementation, &errorInfo);
	if (rc != RFC_OK) errorHandling(rc, cU("Error Setting zSQLCommitImplementation "), &errorInfo, repoHandle);

	rc = RfcInstallServerFunction(NULL, zSQLRollback, zSQLRollbackImplementation, &errorInfo);
	if (rc != RFC_OK) errorHandling(rc, cU("Error Setting zSQLRollbackImplementation "), &errorInfo, repoHandle);

	printfU(cU("Registering Server..."));
//	serverHandle = RfcRegisterServer(serverCon, 3, &errorInfo);
	serverHandle = RfcRegisterServer(serverCon, 1, &errorInfo);
	if (serverHandle == NULL) errorHandling(errorInfo.code, cU("Error Starting RFC Server"), &errorInfo, NULL);
	printfU(cU(" ...done\n"));

	printfU(cU("Starting to listen...\n\n"));
	while(RFC_OK == rc || RFC_RETRY == rc || RFC_ABAP_EXCEPTION == rc){
		rc = RfcListenAndDispatch(serverHandle, 120, &errorInfo);
		printfU(cU("RfcListenAndDispatch() returned %s\n"), RfcGetRcAsString(rc));
		switch (rc){
			case RFC_OK:
				break;
			case RFC_RETRY:	// This only notifies us, that no request came in within the timeout period.
						    // We just continue our loop.
				printfU(cU("No request within 120s.\n"));
				break;
			case RFC_ABAP_EXCEPTION:	// Our function module implementation has returned RFC_ABAP_EXCEPTION.
								// This is equivalent to an ABAP function module throwing an ABAP Exception.
								// The Exception has been returned to R/3 and our connection is still open.
								// So we just loop around.
				printfU(cU("ABAP_EXCEPTION in implementing function: %s\n"), errorInfo.key);
				break;
			case RFC_NOT_FOUND:	// R/3 tried to invoke a function module, for which we did not supply
							    // an implementation. R/3 has been notified of this through a SYSTEM_FAILURE,
							    // so we need to refresh our connection.
				printfU(cU("Unknown function module: %s\n"), errorInfo.message);
            /*FALLTHROUGH*/
            case RFC_EXTERNAL_FAILURE:	// Our function module implementation raised a SYSTEM_FAILURE. In this case
								        // the connection needs to be refreshed as well.
				printfU(cU("SYSTEM_FAILURE has been sent to backend.\n\n"));
            /*FALLTHROUGH*/
			case RFC_COMMUNICATION_FAILURE:
			case RFC_ABAP_MESSAGE:		// And in these cases a fresh connection is needed as well
            default:
                serverHandle = RfcRegisterServer(serverCon, 3, &errorInfo);
				rc = errorInfo.code;
				break;
		}

		// This allows us to shutdown the RFC Server from R/3. The implementation of STFC_DEEP_TABLE
		// will set listening to false, if IMPORT_TAB-C == STOP.
		if (!listening){
			RfcCloseConnection(serverHandle, NULL);
			break;
		}
	}

  close_connection();
	return 0;
}

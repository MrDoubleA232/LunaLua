#include <windows.h>
#include "ErrorReporter.h"
#include <dbghelp.h>
#include "../Globals.h"
#include "../GlobalFuncs.h"
#include "../Defines.h"
#include <array>
#include "Gui/GuiCrashNotify.h"

std::string lastErrDesc;
ErrorReport::VB6ErrorCode lastVB6ErrCode;
CONTEXT lastVB6ErrContext;

std::string ErrorReport::generateStackTrace(CONTEXT* context)
{
    CustomStackTracer cst;
    cst.ShowCallstack(GetCurrentThread(), context);
    return cst.theOutput.str();
}

void ErrorReport::writeErrorLog(const std::string &text)
{
    std::string smbxPath = gAppPathUTF8;
    smbxPath += "\\logs\\ERROR_";
    smbxPath += generateTimestampForFilename();
    smbxPath += ".txt";

    writeFile(text, smbxPath);
}


std::string ErrorReport::getCustomVB6ErrorDescription(VB6ErrorCode errCode)
{
    std::string errDesc = "";
    errDesc += "VB6 Error Code: " + std::to_string(static_cast<int>(errCode));

    switch (errCode)
    {
    case ErrorReport::VB6ERR_INVCALLARG:
        errDesc += " (Invalid call or argument)\n";
        break;
    case ErrorReport::VB6ERR_OVERFLOW:
        errDesc += " (Overflow)\n";
        break;
    case ErrorReport::VB6ERR_OUTOFMEMORY:
        errDesc += " (Out of Memory)\n";
        break;
    case ErrorReport::VB6ERR_OUTOFRANGE:
        errDesc += " (Subscript out of range)\n";
        break;
    case ErrorReport::VB6ERR_DIVBYZERO:
        errDesc += " (Division by zero)\n";
        break;
    case ErrorReport::VB6ERR_TYPEMISMATCH:
        errDesc += " (Type mismatch)\n";
        break;
    case ErrorReport::VB6ERR_FILENOTFOUND:
        errDesc += " (File not found)\n";
        break;
    case ErrorReport::VB6ERR_INPUTPASTEOF:
        errDesc += " (Input past end of file)\n";
        break;
    case ErrorReport::VB6ERR_PATHNOTFOUND:
        errDesc += " (Path not found)\n";
        break;
    case ErrorReport::VB6ERR_OBJVARNOTSET:
        errDesc += " (Object variable not set)\n";
        break;
    default:
        errDesc += "\n";
        break;
    }
    return errDesc;
}

void ErrorReport::manageErrorReport(const std::string &url, std::string &errText)
{
    GuiCrashNotify notifier(errText);
    notifier.show();

    const std::string& username = notifier.getUsername();
    const std::string& usercomment = notifier.getUsercomment();

    errText += "\n\n\nUSERNAME: \n";
    errText += (username.length() == 0 ? "(NONE)" : username);
    errText += "\n\n\nUSERCOMMENT: \n";
    errText += (usercomment.length() == 0 ? "(NONE)" : usercomment);
    errText += "\n";

    if (notifier.shouldSend()){
        sendPUTRequest(url, errText);
    }
}

static_assert(EXCEPTION_FLT_INEXACT_RESULT == 0xc000008f, "BOO");

void ErrorReport::SnapshotError(EXCEPTION_RECORD* exception, CONTEXT* context)
{
    bool isVB6Exception = (exception->ExceptionCode == EXCEPTION_FLT_INEXACT_RESULT);
    std::string stackTrace = generateStackTrace(isVB6Exception ? &lastVB6ErrContext : context);
    std::stringstream fullErrorDescription;

    fullErrorDescription << "== Crash Summary ==\n";
    fullErrorDescription << "LunaLua Version: " + std::string(LUNALUA_VERSION) + "\n";
#ifdef __clang__
    fullErrorDescription << "This LunaLua build has been compiled with Clang. Support for this compiler is still experimental so errors might happen.\n";
#endif
    fullErrorDescription << "Exception Code: 0x" << std::hex << exception->ExceptionCode;

    switch (exception->ExceptionCode)
    {
    case EXCEPTION_FLT_INEXACT_RESULT:
        fullErrorDescription << " (VB Error)\n";
        break;
    case EXCEPTION_ACCESS_VIOLATION:
        fullErrorDescription << " (Access Violation)\n";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        fullErrorDescription << " (Stack Overflow)\n";
        break;
    default:
        fullErrorDescription << "\n";
        break;
    }

    if (isVB6Exception) {
        fullErrorDescription << getCustomVB6ErrorDescription(lastVB6ErrCode);
    }

    fullErrorDescription << "\n== Stack Trace ==\n";
    fullErrorDescription << stackTrace;

    fullErrorDescription << "\n== Reporting ==\n";
    fullErrorDescription << "If you like to help us finding the error then please post this log at:\n";
    fullErrorDescription << "* http://wohlsoft.ru/forum/ or\n";
    fullErrorDescription << "* http://www.supermariobrosx.org/forums/viewforum.php?f=35 or\n";
    fullErrorDescription << "* http://talkhaus.raocow.com/viewforum.php?f=36\n";
    fullErrorDescription << "\n";

    lastErrDesc = fullErrorDescription.str();
}

void ErrorReport::report()
{
    manageErrorReport("http://wohlsoft.ru/LunaLuaErrorReport/index.php", lastErrDesc);
    writeErrorLog(lastErrDesc);
}

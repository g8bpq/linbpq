

1 VERSIONINFO
 FILEVERSION Vers
 PRODUCTVERSION Vers
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x2L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "080904b0"
        BEGIN
            VALUE "Comments", VerComments
            VALUE "CompanyName", "G8BPQ\0"
            VALUE "FileDescription", VerDesc
            VALUE "FileVersion", Verstring
 //           VALUE "InternalName", "bpq32\0"
            VALUE "LegalCopyright", VerCopyright
//            VALUE "OriginalFilename", "bpq32.dll\0"
            VALUE "ProductName", "BPQ32\0"
            VALUE "ProductVersion", Verstring
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x809, 1200
    END
END

#define BPQICON 2

BPQICON             ICON    DISCARDABLE     "..\\bpqicon.ico"

#ifdef MAILCHAT

IDD_ABOUTBOX DIALOG DISCARDABLE  22, 17, 202, 69
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "About"
FONT 8, "System"
BEGIN
    ICON            107,BPQICON,14,9,20,20
    LTEXT           "G8BPQ Mail Version ",-1,45,15,101,8,
                    SS_NOPREFIX | NOT WS_GROUP
    LTEXT           Verstring,-1,115,15,58,8,NOT WS_GROUP
    LTEXT           VerCopyright,-1,10,32,180,13,NOT WS_GROUP | ES_CENTER   
                    
    DEFPUSHBUTTON   "OK",IDOK,88,51,30,11,BS_CENTER | BS_VCENTER | WS_GROUP
END

#endif



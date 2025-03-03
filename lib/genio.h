/*****************************************************************
GENIO.H
*****************************************************************/

/*****************************************************************
Defines
*****************************************************************/
#define CURSES_ESC_KEY     0x1b

/*****************************************************************
External Routine Prototypes
*****************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void InitScreen();
void ErrorMsg( char const *pszFmt, ... );
void WarningMsg( char const *pszFmt, ... );
bool GetYesNo( char const *pszFmt, ... );

void GetNumber( char *pszResp, size_t sMaxRep, char const *pszPrompt, char const *pszFmt, ... );

void PrintAt( int nRow, int nCol, char const *psz, ... );

#ifdef __cplusplus
};
#endif

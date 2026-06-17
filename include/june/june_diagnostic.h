#ifndef JUNE_JUNE_DIAGNOSTIC_H
#define JUNE_JUNE_DIAGNOSTIC_H

//-----------------------------------------------------------------------------
// [SECTION]
//-----------------------------------------------------------------------------

typedef char *DiagnosticCode;
typedef char *DiagnosticMessage;
typedef char *DiagnosticSuggestion;

//-----------------------------------------------------------------------------
// [SECTION]
//-----------------------------------------------------------------------------

typedef struct
{
    char *file; /** */
    int line;   /** */
    int column; /** */
} DiagnosticLocation;

typedef enum
{
    DIAG_INFO,    /** */
    DIAG_NOTE,    /** */
    DIAG_ERROR,   /** */
    DIAG_IGNORE,  /** */
    DIAG_WARNING, /** */
} DiagnosticSeverity;

typedef struct
{
    DiagnosticCode code;             /** */
    DiagnosticMessage message;       /** */
    DiagnosticSeverity severity;     /** */
    DiagnosticLocation location;     /** */
    DiagnosticSuggestion suggestion; /** */
} Diagnostic;

//-----------------------------------------------------------------------------
// [SECTION]
//-----------------------------------------------------------------------------

typedef struct
{
} DiagnosticEmitter;

//-----------------------------------------------------------------------------
// [SECTION]
//-----------------------------------------------------------------------------

#endif // JUNE_JUNE_DIAGNOSTIC_H

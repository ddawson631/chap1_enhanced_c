/*****************************************************************
 *                     DECLARATIONS                              *
 *****************************************************************/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

jmp_buf JL99;

#define NAMELENG 20     // Max length of a name

#ifdef TESTSIZE
#define MAXNAMES  28    // Max no. of names to test err_max_names
#define MAXINPUT  50    // Max input length to test err_input_len
#else
#define MAXNAMES 100    // Max number of different names
#define MAXINPUT 500    // Max length of an input
#endif

#define MAXCMDLENG 8    // Max length of a command name
#define MAXCMDS 3       // Max number of different commands

#define ARGLENG 40      // Maximum length of a command argument

#define MAXDIGITS 19    // Max digits in LONG_MAX in limits.h
                        // for __WORDSIZE == 64

#define TAB 9           
#define LF 10
#define CR 13

#define PROMPT      "-> "  // Initial prompt
#define PROMPT2     "> "   // continuation prompt
#define COMMENTCHAR '!'
#define SPACE       ' '
#define DOLLAR      '$'    // marks END of expr or fundef input by user
#define RPAREN      ')'    // marks beginning of a Cmd

typedef short int NAMESIZE;
typedef char *NAMESTRING;
typedef short int NAME;

typedef short int CMDSIZE;
typedef char *CMDSTRING;
typedef short int CMD;

typedef long NUMBER;
typedef short int ARGSIZE;

typedef enum {false, true} BOOLEAN;

typedef enum {IFOP,WHILEOP,ASSIGNOP,SEQOP,ADDOP,SUBOP,
              MULOP,DIVOP,EQOP,LTOP,GTOP,PRINTOP} BUILTINOP;
                
typedef enum {nameidsy=1,numsy,funidsy,ifsy,thensy,elsesy,fisy,whilesy,dosy,
              odsy,seqsy,qessy,funsy,nufsy,assignsy,rparsy,
              lparsy,semsy,comsy,addsy,subsy,mulsy,divsy,eqsy,lssy,
              gtsy,printsy,quitsy,dollarsy} TOKEN; 
             
typedef enum {err_arglist=1, err_function, err_exp6, err_expr, err_cwd,
              err_open, err_max_names, err_input_len, err_cmd_len, err_no_cmd,
              err_bad_cmd, err_arg_len, err_no_name, err_name_len,
              err_no_name2, err_digits, err_no_name3, err_mismatch,
              err_not_var, err_num_args, err_undef_func, err_num_args2,
              err_undef_var, err_undef_op, err_nested_load
             } ERROR_NUM; // error codes passed to errmsg()

typedef struct EXPREC* EXP;
typedef struct EXPLISTREC* EXPLIST;
typedef struct ENVREC* ENV;
typedef struct VALUELISTREC* VALUELIST;
typedef struct NAMELISTREC* NAMELIST;
typedef struct FUNDEFREC* FUNDEF;


enum EXPTYPE {VALEXP,VAREXP,APEXP};

struct EXPREC {
    enum EXPTYPE etype;
    union {
        NUMBER num;
        NAME varble;
        struct {
            NAME optr;
            EXPLIST args;
        } ap;
    } u;
};

struct EXPLISTREC {
    EXP head;
    EXPLIST tail;    
};

struct VALUELISTREC {
    NUMBER head;
    VALUELIST tail;    
};

struct NAMELISTREC {
    NAME head;
    NAMELIST tail;
};

struct ENVREC {
    NAMELIST vars;
    VALUELIST values;
};

struct FUNDEFREC {
    NAME     funname;
    NAMELIST formals;
    EXP      body;
    FUNDEF   nextfundef;
};

FUNDEF fundefs;
NUMBER numval;
ENV globalEnv;
EXP currentExp;

short int j, inputleng, pos;

NAMESTRING printNames[MAXNAMES];  // built-in & user-defined names
CMDSTRING  printCmds[MAXCMDS];    // built-in command names

char userinput[MAXINPUT];
char punctop[] = "()+-*/:=<>;,$!";  // punctuation and operator chars
char null_str[] = "\0";             // default string passed to errmsg()
int null_int = 0;                   // default int passed to errmsg()

char tokstring[NAMELENG+1];  // token string for display in error messages
NAMESIZE tokleng;

CMD load,        // load file, echo characters
    sload,       // silently load file, no echo
    user;        // print user defined names

NAME numNames, numCmds, numBuiltins, tokindex, mulsy_index, 
     // initNames() saves index of first/last ControlOp & ValueOp 
     // in following variables for checking which range Op is in.
     firstValueOp, lastValueOp, firstControlOp, lastControlOp;

char infilename[ARGLENG];
FILE *infp;     // input source file pointer

TOKEN toksy;     // symbolic name of token
TOKEN toktable[MAXNAMES]; // symbolic name of each token in printNames.
                          // Corresponding toktable & printNames elements
                          // have same index

BOOLEAN quittingtime, // true = exit the program
        eof,          // eof and eoln are used to mimic the boolean
        eoln,         // Pascal functions eof() and eoln().
        dollarflag,   // true = $ was entered which marks end of input
        echo,         // true = echo chars during a load command
        readfile;     // true = an input file is being loaded

void errmsg(ERROR_NUM, char[], int); // forward declaration

//---------------------------
// DATA STRUCTURE OPERATIONS
//---------------------------

// mkVALEXP - return an EXP of type VALEXP with num n
EXP mkVALEXP (NUMBER n)
{
    EXP e;

    e = malloc(sizeof(struct EXPREC));
    e->etype = VALEXP;
    e->u.num = n;
    return e;
} // mkVALEXP

// mkVAREXP - return an EXP of type VAREXP with varble nm
EXP mkVAREXP (NAME nm)
{
    EXP e;

    e = malloc(sizeof(struct EXPREC));
    e->etype = VAREXP;
    e->u.varble = nm;
    return e;
} // mkVAREXP

// mkAPEXP - return EXP of type APEXP with optr op and args el
EXP mkAPEXP (NAME op, EXPLIST el)
{
    EXP e;

    e = malloc(sizeof(struct EXPREC));
    e->etype = APEXP;
    e->u.ap.optr = op;
    e->u.ap.args = el;
    return e;
} // mkAPEXP

// mkExplist - return an EXPLIST with head e and tail el
EXPLIST mkExplist (EXP e, EXPLIST el)
{
    EXPLIST newel;

    newel = malloc(sizeof(struct EXPLISTREC));
    newel->head = e;
    newel->tail = el;
    return newel;
} // mkExplist

// mkNamelist - return a NAMELIST with head n and tail nl
NAMELIST mkNamelist (NAME nm, NAMELIST nl) 
{
    NAMELIST newnl;

    newnl = malloc(sizeof(struct NAMELISTREC));
    newnl->head = nm;
    newnl->tail = nl;
    return newnl;
} // mkNamelist

// mkValuelist - return an VALUELIST with head n and tail vl
VALUELIST mkValuelist (NUMBER n, VALUELIST vl)
{
    VALUELIST newvl;
    
    newvl = malloc(sizeof(struct VALUELISTREC));
    newvl->head = n;
    newvl->tail = vl;
    return newvl;
}

// mkEnv - return an ENV with vars nl and values vl
// Author Samuel Kamin uses Greek letter rho to refer to an ENV in the mathematical description
// in his textbook. So he uses variable rho for an ENV throughout this source code.
ENV mkEnv (NAMELIST nl, VALUELIST vl)
{
    ENV rho;

    rho = malloc(sizeof(struct ENVREC));
    rho->vars = nl;
    rho->values = vl;
    return rho;
} // mkEnv

// prEnv - print vars & values in an ENV
void prEnv(ENV env)
{
    
    NAMELIST nl;
    VALUELIST vl;
    
    int i;
    
    i=0;
    nl = env->vars;
    vl = env->values;
    
    while (nl != 0)
    {   
        i = i + 1;
        printf("%d.  %s = %ld", i, printNames[nl->head], vl->head);
        nl = nl->tail;
        vl = vl->tail;
    }
} // prEnv

// lengthVL - return length of VALUELIST vl
int lengthVL (VALUELIST vl)
{
    int i;

    i = 0;
    while (vl != 0)
    {
        i = i+1;
        vl = vl->tail;
    }
    return i;
} // lengthVL

// lengthNL - return length of NAMELIST nl
int lengthNL (NAMELIST nl)
{
    int i;

    i = 0;
    while (nl != 0)
    {
        i = i+1;
        nl = nl->tail;
    }
    return i;
} // lengthNL

void prExplist(EXPLIST); //forward declaration

// print an EXP
void prExp(EXP e)
{
    switch (e->etype)
    {
        case VALEXP:
            printf("etype = VALEXP\n");
            printf("  num = %ld\n", e->u.num);
            break;
            
        case VAREXP:
            printf(" etype = VAREXP\n");
            printf("varble = %s\n", printNames[e->u.varble]);           
            break;
        
        case APEXP:
            printf(" etype = APEXP\n");
            printf("  optr = %s\n", printNames[e->u.ap.optr]);
            prExplist(e->u.ap.args); 
            break;
        
        default:
            printf("Invalid etype = %d", e->etype);
            break;
    }
}

// prExplist - print an Explist
void prExplist(EXPLIST el)
{
    prExp(el->head);
    
    if (el->tail != 0)
        prExplist(el->tail);
    return;
} // prExplist

//-----------------
// NAME MANAGEMENT
//-----------------

// fetchDef - get FUNCTION definition of fname from fundefs
FUNDEF fetchDef (NAME fname)
{
     FUNDEF f;
     BOOLEAN found;

     found = false;
     f = fundefs;
     while (f != 0 && !found)
         if (f->funname == fname)
             found = true;
         else
             f = f->nextfundef;
     return f;
} // fetchDef

// newDef - add new FUNCTION fname with parameters nl, body e
void newDef (NAME fname, NAMELIST nl, EXP e)
{
    FUNDEF f;

    f = fetchDef(fname);
    if (f == 0)  // fname not yet defined as a FUNCTION
    {
        f = malloc(sizeof(struct FUNDEFREC));
        f->nextfundef = fundefs;   // place new FUNDEFREC
        fundefs = f;               // at front of fundefs list
    }
    f->funname = fname;
    f->formals = nl;
    f->body = e;
} // newDef

// initNames - place all pre-defined (built in) names into printNames
//             and corresponding token symbols in toktable.
//             user-defined names (functions, variables) will also be kept here
void initNames()
{
    int i;

    fundefs = 0;   //empty list of fundefs

    firstControlOp = i = 0;
    printNames[i] = "if";    toktable[i] = ifsy;     i = i+1;
    printNames[i] = "while"; toktable[i] = whilesy;  i = i+1;
    printNames[i] = ":=";    toktable[i] = assignsy; i = i+1;
    
    lastControlOp = i;
    printNames[i] = "seq";   toktable[i] = seqsy;    i = i+1;

    firstValueOp = i;
    printNames[i] = "+";     toktable[i] = addsy;   i = i+1;
    printNames[i] = "-";     toktable[i] = subsy;   i = i+1;

// To handle negative numbers (unary minus), we build an APEXP with the multiply 
// operator and operand -1. Below we save the multiply symbol index so we can
// insert it into the APEXP. This avoids having to look it up later.

    mulsy_index = i;
    printNames[i] = "*";     toktable[i] = mulsy;   i = i+1;
    printNames[i] = "/";     toktable[i] = divsy;   i = i+1;
    printNames[i] = "=";     toktable[i] = eqsy;    i = i+1;
    printNames[i] = "<";     toktable[i] = lssy;    i = i+1;
    printNames[i] = ">";     toktable[i] = gtsy;    i = i+1;
                             
    lastValueOp = i;
    printNames[i] = "print"; toktable[i] = printsy;  i = i+1;
    printNames[i] = "quit";  toktable[i] = quitsy;   i = i+1;
    printNames[i] = "then";  toktable[i] = thensy;   i = i+1;
    printNames[i] = "else";  toktable[i] = elsesy;   i = i+1;
    printNames[i] = "fi";    toktable[i] = fisy;     i = i+1;
    printNames[i] = "do";    toktable[i] = dosy;     i = i+1;
    printNames[i] = "od";    toktable[i] = odsy;     i = i+1;
    printNames[i] = "qes";   toktable[i] = qessy;    i = i+1;
    printNames[i] = "fun";   toktable[i] = funsy;    i = i+1;
    printNames[i] = "nuf";   toktable[i] = nufsy;    i = i+1;
    printNames[i] = "(";     toktable[i] = lparsy;   i = i+1;
    printNames[i] = ")";     toktable[i] = rparsy;   i = i+1;
    printNames[i] = ";";     toktable[i] = semsy;    i = i+1;
    printNames[i] = ",";     toktable[i] = comsy;    i = i+1;
    printNames[i] = "$";     toktable[i] = dollarsy; i = i+1;

    numNames = numBuiltins = i;  // no. of entries in 0 to numBuiltins-1
} // initNames

// initCmds - place all pre-defined commands into printCmds
void initCmds()
{
    short int i;
    
    i = 0;
    printCmds[i] = "sload"; sload = i; i = i+1;
    printCmds[i] =  "load";  load = i; i = i+1;
    printCmds[i] =  "user";  user = i; i = i+1;
    numCmds = i;
} //initCmds

// prName - print name nm
void prName(NAME nm)
{
    printf("%s",printNames[nm]);
} // prName

// prUserList - List user-defined names and token type id
void prUserList()
{
    int i;
    
    for(i = numBuiltins; i < numNames; i++) 
    {
        printf("printNames[%d]= ", i);
        prName(i);
        printf("   toktable[%d]= %d\n", i, toktable[i]);
    }
} // prUserList

// install - insert new name into printNames, set its type in toktable
//         - save its type in toksy and return its index in printNames array
NAME install(char *nm)
{
    NAME i;
    int result;
    BOOLEAN found;
    
    i = 0; 
    found = false;
    result = 0;
    
    while (i < numNames && !found)
    {
        result = strcmp(nm, printNames[i]);
        
        if (!result)
            found = true;
        else
            i = i+1;
    }   
    
    if (!found)
    {
        if (i == MAXNAMES)
            errmsg(err_max_names, null_str, null_int);
        printNames[i] = malloc(sizeof(char *));  //alloc memory for a ptr to char 
        numNames = i + 1;
        strcpy(printNames[i], nm);  //insert name in printNames
        toktable[i] = nameidsy;     //set its type in toktable
    }
    toksy = toktable[i];   //save current token symbol type 

    return i;   //return index of name
} // install

// initParse - initialization of variables
void initParse()
{
    readfile = echo = eoln = eof = dollarflag = false;
    userinput[0] = tokstring[0] = 0; //null terminate
    inputleng = tokleng = pos = 0; 
}

//------------
// INPUT
//------------

// isDelim - check if c is a delimiter
BOOLEAN isDelim (char c)
{
    if (c == SPACE || strchr(punctop, c) != NULL)
        return true;
    else
        return false;
} // isDelim

// skipblanks - return next non-blank position in userinput starting at position p
int skipblanks (int p)
{
    while (userinput[p] == SPACE)
        p = p + 1;
    return p;
} // skipblanks

// nextchar - read next char - filter tabs, comments
// DD: Also filter LF which is returned under Cygwin on Windows 7. It would become
//     first char of next input line and cause an "undefined variable" error
//     LF = ascii character 10 = newline "\n"          
char nextchar()
{
    int c;
 
    eoln = false;

    if (readfile)
    {
        if ((c = getc(infp)) == EOF) // read from file
        {
            readfile = false;
            echo = false;
            eoln = true;
            eof = true;
            c = DOLLAR; // exit read loop in readDollar and readInput 
            fclose(infp);
        }
        else if (echo)
            printf("%c", c); // echo char
    }
    else
        c = getc(stdin); // read from stdin

    if (c == LF)
        eoln = true;

// replace tab and eoln chars with space, skip comments

    if (c == TAB || c == LF || c == CR)
        c = SPACE;
    else if (c == COMMENTCHAR) // skip comment chars
    {          
        if (readfile)
        {
            while ((c = getc(infp)) != LF)
                if (echo) printf("%c", c); // echo comment chars
            if (echo) printf("%c", c);     // echo LF 
        }
        else
            while ((c = getc(stdin)) != LF)
                ;

        eoln = true;
        c = SPACE; // replace LF with space
    }

    return c;
} // nextchar

// readDollar - read char's, ignoring newlines, till '$' is read
//              '$' marks END of the fundef or expr that is being input
void readDollar()
{
    char c = SPACE;
    char str[2]="";

    do
    {   
        if (!readfile && eoln)
        {
            printf(PROMPT2); //continuation prompt
            fflush(stdout);
        }
        c = nextchar();
        pos = pos+1;
        if (pos == MAXINPUT)
        {
            str[0] = c;      //last char read
            errmsg(err_input_len, str, MAXINPUT);
        }
        userinput[pos] = c;

    } while (c != DOLLAR);
    dollarflag = true;
} // readDollar

/* The next four functions, readCmdLine, parseCmdName, parseCmdNameArg and processCmd handle 
   reading, parsing and executing the new load and sload commands. 
   If readInput detects a right parenthesis as first character of the input line 
   then it calls processCmd (which calls the others) to open the file and set
   readfile to true. While readfile is true, the nextchar function will read 
   input chars from the opened file instead of the terminal. */

// readCmdLine - read command line into userinput buffer.
//               When readInput detects RPAREN as first char of input,
//               it calls processCmd which calls readCmdLine to read
//               rest of the cmd line. On entry to this function,
//               pos==0 and userinput[0]==RPAREN.
void readCmdLine()
{
    char c = SPACE, *dollarPtr;
    
    while (!eoln)
    {
        c = nextchar(); // sets eoln true on LF & returns SPACE
        pos = pos + 1;
        userinput[pos] = c;
    } 

// use either dollar position (if any) or LF position to determine input length

    if ((dollarPtr = strchr(userinput, DOLLAR)) != NULL)
        inputleng = dollarPtr - userinput;
    else
        inputleng = pos;

} // readCmdLine

// parseCmdName - return Cmd starting at userinput[pos]
CMD parseCmdName()
{
    char nm[MAXCMDLENG+1]; // for accumulating characters in Cmd
    CMDSIZE leng; 
    CMD i;
    BOOLEAN found;
    int result;

    nm[0] = 0;
    result = 0;
    leng = 0;
    while (pos < inputleng && isalpha(userinput[pos]))
    {
        if (leng == MAXCMDLENG)
        {
            nm[leng] = 0;
            errmsg(err_cmd_len, nm, MAXCMDLENG);
        }
        nm[leng] = userinput[pos];
        leng = leng+1;
        pos = pos+1;
    }
    
    if (leng == 0)
        errmsg(err_no_cmd, null_str, null_int);
    
    nm[leng] = 0;  //null terminate Cmd name

// Determine Cmd index in printCmds

    i = 0; 
    found = false;
    while (i < MAXCMDS && !found)
    {
        result = strcmp(nm, printCmds[i]);
        
        if (!result)
            found = true;
        else
            i = i+1;
    }   

    if (!found)
        errmsg(err_bad_cmd, nm, null_int);

    pos = skipblanks(pos); /* skip blanks after command name */
    return i;
} // parseCmdName

/* parseCmdArg - return the character string argument starting at
   userinput[pos]. This function is currently used to parse the
   filename argument from the load & sload commands */
void parseCmdArg(char nm[])
{
    ARGSIZE leng;   // length of argumnet name 

    nm[0] = 0; 
    leng = 0;
    while (pos < inputleng && userinput[pos] > SPACE)
    {
        if (leng == ARGLENG)
        {
            nm[leng] = 0;
            errmsg(err_arg_len, nm, ARGLENG);
        }
        nm[leng] = userinput[pos];
        leng = leng+1;
        pos = pos+1;
    }

    if (leng == 0)
        errmsg(err_no_name, null_str, null_int);
    nm[leng] = 0;  //null terminate string
} // parseCmdArg

// processCmd - input, parse, and execute the command
void processCmd()
{
    CMD cmdnm;  // cmdnm is an index to printCmds
    char cwd[PATH_MAX];

    readCmdLine();
    pos = 1;                 // pos of 1st letter of command name
    cmdnm = parseCmdName();  // parse command name
    if (cmdnm == sload || cmdnm == load)
    {
        parseCmdArg(infilename);   // parse filename argument

        // Load commands cannot be issued inside another file being loaded.
        // They can only be issued interactively.
        // The following if condition ensures that they are not carried out
        // from within a file that is being loaded.
        // Instead we print an error and quit the program.
        // This allows user to redesign the logic without a load command
        // nested inside of another file that is being loaded.

        if (!readfile) //if not already loading a file
        {
            if ((infp = fopen(infilename, "r")) == NULL)
                errmsg(err_open, null_str, null_int);

            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("\nCurrent Directory is: %s\n", cwd);
            else
                errmsg(err_cwd, null_str, null_int);

            printf(" Loading file : %s\n\n",infilename);
            readfile = true;       // tell nextchar to read from file
            eof = false;
            if (cmdnm == load)
                echo = true;       // remains false for sload (silent load)
        }
        else //disallow nested load cmds
            errmsg(err_nested_load, infilename, null_int);

    }
    if (cmdnm == user)
        prUserList();
}  // processCmd

// readInput - read char's into userinput array until dollar sign is input
void readInput()
{
    char c = SPACE;
    char str[2]="";

    dollarflag = false;
    if (!readfile)
    {
        printf(PROMPT);  //display main prompt for new input
        fflush(stdout);
    }
    pos = -1;
    do
    {
        c = nextchar();
        pos = pos+1;
        if (pos == MAXINPUT)
        {
            str[0] = c;      //last char read
            errmsg(err_input_len, str, MAXINPUT);
        }
        userinput[pos] = c;

// parse a command or fundef or expression

        if (pos == 0 && c == RPAREN)  // RPAREN begins a cmd (e.g. load or sload)
        {
            processCmd();  //opens file, sets echo
            if (!readfile)
            {                
                printf(PROMPT); // redisplay main prompt after a cmd
                fflush(stdout);
            }                               
            pos = -1;  //restart userinput index
        }
        else //parse fundef or expression
        {
            if (userinput[pos] == DOLLAR)
                dollarflag = true;
            else
                if (eoln) readDollar();
        }
    } while (!dollarflag);

    inputleng = pos; // pos of '$' is length of input in 0 to pos-1

    //remove LF and any other chars that follow dollar sign from input buffer
    while (!eoln)
        c = nextchar(); // sets eoln true on LF & returns SPACE
} // readInput

// reader - read char's into userinput; be sure input is not blank
void reader()
{
    do
    {
        readInput();                  // read input into userinput array
        pos = skipblanks(0);          // advance to first non-blank
        if (userinput[pos] == DOLLAR) // if it is '$' then
            inputleng = 0;           // it is a blank line
    } while (inputleng == 0); // ignore blank lines
} // reader

NAME install(char *); // forward declaration

// parseName - return (installed) NAME starting at userinput[pos]
NAME parseName()
{
    char nm[NAMELENG+1]; // array to accumulate characters
    NAMESIZE leng; // length of name

    nm[0] = tokstring[0] = 0;
    leng = 0;
    while (pos < inputleng && !isDelim(userinput[pos]))
    {
        if (leng == NAMELENG)
        {
            nm[leng] = 0;
            errmsg(err_name_len, nm, NAMELENG);
        }
        nm[leng] = tokstring[leng] = userinput[pos];
        leng = leng+1;
        pos = pos+1;
    }
    if (leng == 0)
        errmsg(err_no_name2, null_str, null_int);
    nm[leng] = tokstring[leng] = 0;  //null terminate
    tokleng = leng;
    pos = skipblanks(pos); /* skip blanks after name */
    return install(nm);
} // parseName


// isNumber - check if a number begins at userinput[i]
//            It must be a sequence of digits followed by a delimiter
//            otherwise it will be treated as a name.
//            E.g. 232+ will be parsed as the number 232 followed by a plus sign.
//            but 232abc will be parsed as a name since the digits are not
//            followed by a delimiter.
BOOLEAN isNumber(short int i)
{
    if (!isdigit(userinput[i]))
        return false;
    else
    { 
        while (isdigit(userinput[i])) i = i + 1;
        if (isDelim(userinput[i]))
            return true;
        else
            return false;
    }
} // isNumber

// parseVal - return number starting at userinput[pos]
NUMBER parseVal()
{
    NUMBER n;
    char numString[MAXDIGITS+1]; //digits in value being parsed

    numString[0] = tokstring[0] = 0;
    n = 0;
    tokleng = 0;
    while (isdigit(userinput[pos]) && tokleng < MAXDIGITS)
    {
        n = 10*n + (userinput[pos] - '0');
        numString[tokleng] = tokstring[tokleng] = userinput[pos];
        tokleng = tokleng+1;
        pos = pos+1;
    }
    numString[tokleng] = tokstring[tokleng] = 0; // null terminate
    
    if (isdigit(userinput[pos]))
        errmsg(err_digits, null_str, MAXDIGITS); // Too many digits

    pos = skipblanks(pos); //skip any blanks after number
    return n;
} // parseVal

//----------------------
// NEW PARSING ROUTINES
//----------------------

// writeTokenName - Display the name corresponding to token symbol t.
void writeTokenName(TOKEN t)
{
    NAME i;

    if (t == nameidsy || t == numsy || t == funidsy)
        // display generic name for user-defined symbols
        switch (t)
        {
            case nameidsy:
                printf("nameid");
                break;
            case numsy:
                printf("number");
                break;
            case funidsy:
                printf("funid");
                break;
            default:
                break;
         }
    else
    {
    // display builtin name
        i = 0;
        while (toktable[i] != t && i < numBuiltins)
            i = i+1;
        if (i < numBuiltins)
            // write the name of the token
            printf("%s", printNames[i]);
        else   
            // name not found, display token id
            printf("name not found for token %d", t);
    }
}  // of writeTokenName

// writeTokenStr - Display token string. During errors, this function
// is used to display the invalid token found in userinput
void writeTokenStr()
{
    printf("%s ", tokstring);
} // writeTokenStr 

// errmsg - display error message for given error number
// and jump to JL99 in main fuction
void errmsg(ERROR_NUM errnum, char err_str[], int err_int)
{
    printf("*****");
    switch (errnum)
    {
        case err_arglist:
            printf("Error parsing arglist.  Found ");
            writeTokenStr();
            printf("where \")\" or nameid is expected.");
            break;
        case err_function:
            printf("Error parsing function name.  Found ");
            writeTokenStr();
            printf("where funid or nameid is expected.");
            break;
        case err_exp6:
            printf("Error parsing exp6.  Found ");
            writeTokenStr();
            printf("where nameid, funid, \"(\", or a number is expected.");
            break;
        case err_expr:
            printf("Error parsing expr.  Found ");
            writeTokenStr();
            printf("where one of the following is expected:\n");
            printf("\"if\", \"while\", \"seq\", \"print\", nameid, funid, number, or \"(\"");
            break;
        case err_cwd:
            perror("getcwd() error");
            break;
        case err_open:
            perror("fopen() error");
            break;
        case err_max_names:
            printf("No more room for names");
            break; 
        case err_input_len:
            printf("Input exceeds %d chars. Last char read = %s", err_int, err_str);
            printf("\nSkipping rest of input and quitting.");
            quittingtime = true;
            if (readfile) fclose(infp);
            break;
        case err_cmd_len:
            printf("Command Name exceeds %d chars, begins: %s", err_int, err_str);
            break;
        case err_no_cmd:
            printf("Error: expected Command name, instead read: %c", userinput[pos]);
            break;
        case err_bad_cmd:
            printf("Unrecognized Command Name, begins: %s", err_str);
            break;
        case err_arg_len:
            printf("Argument name exceeds %d chars, begins: %s", err_int, err_str);
            break;
        case err_no_name:
            printf("Error: expected name, instead read: %c", userinput[pos]);
            break;
        case err_name_len:
            printf("Name exceeds %d chars, begins: %s", err_int, err_str);
            break;
        case err_no_name2:
            printf("Error: expected name, instead read: %c", userinput[pos]);
            break;
        case err_digits:
            printf("parseVal: Max digits allowed in 64 bit signed long is %d", err_int);
            break;
        case err_no_name3:
            printf("mutate: found ");
            writeTokenStr();
            printf(" where nameid or funid is expected.");
            break; 
        case err_mismatch:
            printf("Error in match. Found ");
            writeTokenStr();
            printf(" where ");
            writeTokenName(err_int);
            printf(" is expected.");
            break;  
        case err_not_var:
            printf("parseExp1: left hand side of assignment must be a variable");
            break;
        case err_num_args:
            printf("Wrong number of arguments to ");
            prName(err_int);
            break;
        case err_undef_func:
            printf("Undefined function: ");
            prName(err_int);
            break;
        case err_num_args2:
            printf("Wrong number of arguments to: ");
            prName(err_int);
            break;
        case err_undef_var:
            printf("Undefined variable: ");
            prName(err_int);
            break;
        case err_undef_op:
            printf("eval: invalid value for op = %d", err_int);
            break;
        case err_nested_load:
            printf("Load commands cannot occur inside a file being loaded.\n");
            printf("*****");
            printf("Remove the load command for file %s", err_str);
            quittingtime = true;
            fclose(infp);
        default:
            break;
    }
    printf("\n\n");
    longjmp(JL99, errnum);
}  // errmsg

// getToken - get next token from userinput and set toksy, tokstring, tokleng.
//  for names and operators, also set tokindex.
//  for numbers, tokindex does not apply since they are not stored in printNames.
void getToken()
{
    char nm[2]; // array to accumulate characters

    nm[0] = tokstring[0] = 0;

    if (isNumber(pos))  // parse a number
    {
        numval = parseVal();  // set tokstring, tokleng
        toksy = numsy;
    }
    else if (userinput[pos] == ':' && userinput[pos+1] == '=')
    // parse an assignment
    {
        tokleng = 2;
        nm[0] = tokstring[0] = ':';
        nm[1] = tokstring[1] = '=';
        nm[2] = tokstring[2] = 0;

        pos = pos + 2;
        pos = skipblanks(pos);
        tokindex = install(nm);  // set toksy
    }
    else if (strchr(punctop, userinput[pos]) != NULL) 
    // parse single char punctuation or operator
    {
        tokleng = 1;
        nm[0] = tokstring[0] = userinput[pos];
        nm[1] = tokstring[1] = 0; // null terminate 1 char string
        
        pos = pos + 1;
        pos = skipblanks(pos);
        tokindex = install(nm); // set toksy
    }
    else  // else assume it is a name
        tokindex = parseName(); // set toksy, tokstring, tokleng
} // getToken

// mutate - Change type of toksy in toktable to newtype.
//  This function is currently used to redefine a variable name (nameidsy)
//  as a function name (funidsy)
void mutate(TOKEN newtype)
{
    if (toksy != nameidsy && toksy != funidsy)
        errmsg(err_no_name3, null_str, null_int);
    else
        toktable[tokindex] = toksy = newtype;
} // of mutate

// match - If the expected token t matches the current one in toksy
//         then call getToken() to get the next toksy
//         else print mismatch error
void match(TOKEN t)
{
     if (toksy == t)
         getToken();
     else
         errmsg(err_mismatch, null_str, t);
} // match

// parse parameters of a function definition 
NAMELIST parseParams()
{
    NAME nm;
    NAMELIST nl;

    switch (toksy)
    {
        case rparsy:      // end of list, return null
            nl = 0;
            break;
        case nameidsy:
            nm = tokindex;
            match(nameidsy);
            if (toksy == comsy)  // recursively parse remainder of list
            {
                match(comsy);
                nl = parseParams();
            }
            else
                nl = 0;
            nl = mkNamelist(nm, nl);
            break;
        default:
            errmsg(err_arglist, null_str, null_int);
            break;
    }
    return nl;
}  // parseParams

EXP parseExpr(void); //forward declaration

// parseDef - parse function definition at userinput[pos]
// syntax: fun fun_name(arglist):=el nuf
// function name is returned as value of a fundef
NAME parseDef()
{
     NAME fname;        // function name
     NAMELIST nl;       // formal parameters
     EXP e;             // body

     match(funsy);      // match "fun" and get next toksy
     switch (toksy)
     {
         case nameidsy:
            mutate(funidsy);   // set type to funidsy & do case funidsy
         case funidsy:
            fname = tokindex; // save name index for use below
            match(funidsy);
            break;
         default:
            errmsg(err_function, null_str, null_int);
            break;
     }
     match(lparsy);
     nl = parseParams();
     match(rparsy);
     match(assignsy);
     e = parseExpr();
     match(nufsy);
     newDef(fname, nl, e);
     return fname;
} // parseDef

// parse arguments of a function call
EXPLIST parseArgs()
{
    EXP e;
    EXPLIST el;

    if (toksy == rparsy)      // RPAREN marks end of arg list
        return 0;
    else
    {
        e = parseExpr();
        if (toksy == comsy)   // recursively parse rest of arg list   
        {
            match(comsy);
            el = parseArgs();
        }
        else
            el = 0;
        return mkExplist(e,el);
    }
} // parseArgs

// parse function call
// syntax: f(e1,e2, . . . , en)
EXP parseCall()
{
    EXPLIST el;
    NAME nm;

    nm = tokindex;
    match(funidsy);
    match(lparsy);
    el = parseArgs();
    match(rparsy);
    return mkAPEXP(nm,el);
} /* parseCall */

// parseExplist - parse expression list
EXPLIST parseExplist()
{
    EXP e;
    EXPLIST el;

    e = parseExpr();
    if (toksy == semsy)
    {
        match(semsy);
        el = parseExplist();
    }
    else
        el = 0;
    return mkExplist(e,el);
} // parseExplist

// parse if expression
// syntax: if e1 then e2 else e3 fi
EXP parseIf()
{
    EXP e1,e2,e3;
    EXPLIST eL;
    NAME nm;

    nm = tokindex;
    match(ifsy);
    e1 = parseExpr();
    match(thensy);
    e2 = parseExpr();
    match(elsesy);
    e3 = parseExpr();
    match(fisy);
    eL = mkExplist(e3,0);
    eL = mkExplist(e2,eL);
    eL = mkExplist(e1,eL);
    return mkAPEXP(nm,eL);
} // parseIf

// parse while expression
// syntax: while e1 do e2 od
EXP parseWhile()
{
    EXP e1,e2;
    EXPLIST eL;
    NAME nm;

    nm = tokindex;
    match(whilesy);
    e1 = parseExpr();
    match(dosy);
    e2 = parseExpr();
    match(odsy);
    eL = mkExplist(e2,0);
    eL = mkExplist(e1,eL);
    return mkAPEXP(nm,eL);
} // parseWhile

// parse sequence expression
EXP parseSeq()
{
    EXPLIST eL;
    NAME nm;

    nm = tokindex;
    match(seqsy);
    eL = parseExplist();
    match(qessy);
    return mkAPEXP(nm,eL);
} // parseSeq

/* 
   The following functions (parseExp1 through parseExp6) implement the
   following grammar rules.

   exp1 −→ exp2 [ :=  exp1 ]*
   exp2 −→ [ prtop ] exp3
   exp3 −→ exp4 [ relop exp4 ]*
   exp4 −→ exp5 [ addop exp5 ]*
   exp5 −→ [ addop ] exp6 [ mulop exp6 ]*
   exp6 −→ name | integer | funcall | ( expr )

   The recursive structure of these rules yields the following precedence from
   lowest to highest:
   
     :=
     prtop
     relop
     addop
     unary addop, mulop
     variable name, integer, function call, expression in parentheses

   Since the functions call each other recursively, they are implemented in 
   reverse order below to avoid forward declarations.
*/

// parse variable name, integer, parenthesized expr, function call
EXP parseExp6()
{
    EXP ex;
    NAME varnm;
    NUMBER num;

    switch (toksy)
    {
        case nameidsy:
            varnm = tokindex;
            match(nameidsy);
            ex = mkVAREXP(varnm);
            break;
        case numsy:
            num = numval;
            match(numsy);
            ex = mkVALEXP(num);
            break;
        case lparsy:
            match(lparsy);
            ex = parseExpr();
            match(rparsy);
            break;
        case funidsy:
            ex = parseCall();
            break;
        default:
             errmsg(err_exp6, null_str, null_int);
             break;
    }
    return ex;     // return pointer to expression
} // parseExp6

// parse unary addop, binary mulop
EXP parseExp5()
{
    NAME nm;
    EXP ex,e1,e2;
    EXPLIST eL;
    TOKEN addop_token = 0;
    NUMBER sign;

    if (toksy == addsy || toksy == subsy) // unary + or -
    {
        addop_token = toksy;
        match(toksy);
    }

    e1 = parseExp6();

    if (addop_token == subsy)  // unary minus 
    {
        // make an expr to multiply e1 by -1
        sign = -1;
        ex = mkVALEXP(sign);
        eL = mkExplist(ex,0);
        eL = mkExplist(e1,eL);
        nm = mulsy_index;
        e1 = mkAPEXP(nm,eL);
    }

    while (toksy == mulsy || toksy == divsy) // binary mulops
    {
        nm = tokindex;
        match(toktable[nm]);
        e2 = parseExp6();
        eL = mkExplist(e2,0);
        eL = mkExplist(e1,eL);
        e1 = mkAPEXP(nm,eL);
    }
    return e1;
} // parseExp5

// parse binary addop
EXP parseExp4()
{
    NAME nm;
    EXP e1,e2;
    EXPLIST eL;

    e1 = parseExp5();
    while (toksy == addsy || toksy == subsy)
    {
        nm = tokindex;
        match(toktable[nm]);
        e2 = parseExp5();
        eL = mkExplist(e2,0);
        eL = mkExplist(e1,eL);
        e1 = mkAPEXP(nm,eL);
    }
    return e1;
} // parseExp4

// parse binary relop
EXP parseExp3()
{
    NAME nm;
    EXP e1,e2;
    EXPLIST eL;

    e1 = parseExp4();
    while (toksy == lssy || toksy == eqsy || toksy == gtsy)
    {
        nm = tokindex;
        match(toktable[nm]);
        e2 = parseExp4();
        eL = mkExplist(e2,0);
        eL = mkExplist(e1,eL);
        e1 = mkAPEXP(nm,eL);
    }
    return e1;
} // parseExp3

// parse print op
EXP parseExp2()
{
    EXPLIST eL;
    EXP ex;
    NAME nm;
    BOOLEAN printflag;

    printflag = false;
    if (toksy == printsy)
    {
        printflag = true;
        nm = tokindex;
        match(printsy);
    }
    ex = parseExp3();
    if (printflag)
    {
        eL = mkExplist(ex,0);
        return mkAPEXP(nm,eL);
    }
    else
        return ex;
} // parseExp2

// parse assign op
EXP parseExp1()
{
    EXPLIST eL;
    EXP ex,e2;
    NAME nm;

    ex = parseExp2();
    while (toksy == assignsy)
    {
        // build an assignment expression

        nm = tokindex;
        match(assignsy);
        if (ex->etype == VAREXP) // lhs must be a variable
        {
            e2 = parseExp1();   // process rhs
            eL = mkExplist(e2,0);
            eL = mkExplist(ex,eL);
            ex = mkAPEXP(nm,eL);
        }
        else  // illegal lhs
            errmsg(err_not_var, null_str, null_int);
    } // while
    return ex;
} // parseExp1

// parse if, while, seq, Exp1 
EXP parseExpr()
{
    EXP ex;

    switch (toksy)
    {
        case ifsy:
            ex = parseIf();
            break;
        case whilesy:
            ex = parseWhile();
            break;
        case seqsy:
            ex = parseSeq();
            break;
        case nameidsy:
        case numsy:
        case subsy:
        case funidsy:
        case printsy:
        case lparsy:
            ex = parseExp1();
            break;
        default:
            errmsg(err_expr, null_str, null_int);
            break;
    } // case
    return ex;
} /* parseExpr */


//--------------
// ENVIRONMENTS
//--------------

// emptyEnv - return an environment with no bindings
ENV emptyEnv()
{
    return mkEnv(0, 0);
} // emptyEnv

// bindVar - bind variable nm to value n in environment rho
void bindVar (NAME nm, NUMBER n, ENV rho)
{
    rho->vars = mkNamelist(nm, rho->vars);
    rho->values = mkValuelist(n, rho->values);
} // bindVar


// findVar - look up nm in rho
VALUELIST findVar (NAME nm, ENV rho)
{
    NAMELIST nl;
    VALUELIST vl;
    BOOLEAN found;

    found = false;
    nl = rho->vars;
    vl = rho->values;
    while (nl != 0 && !found)
    {
        if (nl->head == nm)
            found = true;
        else
        {
            nl = nl->tail;
            vl = vl->tail;
        }
    }
    return vl;
} // findVar

// assign - assign value n to variable nm in rho
void assign (NAME nm, NUMBER n, ENV rho)
{
    VALUELIST varloc;

    varloc = findVar(nm, rho);
    varloc->head = n;
} // assign

// fetch - return number bound to nm in rho
NUMBER fetch (NAME nm, ENV rho)
{
    VALUELIST vl;

    vl = findVar(nm, rho);
    return vl->head;
}  // fetch

// isBound - check if nm is bound in rho
int isBound (NAME nm, ENV rho)
{
// isBound := findVar(nm, rho) <> nil
    return findVar(nm, rho) != 0;
} // isBound


//---------
// NUMBERS
//---------

// prValue - print number n
void prValue (NUMBER n)
{
    printf("%ld",n);
}

// arity - return number of arguments expected by op
int arity (BUILTINOP op)
{
    if (op >= ADDOP && op <= GTOP)
        return 2; 
    else 
        return 1;
} // arity

void prName(NAME);  // forward declaration

// applyValueOp - apply operator to arguments in VALUELIST
NUMBER applyValueOp (BUILTINOP op, VALUELIST vl)
{
    NUMBER n, n1, n2;

    if (arity(op) != lengthVL(vl))
        errmsg(err_num_args, null_str, op);
    
    n1 = vl->head;           // 1st actual
    if (arity(op) == 2) 
        n2 = vl->tail->head; // 2nd actual
    
    switch (op)
    {
        case ADDOP:
            n = n1 + n2;
            break;
        case SUBOP:
            n = n1 - n2;
            break;
        case MULOP:
            n = n1 * n2;
            break;
        case DIVOP:
            n = n1 / n2;
            break;
        case EQOP:
            if (n1 == n2)
                n = 1;
            else
                n = 0;
            break;
        case LTOP: 
            if (n1 < n2)
                n = 1;
            else
                n = 0;
            break;
        case GTOP:
            if (n1 > n2)
                n = 1;
            else
                n = 0;
            break;
        case PRINTOP:
            prValue(n1);
            printf("\n");
            n = n1;
            break;
        default:   // this case should never occur
            printf("applyValueOp: bad value for op = %d\n", op);
            break;
    } // switch
    return n;
} // applyValueOp


//------------
// EVALUATION
//------------

NUMBER eval (EXP, ENV);   // forward declaration

// evalList - evaluate each expression in el
VALUELIST evalList (EXPLIST el, ENV rho)
{
    NUMBER h;
    VALUELIST t;

    if (el == 0)
        return 0;
    else
    {
        h = eval(el->head, rho);
        t = evalList(el->tail, rho);
        return mkValuelist(h, t);
    }
} // evalList

// applyUserFun - look up definition of nm and apply to actuals
NUMBER applyUserFun (NAME nm, VALUELIST actuals)
{
    FUNDEF f;
    ENV rho;

    f = fetchDef(nm);
    if (f == 0)
        errmsg(err_undef_func, null_str, nm);

    if (lengthNL(f->formals) != lengthVL(actuals))
        errmsg(err_num_args2, null_str, nm);
    rho = mkEnv(f->formals, actuals);
    return eval(f->body, rho);
} // applyUserFun

// applyCtrlOp - apply CONTROLOP op to args in rho
NUMBER applyCtrlOp (BUILTINOP op, EXPLIST args, ENV rho)
{
    NUMBER n;

    switch (op)
    {
        case IFOP:
            if (eval(args->head, rho))
                n = eval(args->tail->head, rho);
            else
                n = eval(args->tail->tail->head, rho);
            break;
        case WHILEOP:
            n = eval(args->head, rho);
            while (n)
            {
                n = eval(args->tail->head, rho);
                n = eval(args->head, rho);
            }
            break;
        case ASSIGNOP:
            n = eval(args->tail->head, rho);
            if (isBound(args->head->u.varble, rho))
                assign(args->head->u.varble, n, rho);
            else if (isBound(args->head->u.varble, globalEnv))
                assign(args->head->u.varble, n, globalEnv);
            else 
                bindVar(args->head->u.varble, n, globalEnv);
            break;
        case SEQOP:
            while (args->tail != 0)
            {
                n = eval(args->head, rho);
                args = args->tail;
            }
            n = eval(args->head, rho);  //value of last statement in seq
            break;
        default:
            printf("applyCtrlOp: bad value for op = %d\n", op);
            break;
    }  // switch
    return n;
} // applyCtrlOp

// eval - return value of expression e in local environment rho
NUMBER eval (EXP e, ENV rho)
{
    BUILTINOP op;
    NUMBER n;

    switch (e->etype)
    {
        case VALEXP:
            n = e->u.num;
            break;
        case VAREXP:
            if (isBound(e->u.varble, rho))
                n = fetch(e->u.varble, rho);
            else if (isBound(e->u.varble, globalEnv))
                n = fetch(e->u.varble, globalEnv);
            else
                errmsg(err_undef_var, null_str, e->u.varble);
            break;
        case APEXP:
            op = e->u.ap.optr;
            if (op > numBuiltins-1)
                n = applyUserFun(op, evalList(e->u.ap.args, rho));
            else
            {
                if (op >= firstControlOp && op <= lastControlOp)
                    n = applyCtrlOp(op, e->u.ap.args, rho);
                else if (op >= firstValueOp && op <= lastValueOp)
                    n = applyValueOp(op, evalList(e->u.ap.args, rho));
                else
                    errmsg(err_undef_op, null_str, op);
            }
            break;
        default:
            printf("Invalid etype = %d", e->etype);
            break;
    } // switch
    return n;         
} // eval

//----------------------------
// READ-EVAL-PRINT LOOP (REPL)
//----------------------------

int main (int argc, char **argv)
{
    short int error_no=0;

    initNames();
    initCmds();
    initParse();

    globalEnv = emptyEnv();
    quittingtime = false;

// Set the jump location for longjmp.
// After a parse error, errmsg() displays the error message and does
// a longjmp to here. The program then continues with the REPL below.
    if (setjmp(JL99))
        ;

// repeatedly read, parse and evaluate the input
// until "quit$" is entered

    while (!quittingtime)
    {
        reader();
        getToken();

        if (toksy == quitsy)
            quittingtime = true;
        else if (toksy == funsy)
        {
            prName(parseDef());
            printf("\n\n");
        }
        else
        {
            currentExp = parseExpr();
            prValue(eval(currentExp, emptyEnv()));
            printf("\n\n");
        }
    } // while

    exit (EXIT_SUCCESS);
}

# chap1_enhanced_c

## Introduction
This repo builds and tests a version of my enhanced interpreter that is rewritten in the C programming language. It is based on the one presented in chapter 1 of Samuel Kamin's book, [Programming Languages: An Interpreter-Based Approach](https://www.amazon.com/Programming-Languages-Samuel-N-Kamin/dp/0201068249/ref=sr_1_1). Kamin's original version is described at [chap1_orig](https://github.com/ddawson631/chap1_orig). The enhanced version written in Pascal is described in detail at [chap1_enhanced](https://github.com/ddawson631/chap1_enhanced). This enhanced version written in C is a direct translation from the enhanced version written in Pascal.

My enhancements replaced the original Lisp-style syntax with a Pascal-like syntax and added new commands (load & sload) which allow instructions to be read from a file. But the overall design of the enhanced interpreter is the same as the original. It still uses the same data structures and built-in operations as the original. For a good introduction to the overall design, please read Section 1.2, entitled *The Structure of the Interpreter*, in the following document that Professor Kamin kindly provided from his textbook.

- __[Chapter 1](docs/chapter1.pdf)__ - original grammar, syntax, design & documentation

It describes the data structures and functions used in the original source code which it divides into the following 8 sections: DECLARATIONS, DATA STRUCTURE OPS, NAME MANAGEMENT, INPUT, ENVIRONMENTS, NUMBERS, EVALUATION, MAIN. In order to implement my enhancements, I updated the DECLARATIONS, NAME MANAGEMENT, INPUT & MAIN sections and also added a new section entitled NEW PARSING ROUTINES as described at [chap1_enhanced](https://github.com/ddawson631/chap1_enhanced).

The comparison document below describes my translation of the enhanced interpreter from Pascal into C. It first reviews the grammar (Section 1) and instruction syntax (Section 2) which did not change between the Pascal and C versions. Section 3 describes my decisions about some of the translation issues and presents a side-by-side comparison of the Pascal and C source code with some clarifying comments.

- __[Comparison](docs/comparison.pdf)__ - grammar, syntax, C translation decisions, Pascal vs C source code.\
      NOTE: If this PDF does not render well on github then please download the corresponding RTF for a better viewing
      experience offline.

The information below presents an interactive run, Unit Test (UT) description and a listing of the Makefile.

## Interactive Run

Below is an interactive test run of the examples given in section 1.1.3 of the above chapter 1 document. The last two examples define and run a non-recursive gcd function and a recursive one.

```console
~/c/proglang/chap1$ ./chap1
-> 3$
3

-> 4+7$
11

-> x:=4$
4

-> x+x$
8

-> print x$
4
4

-> y:=5$
5

-> seq print x; print y; x*y qes$
4
5
20

-> if y>0 then 5 else 10 fi$
5

-> while y>0 do
>    seq x:=x+x; y:=y-1 qes
> od$
0

-> x$
128

-> fun #1 (x) := x + 1 nuf$
#1

-> #1(4)$
5

-> fun double(x):=x+x nuf$
double

-> double(4)$
8

-> x$
128

-> fun setx(x,y):= seq x:=x+y; x qes nuf$
setx

-> setx(x,1)$
129

-> x$
128

-> fun not(boolval):= if boolval then 0 else 1 fi nuf$
not

-> fun ## (x,y):= not(x=y) nuf$
##

-> fun mod(m,n):=m-n*(m/n)nuf$
mod

-> fun gcd(m,n):=
>    seq
>     r:=mod(m,n);
>     while ##(r,0) do
>      seq
>       m:=n;
>       n:=r;
>       r:=mod(m,n)
>      qes
>     od;
>     n
>    qes
>   nuf$
gcd

-> gcd(6,15)$
3

-> fun gcd(m,n):=
>    if n=0 then m else gcd(n,mod(m,n)) fi nuf$
gcd

-> gcd(6,15)$
3

-> quit$
~/c/proglang/chap1$
```

## Unit Test Strategy

The unit test (UT) script is `chap1_ut.sh`. 
The test results that it generates are saved to `chap1_ut.rs1` which is then compared to `chap1_ut.rs0`.

The extensions `.rs1` & `.rs0` stand for "result 1" & "result 0", respectively.\
The `.rs1` file contains the current result from most recent UT run.\
The `.rs0` file contains the last saved, validated result from a previous UT run. 

When the current and previous results are compared (via diff command), the only differences should be due to new or modified test cases. Unexpected differences must be investigated and corrected if necessary. Once all differences are 
confirmed to be valid then the `.rs1` file is copied to the `.rs0` file which is then saved as the latest valid result.

## UT Script

The UT cases for the Pascal version were reused for and produce the same results in the C version.
I also added new test cases to test the error messages. 

The UT script passes load commands to the interpreter as a Here String as follows.

./chap1 <<< ")load chap1_ut.input1"

This tells the interpreter to read its instructions from `chap1_ut.input1`.
It echoes each instruction as it is read then parses and executes it. 

The UT script runs the interpreter four times to process the test data in the following four input files. 

`chap1_ut.input1` - tests cases for many valid and invalid instructions\
`chap1_ut.input2` - error cases that exceed maximum sizes\
`chap1_ut.input3` - error case for a load command inside a file being loaded - aborts program\
`chap1_ut.input4` - error case for a sload command inside a file being loaded - aborts program

Below is the listing of the current UT script.

```sh
~/c/proglang/chap1$ cat chap1_ut.sh
#!/bin/bash
#
# Unit Test script for chap1 program
# See Backup Strategy and Log of Example Run at end of script
#

#
# Make the exe then run the unit test cases
#

name=chap1
echo "making $name"
make

#
# Run test cases, save results, diff with previous result.
# Redirection of stdout, stderr to result file follows closing brace below
#

echo -e "\nRunning $name Test Cases - Saving results to ${name}_ut.rs1.\n"

{
echo -e "./$name <<< \")load ${name}_ut.input1\""
./$name <<< ")load ${name}_ut.input1"
} > ${name}_ut.rs1 2>&1

echo -e "Rebuilding with smaller sizes to test error cases."
make testsize
echo -e "Running Tests that exceed sizes and appending results to ${name}_ut.rs1.\n"

{
echo -e "./$name <<< \")load ${name}_ut.input2\""
./$name <<< ")load ${name}_ut.input2"
} >> ${name}_ut.rs1 2>&1

echo -e "Rebuilding with normal size limits for other test cases."
touch chap1.c
make

echo -e "\nNested load test: Use load command to read a file containing a load command."
echo -e "An error will be displayed and the program aborts"
echo -e "Running Nested load test and appending results to ${name}_ut.rs1.\n"
{
echo -e "./$name <<< \")load ${name}_ut.input3\""
./$name <<< ")load ${name}_ut.input3"
} >> ${name}_ut.rs1 2>&1

echo -e "Nested sload test: Use sload command to read a file containing a sload command."
echo -e "We get same result as with load command above except that sload (silent load)"
echo -e "does not echo the commands and all the comments."
echo -e "Running Nested sload test and appending results to ${name}_ut.rs1.\n"
{
echo -e "./$name <<< \")sload ${name}_ut.input4\""
./$name <<< ")sload ${name}_ut.input4"
} >> ${name}_ut.rs1 2>&1


#
# Diff Results
# Below we compare current result (.rs1) with previous good result (.rs0).
#

echo -e "diff current result ${name}_ut.rs1 with last validated result ${name}_ut.rs0."
diff -qs ${name}_ut.rs1 ${name}_ut.rs0

#
# Backup Strategy
# If diffs are valid then copy .rs1 to .rs0 and save .rs0 as latest good result.
# Backup Makefile, source, UT script, inputs and result (.rs0) together.
#
#
# Log of running this script
#
# ~$ touch chap1.c
# ~$ ./chap1_ut.sh
# making chap1
# gcc -std=c99 -c chap1.c
# gcc -o chap1 chap1.o
#
# Running chap1 Test Cases - Saving results to chap1_ut.rs1.
#
# Rebuilding with smaller sizes to test error cases.
# gcc -std=c99 -DTESTSIZE -o chap1 chap1.c
# Running Tests that exceed sizes and appending results to chap1_ut.rs1.
#
# Rebuilding with normal size limits for final test cases.
# gcc -std=c99 -c chap1.c
# gcc -o chap1 chap1.o
#
# Nested load test: Use load command to read a file containing a load command.
# An error will be displayed and the program aborts
# Running Nested load test and appending results to chap1_ut.rs1.
#
# Nested sload test: Use sload command to read a file containing a sload command.
# We get same result as with load command above except that sload (silent load)
# does not echo the commands and all the comments.
# Running Nested sload test and appending results to chap1_ut.rs1.
#
# diff current result chap1_ut.rs1 with last validated result chap1_ut.rs0.
# Files chap1_ut.rs1 and chap1_ut.rs0 are identical
```

## UT Run

Below is an example UT run followed by a listing of its result file. 
Since the UT script reports that the `.rs1` and `.rs0` files are identical, there is no need to
copy the `.rs1` to `.rs0` in this case.

```console
~/c/proglang/chap1$ ./chap1_ut.sh
making chap1
gcc -std=c99 -c chap1.c
gcc -o chap1 chap1.o

Running chap1 Test Cases - Saving results to chap1_ut.rs1.

Rebuilding with smaller sizes to test error cases.
gcc -std=c99 -DTESTSIZE -o chap1 chap1.c
Running Tests that exceed sizes and appending results to chap1_ut.rs1.

Rebuilding with normal size limits for other test cases.
gcc -std=c99 -c chap1.c
gcc -o chap1 chap1.o

Nested load test: Use load command to read a file containing a load command.
An error will be displayed and the program aborts
Running Nested load test and appending results to chap1_ut.rs1.

Nested sload test: Use sload command to read a file containing a sload command.
We get same result as with load command above except that sload (silent load)
does not echo the commands and all the comments.
Running Nested sload test and appending results to chap1_ut.rs1.

diff current result chap1_ut.rs1 with last validated result chap1_ut.rs0.
Files chap1_ut.rs1 and chap1_ut.rs0 are identical
~/c/proglang/chap1$
```

## UT Result

Below is a listing of the current result file. 
A "Loading file" message is displayed at the beginning of each of the four input files mentioned earlier. 
The lines that begin with an exclamation point are comments from the input files. 

```console
~/c/proglang/chap1$ cat chap1_ut.rs1
./chap1 <<< ")load chap1_ut.input1"
->
Current Directory is: /home/dawsond/c/proglang/chap1
 Loading file : chap1_ut.input1

!Redo tests from section 1.1.3 of Kamin's text using the Pascal-style syntax

3$
3

4+7$
11

x:=4$
4

x+x$
8

print x$
4
4

y:=5$
5

seq print x; print y; x*y qes$
4
5
20

if y>0 then 5 else 10 fi$
5

while y>0 do
  seq x:=x+x; y:=y-1 qes
od$
0

x$
128

fun #1 (x) := x + 1 nuf$
#1

#1(4)$
5

fun double(x):=x+x nuf$
double

double(4)$
8

x$
128

fun setx(x,y):= seq x:=x+y; x qes nuf$
setx

setx(x,1)$
129

x$
128

fun not(boolval):= if boolval then 0 else 1 fi nuf$
not

fun ## (x,y):= not(x=y) nuf$
##

fun mod(m,n):=m-n*(m/n)nuf$
mod

fun gcd(m,n):=
 seq
  r:=mod(m,n);
  while ##(r,0) do
   seq
    m:=n;
    n:=r;
    r:=mod(m,n)
   qes
  od;
  n
 qes
nuf$
gcd

gcd(6,15)$
3


fun gcd(m,n):=
  if n=0 then m else gcd(n,mod(m,n)) fi nuf$
gcd

gcd(6,15)$
3


!Normal precedence and associativity are implemented.
5*3+7$
22

5+3*7$
26

14-7-3$
4

48/12/2$
2

!relational operators
5<10$
1

5>10$
0

5=5$
1

10<5$
0

10<5>-1$
1

10<5>-1=1$
1

!Keywords cannot be redefined
fun if (x) := x+5 nuf$
*****Error parsing function name.  Found if where funid or nameid is expected.

if := 20$
*****Error parsing expr.  Found := where one of the following is expected:
"if", "while", "seq", "print", nameid, funid, number, or "("


!Names may contain any char that is not a delimiter and must
!not contain only digits.
!Delimiters = ' ','(',')','+','-','*','/',':','=','<','>',';',',','$','!'
~12#ab:=25$
25

~12#ab$
25

x:=15-~12#ab+7$
-3


!A string of digits is not a valid name.
fun 222  (x) := x+222 nuf$
*****Error parsing function name.  Found 222 where funid or nameid is expected.


!Inserting a non-delimiter char into a string of digits makes a valid name.
fun 222# (x) := x+222 nuf$
222#

222#(3)$
225

x:=100-222#(3)-50$
-175


!Inserting a delimiter in a name causes erroneous results.
a(b:=25$
*****Undefined variable: a


!Function name may not be reused as a variable name.
fun inc10 (x) := x+10 nuf$
inc10

inc10:=25$
*****Error in match. Found :=  where ( is expected.


!Multiple assignment
i:=j:=k:=25$
25

i$
25

j$
25

k$
25


!ERROR MESSAGE TESTS

fun david(x,+,z):= x+1 nuf$
*****Error parsing arglist.  Found + where ")" or nameid is expected.

fun +++(x):= x+1 nuf$
*****Error parsing function name.  Found + where funid or nameid is expected.

abc:=)25$
*****Error parsing exp6.  Found ) where nameid, funid, "(", or a number is expected.


!The next 4 inputs below test the error checking for invalid load commands.
!A valid load command inside a file being loaded will abort the program.
!See chap1_ut.input3 and chap1_ut.input4 for such test cases.
!But since these are invalid load commands, an error message is printed and
!the program moves on to the next input. Moving on instead of aborting may
!not be desirable if the instructions that failed to load were required
!for the commands that follow to work correctly.
!But that is how I decided to implement it for now.
!
!An instruction ending with a $ must occur after a comment line before any
!command is entered. This causes readDollar function to return to readInput
!function where the right parenthesis that begins a command is checked for.
!That is the purpose of the two lines below that contain only a $.
!See the comments in chap1_ut.input3 for a detailed explanation about this.
!
$
)abcdefghi argument$
*****Command Name exceeds 8 chars, begins: abcdefgh

)123 mod.txt$
*****Error: expected Command name, instead read: 1

)average mod.txt$
*****Unrecognized Command Name, begins: average


!Max ARGLENG=40. Arg below has 41 chars.
$
)load abcdefghijklmnopqrstuvwxyzabcdefghijklmno$
*****Argument name exceeds 40 chars, begins: abcdefghijklmnopqrstuvwxyzabcdefghijklmn


!Max NAMELENG=20 but the var and function names below have 21 chars.
print abcdefghijklmnopqrstu$
*****Name exceeds 20 chars, begins: abcdefghijklmnopqrst

fun abcdefghijklmnopqrstu(x):=x+10 nuf$
*****Name exceeds 20 chars, begins: abcdefghijklmnopqrst


!MAXDIGITS=19 but value below has 20 digits.
d:=99999999999999999999$
*****parseVal: Max digits allowed in 64 bit signed long is 19


22:=4$
*****parseExp1: left hand side of assignment must be a variable

mod(100)$
*****Wrong number of arguments to: mod


quit$
./chap1 <<< ")load chap1_ut.input2"
->
Current Directory is: /home/dawsond/c/proglang/chap1
 Loading file : chap1_ut.input2

!Test Cases for exceeding MAXINPUT and MAXNAMES

!Following lines exceed MAXNAMES=28. Builtin names = 26.
!So third name below is 29th name and triggers the error.
a:=25$
25

b:=100$
100

c:=200$
*****No more room for names


!Following function definition exceeds MAXINPUT=50. This error causes
!the program to skip the rest of the file and exit. So the function
!definition for not2 below is skipped and no error message will appear
!for it in the result file.
fun not(boolVal):=
if boolVal then
print 0
els*****Input exceeds 50 chars. Last char read = s
Skipping rest of input and quitting.

./chap1 <<< ")load chap1_ut.input3"
->
Current Directory is: /home/dawsond/c/proglang/chap1
 Loading file : chap1_ut.input3

fun mod(m,n):=m-n*(m/n)nuf$
mod

mod(100,53)$
47


fun not(boolval):=
  if boolval then
  0
  else
  1
  fi
nuf$
not

not(3)$
0

not(0)$
1


!Load commands cannot occur inside a file being loaded.
!So the one below causes the program to print an error and quit.
!The load command below is preceded by a line with a single dollar sign.
!The reason for this is as follows.
!
!The end of input marker for the interpreter is a $.
!The readInput and readDollar functions read input as follows.
!readInput displays the main prompt and reads the first input line.
!If it ends with a $ then the input is complete and the main prompt is
!redisplayed for the next input.
!Otherwise it calls readDollar to repeatedly display the continuation prompt
!and read another line until a $ is read.
!If EOF occurs then a $ is returned.
!
!Consider the above mod function definition.
!It is on one line ending with a $.
!So readInput reads that line without calling readDollar.
!
!Consider the above not function defintion on multiple lines.
!After readInput reads the first line, it reaches eoln without seeing a $.
!So it calls readDollar to prompt for and read lines until "nuf$" is read.
!
!Now consider these comment lines which begin with an exclamation point.
!When readInput sees an exclamation point, it ignores all chars that follow
!on that line. So after readInput reads the first comment line,
!it reaches eoln without seeing a $. It then calls readDollar to read
!lines until a $ is reached.
!
!The chap1 interpreter accepts three kinds of input:
! 1. function defintions - such as those for mod and not above
! 2. expressions - such as the function calls to mod and not above
! 3. commands - such as the load command below
!Commands must have a right parenthesis as the first char on the line.
!
!The last thing to be aware of is that commands are only checked for
!in the readInput function, not in the readDollar function.
!So after readInput displays the main prompt, if it reads a
!right parenthesis of the load cmd below as the first input char,
!it knows it must branch away from reading expressions and function defs
!to process a command (e.g. to open a file to read more expressions
!and function definitions from).
!But if readDollar sees the right parenthesis of the load cmd below
!as the first char of an input line, it will read it and the chars that
!follow as part of an expression or function def and get a syntax error
!since it does not have the logic to detect and process commands.
!
!Now consider what will happen to the following load command which is
!preceded by a comment line followed by a $ line as shown below.
!As mentioned earlier, after readInput reads the first comment line,
!it calls readDollar to keep reading until a $ is reached.
!So readDollar will be reading all comment lines that follow the
!the first one above. After it reads the last comment line
!below, it will read the $ line which will cause it to return
!to readInput. Since all comment chars were ignored, a line
!containing only a $ is received as input which is a blank line
!which is skipped. readinput then reads the right parenthesis
!and detects and processes the load command.
!
!However, in this case the load command is illegal since it is contained
!in a file. The processCmd logic will recognize that it is already
!reading from this file and that the command to open and read from
!another one is invalid. It will issue an error message and quit the program.
!
$
)load gcd_only.txt$
*****Load commands cannot occur inside a file being loaded.
*****Remove the load command for file gcd_only.txt

./chap1 <<< ")sload chap1_ut.input4"
->
Current Directory is: /home/dawsond/c/proglang/chap1
 Loading file : chap1_ut.input4

mod

47

not

0

1

*****Load commands cannot occur inside a file being loaded.
*****Remove the load command for file gcd_only.txt

~/c/proglang/chap1$
```

## Makefile

Below is a listing of the current Makefile.

```sh
~/c/proglang/chap1$ cat Makefile
#
# Makefile for C version of enhanced chapter 1 interpeter.
# The chapter 1 interpreter from Samuel Kamin's Programming Languages texbook
# was enhanced to use a more Pascal-like syntax instead of Lisp-style syntax.
#
SHELL = /bin/bash

chap1: chap1.o
        gcc -o chap1 chap1.o

chap1.o: chap1.c
        gcc -std=c99 -c chap1.c

debug:
        gcc -std=c99 -g -o chap1 chap1.c
#
# The testsize target below builds with TESTSIZE macro defined.
# This causes the MAXNAMES and MAXINPUT macros to be assigned smaller
# sizes so that unit test cases that exceed these values can be performed.
#
testsize:
        gcc -std=c99 -DTESTSIZE -o chap1 chap1.c

clean:
        rm -f chap1.o

```

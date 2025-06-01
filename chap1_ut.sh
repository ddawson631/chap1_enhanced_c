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

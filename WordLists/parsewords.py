#!/usr/bin/env python
# dictionary generator from list files of word assocations  Rob Chapman  Jul 4, 10
# There are 4 lists: words, immediates, constants, comments
# As the input file is parsed, it is sorted into the four different types
# It would be nice to intermix the types but in the end the C file will
# require them to be separated for efficient storage.
# They are built up and then when file is done, a C file is generated
# c conditionals and comments are kept as is
# each dictionary item has a type and data
# types are: entry, c code
#  entry consists of: name function and optionally comment
#  c code is: either # or //

# specific is in other directories
# any file of the form: *bindings.txt is checked for updates

#parse file
# parsing is outer; looking for Words, Immediates, Constants, or //
# parsing for dictionary is word or // or #xxx
# for each line
#  parse line
# generate c file

# Bugs:
# if there are no immediate words then empty array declarations are produced. These don't compile
# if a dependant file has changed the includer will not be recompiled
import os, sys, traceback, time

# globals
words = []
immediates = []
constants = []
comments = []

DIRECTIVE = 'directive' # note: this is somehow getting inserted into the .txt file
COMMENT = 'comment'

# input files
parser   = os.path.abspath(__file__)
dirname  = os.path.dirname(parser)
txtcore  = os.path.join(dirname,'clibindings.txt')
txtfloat = os.path.join(dirname,'floatwords.txt')
inputFiles = [parser, txtcore, txtfloat]

# generated files
targetDir   = './' # default
ctarget		= 'wordlist.c'
helptarget	= 'help.c'
txttarget	= 'wordlist.txt'
outputFiles = [ctarget, helptarget, txttarget]

# file headers for: wordlist.c help.c wordlist.txt
wordlistCheader = ''' \
// names are kept in flash; arrays are used; reduces space requirements
#include "cli.h"

#define NAMES(name) const char name[] = {
#define NAME(s) s "\\000"
#define END_NAMES ""}; // empty string to cover empty array

#define NONAMES(name) const char name[] = {""};
#define NOBODIES(functions) const struct constantCall functions[] = {{NULL}};

#define BODIES(functions) const vector functions[] = {
#define CBODIES const struct constantCall constantbodies[] = {
#define BODY(f) (vector)f,
#define CONSTANTBODY(f)  { cii, &f },
#define CONSTANTNUMBER(n)  { cii, (Byte *)n },
#define END_BODIES };

void cii(void);

'''

helpCheader = '''\
#include <stdlib.h>
#include <string.h>
#include "printers.h"
#include "cli.h"

void help(void);
void printif(char *s);

static char *filter;

void printif(char *s)
{
	if (strstr(s, filter) != NULL)
		print(s);
}

void help(void) {
	cursorReturn();
	parse(0);
	here();
	filter = (char *)ret()+1;
'''

texthelp = '''\
 [v] for variable
 [i] for compile words
 ( a - b ) stack before operation 'a' and after operation 'b'; right is top

'''

# parser
current = None
parseLine = None

keywords = {
	'//':			lambda line: comments.append(line),
	'Words':		lambda line: setDictionary(words),
	'Immediates':	lambda line: setDictionary(immediates),
	'Constants':	lambda line: setDictionary(constants),
	'Include':		lambda line: includeFile(line)
}

def parseFile(line): # parse line from file
	fields = line.split()
	if fields:
		keywords.get(fields[0], lambda line: sys.stdout.write('unknown keyword: %s\n'%fields[0]))(line)

def readWordLists(file): # read in word lists
	global parseLine
	print "reading: "+file
	lines = open(file, 'r').readlines()
	parseLine = parseFile
	for line in lines:
		parseLine(line)

def addEntry(line):
	global parseLine
	try:		
		fields = line.split()
		if not fields:
			pass
		elif fields[0] == 'End':
			parseLine = parseFile
		elif fields[0] in ['#if', '#ifdef', '#else', '#endif']:
			current.append((DIRECTIVE, line))
		elif fields[0] == '//':
			current.append((COMMENT, line))
		else:
			comment = line.partition('//')[2]
			if not comment:
				comment = '\n'
			current.append((fields[0], fields[1], comment))	
	except:
		traceback.print_exc(file=sys.stderr)
		print 'line: %s' % line

def setDictionary(dict):
	global parseLine, current
	parseLine = addEntry
	current = dict

def includeFile(line):
	global parseLine, current
	parser, dict = parseLine, current # remember context
	for file in line.split()[1:]:
		readWordLists(file)
	parseLine, current = parser, dict # restore context

# c code generator
def generateCode(file):
	file = open(file, 'w')

	file.write(wordlistCheader)
	file.write('\n// Words\nNAMES(wordnames)\n')
	names(file, words)
	file.write('END_NAMES\n\n')
	if words:
		voids(file, words)
		file.write('\nBODIES(wordbodies)\n')
		bodies(file, words, 'BODY')
		file.write('END_BODIES\n\n')
	else:
		file.write('NOBODIES(wordbodies)\n')

	file.write('// Immediates\nNAMES(immediatenames)\n')
	names(file, immediates)
	file.write('END_NAMES\n\n')
	if immediates:
		voids(file, immediates)
		file.write('\nBODIES(immediatebodies)\n')
		bodies(file, immediates, 'BODY')
		file.write('END_BODIES\n\n')
	else:
		file.write('NOBODIES(immediatebodies)\n')

	file.write('// Constants\nNAMES(constantnames)\n')
	names(file, constants)
	file.write('END_NAMES\n\n')
	if constants:
		externs(file, constants)
		file.write('\nCBODIES\n')
		bodies(file, constants, 'CONSTANTBODY', 'CONSTANTNUMBER')
		file.write('END_BODIES\n\n')
	else:
		file.write('NOBODIES(constantbodies)\n')

	file.close()
	
def names(file, dict):
	for w in dict:
		if w[0] in [DIRECTIVE, COMMENT]:
			file.write(w[1])
		elif not w[2]:
			file.write('\tNAME("%s")\n' % w[0])
		else:
			file.write('\tNAME("%s")\t\t// %s' % (w[0], w[2]))

def voids(file,dict):
	for w in dict:
		if w[0] in [DIRECTIVE, COMMENT]:
			file.write(w[1])
		else:
			file.write("void %s(void);\n" % w[1])

def externs(file,dict):
	for w in dict:
		if w[0] in [DIRECTIVE, COMMENT]:
			file.write(w[1])
		elif w[1][0] not in '0123456789':
			file.write("extern Byte %s;\n" % w[1])

def bodies(file,dict,prefix,number=""):
	for w in dict:
		if w[0] in [DIRECTIVE, COMMENT]:
			file.write(w[1])
		else:
			if number and w[1][0] in '0123456789':
				file.write("\t%s(%s)\n" % (number,w[1]))
			else:
				file.write("\t%s(%s)\n" % (prefix,w[1]))

# text generator
def generateText(file): # sorted list of words
	dict = [['', words],[' [i]',immediates],[' [v]',constants]]
	entries = []

	for list in dict:
		group = ''
		type = list[0]
		names = list[1]
		for name in names:
			if name[0] in [DIRECTIVE, COMMENT]:
				pass
			else:
				n = ''.join(name[0].split('\\'))
				if len(name) < 3:
					entries.append("%s%s %s %s" % (n, group, type, '\n'))
				else:
					entries.append("%s%s %s %s" % (n, group, type, name[2]))
	
	wordlist = open(file, 'w')
	wordlist.write('Word list for firmware  %s'%generatedBy())
	wordlist.write(texthelp)
	map(wordlist.write, sorted(entries))	
	wordlist.close()

def printLine(string): # print a line given a string
	string = string.rstrip()
	string = string.replace('\\', '\\\\')
	string = string.replace('"', '\\\"')
	return '    printif("'+string+'\\n");\n'

def generateHelp(file): # sorted list of words in a c file
	dict = [['', words],[' [i]',immediates],[' [v]',constants]]
	entries = []

	for list in dict:
		group = ''
		type = list[0]
		names = list[1]
		for name in names:
			if name[0] in [DIRECTIVE, COMMENT]:
				pass
			else:
				n = ''.join(name[0].split('\\'))
				if len(name) < 3:
					entries.append("%s%s %s %s" % (n, group, type, '\n'))
				else:
					entries.append("%s%s %s %s" % (n, group, type, name[2]))
	
	wordlist = open(file, 'w')
	wordlist.write(helpCheader)
	map(wordlist.write, map(printLine, sorted(entries)))	
	wordlist.write('}\n')
	wordlist.close()

# utilities
def generatedBy(): # string for heading
	t = time.strftime('%b %d, %Y  %H:%M:%S',time.localtime())
	return 'generated by parsewords.py  %s\n\n'%t

def emptyWords(): # empty out lists
	global words, immediates, constants, comments
	del words[:]
	del immediates[:]
	del constants[:]
	del comments[:]

# update is require if there is no wordlist.c
#  or if *bindings.txt, this file, clibindings.txt are newer than wordlist.c
def fileModTime(file): # return file modified date
	return time.localtime(os.path.getmtime(file))

def needUpdate(checkfile):
	try:
		# test target file existence
		for file in outputFiles:
			open(file).close()

		# check file mod times > wordlist.c
		modtime = fileModTime(ctarget)
		for file in inputFiles + [checkfile]:
			if fileModTime(file) > modtime:
				print file + ' newer than ' + ctarget
				return True
		return False
	except:
		print 'Updates triggered by "%s" not existing'%file
		return True

def generateWordlists(file):
	readWordLists(file)
	print 'generating %s, %s and %s' % (ctarget, txttarget, helptarget)
	generateCode(ctarget)
	generateText(txttarget)
	generateHelp(helptarget)
	
if __name__ == '__main__':
	arg = sys.argv[-1] # accept a file argument
	try:
		if arg.endswith('.txt') == True:
			# put results in same directory as input file
			targetDir, file = os.path.split(os.path.abspath(arg))				
			os.chdir(targetDir)
			if needUpdate(file): # update all if one is affected
				generateWordlists(file)
		else:
			print "Error: no '.txt' file specified."

	except Exception as message:
		print (type(message))    # the exception instance
		traceback.print_exc(file=sys.stderr)

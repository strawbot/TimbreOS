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

# specific is in other direcotries

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

# make script runable from anywhere from any os
abspath = os.path.abspath(__file__)
dirname = os.path.dirname(abspath)
os.chdir(dirname)

# globals
words = []
immediates = []
constants = []
comments = []

DIRECTIVE = 'directive' # note: this is somehow getting inserted into the .txt file
COMMENT = 'comment'

# parser
current = None
parseLine = None

parsers = {
	'//':			lambda line: comments.append(line),
	'Words':		lambda line: setDictionary(words),
	'Immediates':	lambda line: setDictionary(immediates),
	'Constants':	lambda line: setDictionary(constants),
	'Include':		lambda line: includeFile(line)
}

def emptyWords(): # empty out lists
	global words, immediates, constants, comments
	del words[:]
	del immediates[:]
	del constants[:]
	del comments[:]

def readWordLists(file): # read in word lists
	global parseLine
	lines = open(file, 'r').readlines()
	parseLine = parseFile
	for line in lines:
		parseLine(line)

def parseFile(line): # parse line from file
	fields = line.split()
	if fields:
		parsers.get(fields[0], lambda line: sys.stdout.write('no parser for %s\n'%fields[0]))(line)

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
	header = '''// names are kept in flash; arrays are used; reduces space requirements
#include "botkernl.h"

#define NAMES(name) PROGMEM char name[] = {
#define NAME(s) s "\\000"
#define END_NAMES };

#define BODIES(functions) const void * const functions[] = {
#define BODY(f) (void * const)f,
#define CONSTANTBODY(f)  (void * const)CII,(void * const)&f,
#define CONSTANTNUMBER(n) (void * const)CII,(void * const)n,
#define END_BODIES };

void CII(void);

'''
	file = open(file, 'w')
	file.write('// System dictionaries  %s'%genby())
	file.write(header)
	map(file.write, comments)

	file.write('\n// Words\nNAMES(wordnames)\n')
	names(file, words)
	file.write('END_NAMES\n\n')
	voids(file, words)
	file.write('\nBODIES(wordbodies)\n')
	bodies(file, words, 'BODY')
	file.write('END_BODIES\n\n')

	file.write('// Immediates\nNAMES(immediatenames)\n')
	names(file, immediates)
	file.write('END_NAMES\n\n')
	voids(file, immediates)
	file.write('\nBODIES(immediatebodies)\n')
	bodies(file, immediates, 'BODY')
	file.write('END_BODIES\n\n')

	if len(constants):
		file.write('// Constants\nNAMES(constantnames)\n')
		names(file, constants)
		file.write('END_NAMES\n\n')
		externs(file, constants)
		file.write('\nBODIES(constantbodies)\n')
		bodies(file, constants, 'CONSTANTBODY', 'CONSTANTNUMBER')
		file.write('END_BODIES\n\n')

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
texthelp = ''' [v] for variable
 [i] for compile words
 ( a - b ) stack before operation 'a' and after operation 'b'; right is top

'''
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
	wordlist.write('Word list for firmware  %s'%genby())
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
	header = '''#include <stdlib.h>
#include "printers.h"
#include "botkernl.h"
#include "kernel.h"

void help(void);
void printif(char *s);

static char *filter;

void printif(char *s)
{
	if (strstr(s, filter) != NULL)
		print(s);
}

void help(void) {
	_CR();
	BL(), WORD(), HERE();
	filter = (char *)*sp++;
	filter[filter[0] + 1] = 0;
	filter++;
'''
	wordlist.write(header)
	map(wordlist.write, map(printLine, sorted(entries)))	
	wordlist.write('}\n')
	wordlist.close()

# mobile agent support generator
def generateAgent(file): # generate mobile agent data
	header = '''#include <avr/pgmspace.h>
extern void (*wordbodies[])();
extern void (*constantbodies[])();
extern void (*immediatebodies[])();
extern PROGMEM char wordnames[];
extern PROGMEM char constantnames[];
extern PROGMEM char immediatenames[];

'''
	agent = open(file, 'w')
	agent.write('// Word information for mobile agents  %s'%genby())
	agent.write(header)
	agent.write('#define numconstants %d\n'%len(constants))
	agent.write('#define numwords %d\n'%len(words))
	agent.write('#define numimmediates %d\n'%len(immediates))
	agent.write('#define numtotal %d\n'%(len(constants)+len(words)+len(immediates)))
	agent.close

# utilities
def genby(): # string for heading
	t = time.strftime('%b %d, %Y  %H:%M:%S',time.localtime())
	return 'generated by parsewords.py  %s\n\n'%t

# look for subdirectories and concat those to root bindings to generate
# c files and word lists
import sys, os, glob

if sys.platform[:3] == 'win':
	SEPARATOR = '\\'
else:
	SEPARATOR = '/'

ctarget = 'wordlist.c'
txttarget = 'wordlist.txt'
helptarget = 'help.c'
txtcore = 'clibindings.txt'
txtcpld = 'cpldaccess.txt'
thisfile = 'parsewords.py'
vanilla = 'Vanilla'

def fileModTime(file): # return file modified date
	return time.localtime(os.path.getmtime(file))

if __name__ == '__main__':
	update = 0
	for bindings in glob.glob('*/*bindings.txt'):
		dir, file = bindings.split(SEPARATOR)
		try:
			open(dir+SEPARATOR+ctarget).close() # test file existence
			open(dir+SEPARATOR+ctarget).close()
			open(dir+SEPARATOR+ctarget).close()
			if	fileModTime(txtcore) < fileModTime(dir+SEPARATOR+ctarget) \
			and fileModTime(txtcpld) < fileModTime(dir+SEPARATOR+ctarget) \
			and fileModTime(thisfile) < fileModTime(dir+SEPARATOR+ctarget):
				if fileModTime(bindings) < fileModTime(dir+SEPARATOR+ctarget):
					continue # don't update this subdirectory
			update += 1
		except:
			update += 1
	
	if update != 0: # update all if one is affected; counter the dependancy bug
		print 'at least one is out of date; updating all.'
		for bindings in glob.glob('*/*bindings.txt'):
			dir, file = bindings.split(SEPARATOR)
			emptyWords()
			print 'changing to directory: '+dir
			os.chdir(dir)
			readWordLists(file)
			print 'generating %s, %s and %s'%(ctarget, txttarget, helptarget)
			generateCode(ctarget)
			generateText(txttarget)
			generateHelp(helptarget)
			os.chdir('..')

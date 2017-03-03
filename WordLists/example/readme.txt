Create your own wordset for you app

1. create a folder like example but call it app - this way it will be ignored by TimbreOS git repo
2. copy bootbindings.txt from this folder into the app folder
3. in your app add a prebuild task to invoke: python parsewords.python
   this will create files help.c and wordlist.c which you will include in your app
   it will also create a text file of all your words and their comments
4. add words to bootbindings.txt as needed. The format will be:
    whatiwanttotypein whatCfunctiontocall // one line comment
   The C function should be void cfunction(void)
   It can output values using the available functions in printers.h
5. to add the full list of CLI helper functions, just include ../clibindings.txt
6. to add support for floating point to CLI, include ../floatwords.txt
7. look in those files for examples.
8. to obtain the address of global variable or data structure - it can't be static
   add an entry in Constants or define a function for that matter
Create your own wordset for your app

1. copy bootbindings.txt to your folder space
2. in your build environment, add a prebuild task to invoke:
    python parsewords.py <absolute or relative path to file>/bootbindings.txt
   this will create files: help.c, wordlist.c
   which you will include in your app.
   It will also create a text file: wordlist.txt
   of all your words and their comments. this can be used for documentation
3. add words to bootbindings.txt as needed. The format will be:
     whatIwant2type whatCfunction2call // one line of comment
   The C function should be void cfunction(void)
   A function can output values using the API in printers.h
4. to add the full list of CLI helper functions, just include <path to>/clibindings.txt
5. to add support for floating point to CLI, include <path to>floatwords.txt
6. look in those files for examples.
7. to obtain the address of global variable or data structure - it can't be static -
   add an entry in Constants or define a function for that matter
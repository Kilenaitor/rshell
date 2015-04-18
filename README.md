# rshell
rshell is a custom bash shell created for UCR's CS100 course on Software Construction.

##Dependencies
rshell requires the Boost library for C++ to be installed

If you are on Ubuntu, run `sudo apt-get install libboost-all-dev`

If you are on Mac OS X, I'd recomment using Homebrew. 
Install Homebrew and use the command `brew install boost`

To manually install, the file can be found here: http://www.boost.org/users/download/

##Build and Run
hw0 is the current working build of the program
Any current branch progress is not tested nor confirmed to work as expected
```
$ git clone  https://github.com/kilenaitor/rshell.git
$ cd rshell
$ git checkout hw0
$ make
$ bin/rshell
```

##Limitations

- Need C++11 for it to properly compile
- Need the boost library for it to properly compile
- Host name is limited to 128 characters
- Does not detect triple connectors like `|||` or `&&&`, they're just interpreted as another command
- Will not prompt for more input if connector is the end. (e.g. `ls &&` just prints ls)
- Connectors cannot be touching other commands or parameters (e.g. `echo "this"|| echo "that"` will not work)
- Cannot direct output. (e.g. `echo "this" > test.ext` will output 'this > test.txt')
- Cannot pipe command output
- cd command does not work
- Pressing up on the arrow key does not restore the previous command
- No navigating text input; Enters the literal keys for left, right, up, down, etc.
- `!*command*` does not execute the previous command
- No color output
- Does not allow aliases or general settings

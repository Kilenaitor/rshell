# rshell
rshell is a custom bash shell created for UCR's CS100 course on Software Construction.

##Dependencies
rshell requires the Boost library for C++ to be installed

If you are on Ubuntu, run `sudo apt-get install libboost-all-dev`

If you are on Mac OS, I'd recomment using Homebrew. Use the command `brew install boost`

Download file can be found here: http://www.boost.org/users/download/

##Build and Run
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
- Does not support two connectors in a row (e.g. `echo test || || echo test` will fail)
- Cannot start with a connector or else it will break
- Connectors cannot be touching other commands or parameters (e.g. `echo "this"|| echo "that"` will not work)
- Cannot direct output. (e.g. `echo "this" > test.ext` will output 'this > test.txt')
- No color output


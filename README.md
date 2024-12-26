# RetroMake
##### CMake-like for your favorite text editor!

I **love** CMake. Unfortunately, it only works with "true" IDEs like Visual Studio, fully ignoring the existence of fancy text editors and not-quite-abandoning other IDEs. RetroMake is here to change that!

### Command-line interface
RetroMake has the same command-line interface as CMake except for the `-G` argument. RetroMake accepts not one entity, but many, and these many entities occupy multiple slots. The standard slots include:
 - Compiler -- compiler and linker (essential)
 - Build system -- build automation system (essential)
 - Text editor -- basic text editor
 - IntelliSense -- syntax highlighting, autocomplete and friends for the text editor
 - Debugger -- debugger for the test editor
All slots need to be occupied for a full IDE-like experience. RetroMake's `-G` parameter is also case-invariant.

RetroMake is aware has following modules built-in:
1.  GCC :white_check_mark:
  - Occupied slots: compiler
  - Dependencies: none
2.  Clang (alias: llvm) :white_check_mark:
  - Occupied slots: compiler
  - Dependencies: none
3.  Make (alias: Makefile, Makefiles, Unix makefile, Unix Makefiles) :white_check_mark:
  - Occupied slots: build system
  - Dependencies: none
4.  Ninja :x:
  - Occupied slots: build system
  - Dependencies: none
5.  VS Code (alias: VSCode, Code, VS Code, Visual Studio Code) :white_check_mark:
  - Occupied slots: text editor
  - Dependencies: none
6.  VS Codium (alias: VSCodium, Codium, VSCode OSS, VS Code OSS) :white_check_mark:
  - Occupied slots: text editor
  - Dependencies: none
7.  Clangd :x:
  - Occupied slots: intellisense
  - Dependencies: VS Code or VS Codium
8.  Native Debug (alias: WebFreak, Web Freak) :white_check_mark:
  - Occupied slots: debugger
  - Dependencies: VS Code or VS Codium
9.  Visual Studio (alias: VS) :x:
  - Occupied slots: compiler, build system, text editor, intellisense, debugger
  - Dependencies: none
10. Code::Blocks (alias: CodeBlocks) :white_check_mark:
  - Occupied slots: build system, text editor, intellisense, debugger
  - Dependencies: none
When creating a project, you need to specify all desired functionality in the `-G` parameter, comma-separated. For example:
```
retromake .. -G 'GCC, Ninja, Codium, Native Debug'
```
### Dependencies
RetroMake is built with CMake and uses CMake to parse `CMakeLists.txt`. Right now RetroMake is a mere wrapper around CMake, that hoverer will change with new modules. RetroMake also depends on RapidJSON and RapidXML.

### Configuration
RetroMake determines the list of requested modules by going through following priority list:
1. `-G` argument
2. `RETROMAKE_REQUESTED_MODULES` environmental variable
3. `RETROMAKE_REQUESTED_MODULES` entry in `~/.retromake`
4. `RETROMAKE_REQUESTED_MODULES` entry in `/etc/retromake`

### Contribution
You want to implement your own module? No problem! RetroMake is highly modularized, the new module can be implemented in a fork or added into existing RetroMake installation as a file. In the latter case, your module needs to end with `.so` and to expose a factory function with the following signature:
```
extern "C" rm::Module *create_module(const std::string &requested_module);
```

It also needs to be said that current architecture is not scalable and flexible enough. I don't plan on providing any backwards compatibility for modules for now.
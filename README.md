# AP Git Simulator
An in-memory Git and shell file system simulator written in C++ using object-oriented design.

## Build & Run
```bash
g++ -Wall -Wextra main.cpp FileSystem.cpp Repository.cpp -o git_sim
./git_sim
```

## Supported Commands
### Shell Commands
- `pwd`, `ls`, `mkdir <DIR>`, `cd <PATH>`
- `touch <FILE>`, `echo "<TEXT>" > <FILE>`, `echo "<TEXT>" >> <FILE>`
- `cat <FILE>`, `rm <FILE>`, `exit`

### Core Git Commands
- `git config user.name "<NAME>"` / `git config user.email "<EMAIL>"`
- `git init`
- `git add <FILE>` / `git add .`
- `git reset <FILE>`
- `git commit -m "<MESSAGE>"`
- `git status`
- `git log`
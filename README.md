# COMP.4060 - COMPILER CONSTRUCTION: BMINOR
### Install & Build
```sh
git clone https://github.com/BenHoule/bminor-compiler
cd bminor-compiler/
make
make test
./bminor -scan test/scanner/good_keywords.bminor
```
**NOTE:** Makefile uses gcc-14 and `-std=c23` / `-std=gnu23` flags

### Contributing
Make a branch
```sh
git checkout -b <branch>
```

...make changes...

Push changes
```sh
git add .
git commit -m "message"
git checkout main
git pull
git checkout <branch>
git rebase main
git push [--force-with-lease]
git checkout main
git merge --ff-only <branch>
git push
```


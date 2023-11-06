
# Dactylic Hexameter (Working Title)

This is a (currently still WIP) program/library to scan Latin verses according to the dactylic hexameter.

## Compilation

### Linux

First, bootstrap nob:

```shell
$ gcc -o nob nob.c
```

Then, run nob with or without a target selected:

```shell
$ ./nob
# OR, if you want to crosscompile to Windows
$ ./nob win64-mingw
```

### Windows

Install MinGW from [here](https://www.mingw-w64.org/downloads/#mingw-builds). Then you can bootstrap nob:

```powershell
$ gcc -o nob nob.c
```

Then, run nob:

```powershell
$ ./nob.exe
```

## Usage

Just run the program (you probably know how to run a program by the time you read this readme document).

When you run the program, you are greeted with the following prompt:

```text
Intrare versum: 
```

Insert your line of poetry here. For this example, we used `Multi illam petiere illa adversata petentes`. You can find some more examples in sampleVerses.txt, in the same folder as this README document.

```text
Inritare versum: Multi illam petiere illa adversata petentes

Elision: mult  illam petier  ill  adversata petentes

 1      2      3      4     5      6
 _   _  _   u u_  _   _  _  _ u  u _  _
mult illam petier ill adversata petentes
```

At the bottom, you will see the notation for the dactyli, `_` indicating the syllable should be long, `u` meaning it should be short. If `?` is somewhere in there, the program couldn't find which length the syllable should be. This happens sometimes, because not all rules of the dactylic hexameter are implemented into this program/library (yet).

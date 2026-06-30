@echo off

set "FILE=examples/test.lsp"
if not exist "%FILE%" (
    > "%FILE%" echo ; 此文件不被git跟踪，可随意更改
)

cmake -B vs_build_example -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 17 2022" -DExampleID=test.lsp
start vs_build_example/lisp-interpreter.sln

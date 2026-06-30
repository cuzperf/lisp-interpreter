@echo off

set "FILE=examples/test.lsp"
if not exist "%FILE%" (
    > "%FILE%" echo ; 此文件不被git跟踪，可随意更改
)

vs_build_example\Debug\lisp.exe examples/test.lsp
pause

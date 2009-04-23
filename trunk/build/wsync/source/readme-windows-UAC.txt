to fix UAC in the generated Cmake VS project:

1) go to project properties
2) Manifest Tool -> Input and Output -> set "embed manifest" to "No"

this way UAC requests admin access when the program is executed
####################################################################################################
#    2025 Fall CAS2107 Project 1
#---------------------------------------------------------------------------------------------------
#    .
#    ├── Makefile
#    ├── examples        // example input and output files
#    │   ├── input1.txt
#    │   └── input2.txt
#    │   ├── output1.txt
#    │   └── output2.txt
#    ├── tf32.h
#    ├── tf32.c      // fill this code
#    └── main.c      // the main function
#
#    === How to run ==
#    $ make
#    $ ./main < examples/input1.txt
#
#    === How to submit your source code ==
#    $ make tar
#    Then, upload your tar file to LearnUs.
#
###################################################################################################-

main: main.c tf32.c
	gcc -o main main.c tf32.c

clean:
	rm -f main
	rm -f ${USER}.tar

tar:
	tar -cf ${USER}.tar tf32.c
	@echo "================================================================================="
	@echo "${USER}.tar is generated. This does NOT mean your solution is submitted"
	@echo "You MUST upload ${USER}.tar to LearnUs."
	@echo "You may need to use scp to copy ${USER}.tar from this server to your computer"
	@echo "If you have any difficulty, please contact TA (not your colleague)"


ifeq ($(OS),Windows_NT)
    OUT := myron.exe
else
    OUT := myron
endif

debug:
	gcc -Wall -Wextra -Og -o $(OUT) myron.c

release:
	gcc -Wall -Wextra -Werror -O3 -s -fno-ident -fno-asynchronous-unwind-tables -o $(OUT) myron.c

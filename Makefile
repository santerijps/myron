debug:
	gcc -Wall -Wextra -Og -o myron.exe myron.c

release:
	gcc -Wall -Wextra -Werror -O3 -s -fno-ident -fno-asynchronous-unwind-tables -o myron.exe myron.c && upx --best ./myron.exe

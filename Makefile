CFLAGS = -march=native -Wall -O2 -g -fsanitize=address,undefined

tests: tests/roundtrip tests/parse
	./tests/roundtrip
	./tests/parse

tests/roundtrip tests/parse: snrt.c snrt.h consts.h

consts.h: gen_consts
	./gen_consts

clean:
	rm -rf tests/roundtrip tests/parse gen_consts

.PHONY: tests clean

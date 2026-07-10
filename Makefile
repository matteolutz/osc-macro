RT_LIB = -lrt
CFLAGS = -Os

LIB_SRC = lib/tinyosc.c lib/osc_snippet.c factories/*.c

all: osc-macro

osc-macro: main.c $(LIB_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -I include/ -Wall -Wextra -o osc-macro $^ $(LDLIBS) $(RT_LIB)

# ---------- Local Dev Recipes ----------
dev: RT_LIB =
dev: osc-macro

run: dev
run:
	./osc-macro $(ARGS)

asm: main.c $(LIB_SRC)
	rm -rf asm \
		&& mkdir asm \
		&& cd asm \
		&& $(CC) $(CPPFLAGS) $(CFLAGS) -I ../include/ -Wall -S $(addprefix ../,$^)

preprocess: pp.c
pp.c: main.c $(LIB_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I include/ -Wall -E $^ > pp.c

# ---------- Phony Targets ----------
clean:
	rm -f osc-macro pp.c
	rm -rf asm

.PHONY: clean

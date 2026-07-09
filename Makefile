RT_LIB = -lrt
CFLAGS = -Os

LIB_SRC = lib/tinyosc.c lib/osc_snippet.c

osc-macro: main.c $(LIB_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -I include/ -Wall -o osc-macro $^ $(LDLIBS) $(RT_LIB)

# ---------- Local Dev Recipes ----------
dev: RT_LIB =
dev: osc-macro

run: dev
run:
	./osc-macro

asm: main.c $(LIB_SRC)
	rm -rf asm \
		&& mkdir asm \
		&& cd asm \
		&& $(CC) $(CPPFLAGS) $(CFLAGS) -I ../include/ -Wall -S $(addprefix ../,$^)

# ---------- Phony Targets ----------
clean:
	rm osc-macro
	rm -rf asm

.PHONY: clean

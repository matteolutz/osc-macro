RT_LIB = -lrt
CFLAGS = -Os

LIB_SRC = lib/tinyosc.c lib/osc_snippet.c

main: main.c $(LIB_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -I include/ -Wall -o osc-macro $^ $(LDLIBS) $(RT_LIB)

send: main_send.c $(LIB_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -I include/ -Wall -o osc-macro-send $^ $(LDLIBS) $(RT_LIB)

# ---------- Local Debug Recipes ----------
debug: RT_LIB =
debug: main

debug-run: debug
debug-run:
	./osc-macro

# ---------- Phony Targets ----------
clean:
	rm osc-macro osc-macro-send

.PHONY: clean

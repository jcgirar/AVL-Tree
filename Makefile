CC       ?= gcc
AR       ?= ar
CFLAGS   := -std=c99 -O2 -Wall -Wextra -Wpedantic
ARFLAGS  := rcs

LIB_SRC  := avl.c
LIB_OBJ  := $(LIB_SRC:.c=.o)
LIB_NAME := libavl.a

EX_SRC   := example.c
EX_BIN   := avl_example

.PHONY: all lib example docs clean

# Compila tanto la librería estática como el ejemplo por defecto
all: $(LIB_NAME) $(EX_BIN)

# Genera la librería estática libavl.a
lib: $(LIB_NAME)

$(LIB_NAME): $(LIB_OBJ)
	$(AR) $(ARFLAGS) $@ $^

# Compila el programa de ejemplo
example: $(EX_BIN)

$(EX_BIN): $(EX_SRC) $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c avl.h
	$(CC) $(CFLAGS) -c $< -o $@

# Genera documentación HTML si Doxygen está instalado
docs:
	@command -v doxygen >/dev/null 2>&1 || { echo "Doxygen no está instalado."; exit 1; }
	doxygen -g Doxyfile >/dev/null
	sed -i 's/^PROJECT_NAME.*/PROJECT_NAME = "Intrusive AVL Tree in C"/' Doxyfile
	sed -i 's/^OPTIMIZE_OUTPUT_FOR_C.*/OPTIMIZE_OUTPUT_FOR_C = YES/' Doxyfile
	sed -i 's/^EXTRACT_STATIC.*/EXTRACT_STATIC = YES/' Doxyfile
	doxygen Doxyfile

# Limpia archivos compilados y documentación generada
clean:
	rm -f $(LIB_OBJ) $(LIB_NAME) $(EX_BIN) Doxyfile
	rm -rf html latex

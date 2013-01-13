# (!c) petter wahlman
#
CC      = g++

SRCDIR  = ./src
OBJDIR  = ./obj
BINDIR  = ./bin
INCLUDE = ./src

BINARY  = $(BINDIR)/bat

CXXFLAGS  = -I $(INCLUDE) -g -Wall -Wunused -std=c++11
LDFLAGS = -lm -lboost_program_options

VPATH   = $(SRCDIR)

BINARY_OBJ = \
	$(OBJDIR)/bat.o

$(OBJDIR)/%.o: %.cpp
	$(CC) -c $< $(CXXFLAGS) -o $@

.PHONY:
all:    make_dirs $(BINARY)

$(BINARY): $(BINARY_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY:
clean:
	@rm -rvf \
		$(BINARY) \
		$(BINARY_OBJ) \

.PHONY:
make_dirs:
	@mkdir -p $(OBJDIR) $(BINDIR) ~/local/bin

.PHONY:
install: all
	@cp -v $(BINARY) ~/local/bin \


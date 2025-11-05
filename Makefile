CXX = g++
CXXFLAGS = -std=gnu++17 -O2 -Wall -Wextra -pthread
SRCDIR = src
BINDIR = bin
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/shell.cpp \
          $(SRCDIR)/parser.cpp \
          $(SRCDIR)/jobs.cpp

all: $(BINDIR)/myshell

$(BINDIR)/myshell: $(SOURCES)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(SOURCES) -lreadline -o $(BINDIR)/myshell

clean:
	rm -rf $(BINDIR)

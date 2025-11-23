CC = gcc

OBJDIR = build
TARGET = $(OBJDIR)/out
SRCS = waitforwakeup.c fetch_addresses.c main.c libmemhandle/slice.c libmemhandle/sarray.c libmemhandle/hashy.c
OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))
# OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
CFLAGS=$(shell pkg-config --cflags libelogind)
LDFLAGS=$(shell pkg-config --libs libelogind) -lpthread

all: debug

debug: CFLAGS += -Wextra -Wall -g -O0
debug: $(OBJDIR) $(TARGET)

release: CFLAGS += -O3
release: $(OBJDIR) $(TARGET)
	strip $(TARGET)
	rm -v $(OBJDIR)/*.o
	
clean:
	rm -frv $(OBJDIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: %.c
	@mkdir -pv $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -pv $(OBJDIR)

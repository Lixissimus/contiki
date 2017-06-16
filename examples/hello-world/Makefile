CONTIKI_PROJECT = hello-world
all: $(CONTIKI_PROJECT)

# CFLAGS += -O0
# CFLAGS += -g
# CFLAGS += -gdwarf-2
# SMALL=0

DEBUG_CONF=1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

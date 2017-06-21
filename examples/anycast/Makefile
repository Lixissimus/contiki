CONTIKI_PROJECT = anycast
all: $(CONTIKI_PROJECT)

# CFLAGS += -O0
# CFLAGS += -g
# CFLAGS += -gdwarf-2
# SMALL=0

DEBUG_CONF=1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

MODULES += core/net/llsec/adaptivesec

CONTIKI = ../..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

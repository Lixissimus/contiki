CONTIKI_PROJECT = anycast anycast-rec
all: $(CONTIKI_PROJECT)

DEBUG_CONF=1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

MODULES += core/net/llsec/adaptivesec

CONTIKI = ../..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

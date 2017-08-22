CONTIKI_PROJECT = anycast
all: $(CONTIKI_PROJECT)

# DEBUG_CONF=1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

MODULES += core/net/llsec/adaptivesec
MODULES += core/net/orpl
MODULES += apps/servreg-hack
MODULES += examples/anycast/deployment

CONTIKI = ../..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

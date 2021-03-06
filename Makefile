ifndef CONTIKI
  ${error CONTIKI not defined! Do not forget to source /upb/groups/fg-ccs/public/share/nes/2018w/env.sh}
endif

ifndef TARGET
TARGET=sky
endif

PROJECT_SOURCEFILES+=aodv.c data.c

all: project.sky

upload: project.upload

sim: project.csc project.c
	java -jar $(CONTIKI)/tools/cooja/dist/cooja.jar -quickstart=project.csc -contiki=$(CONTIKI)
simulation:
	java -jar $(CONTIKI)/tools/cooja/dist/cooja.jar -contiki=$(CONTIKI)

CONTIKI_WITH_IPV4 = 1
CONTIKI_WITH_RIME = 1
include $(CONTIKI)/Makefile.include
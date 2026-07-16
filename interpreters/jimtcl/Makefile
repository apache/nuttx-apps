include $(APPDIR)/Make.defs

PROGNAME  = jimsh
PRIORITY  = 100
STACKSIZE = 8192

JIMTCL_VERSION  = 0.83
JIMTCL_TARBALL  = $(JIMTCL_VERSION).tar.gz
JIMTCL_UNPACK   = jimtcl
JIMTCL_URL_BASE = https://github.com/msteveb/jimtcl/archive/refs/tags
JIMTCL_URL      = $(JIMTCL_URL_BASE)/$(JIMTCL_TARBALL)
JIMTCL_SRC      = $(JIMTCL_UNPACK)

MAINSRC = $(JIMTCL_SRC)$(DELIM)jimsh.c

# Obtain object list from jimtcl Makefile
JIMTCL_OBJS := $(shell test -f $(JIMTCL_SRC)$(DELIM)Makefile		\
                       && ( cat $(JIMTCL_SRC)$(DELIM)Makefile;		\
                            printf 'print-%%:\n\techo $$($$*)' )	\
                       | $(MAKE) -f - -s print-OBJS) initjimsh.o

EXTOBJS = $(addprefix $(JIMTCL_SRC)$(DELIM), $(JIMTCL_OBJS))

CFLAGS += -I$(JIMTCL_SRC)

$(JIMTCL_SRC)$(DELIM)%.o: $(JIMTCL_SRC)$(DELIM)Makefile
	$(Q) $(MAKE) -C $(JIMTCL_SRC) $*.o

$(JIMTCL_SRC)$(DELIM)jimsh.c: $(JIMTCL_SRC)$(DELIM)Makefile

$(JIMTCL_SRC)$(DELIM)_initjimsh.c: $(JIMTCL_SRC)$(DELIM)Makefile
	$(Q) $(MAKE) -C $(JIMTCL_SRC) $(notdir $@)

$(JIMTCL_SRC)$(DELIM)Makefile:
	$(Q) cd $(dir $@)					\
	  && ./configure --host=$(patsubst %-,%,$(CROSSDEV))	\
	                 CFLAGS="$(CFLAGS)"

# Get the source code and apply a small patch because environ is
# not settable in Nuttx. This is not required for basic jimtcl
# but build would fail without commenting it out.
$(JIMTCL_TARBALL):
	$(Q) echo "Downloading $(JIMTCL_TARBALL)"
	$(Q) curl -O -L $(JIMTCL_URL)
	$(Q) echo "Unpacking $(JIMTCL_TARBALL) to $(JIMTCL_UNPACK)"
	$(Q) tar -xvzf $(JIMTCL_TARBALL)
	$(Q) mv jimtcl-$(JIMTCL_VERSION) $(JIMTCL_UNPACK)
	$(Q) sed -i '/^void Jim_SetEnviron/,/^}/{s/^/\/* /;s/$$/ *\//}'	\
	  $(JIMTCL_SRC)$(DELIM)jim.c

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(JIMTCL_UNPACK)/.git),)
context:: $(JIMTCL_TARBALL)
endif

clean::
	$(Q) test -f $(JIMTCL_SRC)$(DELIM)Makefile	\
	  && $(MAKE) -C $(JIMTCL_SRC) clean

distclean:: clean
	$(call DELDIR, $(JIMTCL_UNPACK))
	$(call DELFILE, $(JIMTCL_TARBALL))

include $(APPDIR)/Application.mk

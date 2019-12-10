# ----------------------------------------------------------------------------
# BeOS target setup
# ----------------------------------------------------------------------------

# compiler flags
CFLAGS += -std=c99 -Dnsfrontier -g -O0
CFLAGS += -I$(PREFIX)/include -fsanitize=address -fsanitize-address-use-after-scope

CXXFLAGS += -Dnsfrontier -g -O0 -std=c++17
CXXFLAGS += -I$(PREFIX)/include `pkg-config libfrontier --cflags` -fsanitize=address

LDFLAGS += `pkg-config libfrontier --libs` -lstdc++ -fsanitize=address

#VERSION_FULL := $(shell sed -n '/_version.*=.*"/{s/.*"\(.*\)".*/\1/;p;}' desktop/version.c))
#VERSION_MAJ := $(shell sed -n '/_major/{s/.* = \([0-9]*\).*/\1/;p;}' desktop/version.c)
#VERSION_MIN := $(shell sed -n '/_minor/{s/.* = \([0-9]*\).*/\1/;p;}' desktop/version.c)

HARCH := $(shell getarch)
HPARCH := $(shell getarch -p)
ifneq ($(HARCH),$(HPARCH))
PKGSARCH := _$(HARCH)
endif

PKGNAME := netsurf$(PKGSARCH)-$(VERSION_MAJ).$(VERSION_MIN)-1-$(HPARCH).hpkg
HAIKU_TARGET_DIR := netsurf_pkg

# Identity of the packager
PACKAGER := $(shell git config --get user.name) <$(shell git config --get user.email)>

# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# sources purely for the Frontier build
S_FRONTEND := gui.cpp browserwindow.cpp browserwidget.cpp plotters.cpp misc.cpp layout.cpp bitmap.cpp

# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_FRONTEND)
EXETARGET := NetSurf

# The filter and target for split messages
#MESSAGES_FILTER=beos
#MESSAGES_TARGET=$(FRONTEND_RESOURCES_DIR)

# ----------------------------------------------------------------------------
# Resources
# ----------------------------------------------------------------------------

#RDEF_BEOS := res.rdef
#RDEF_BEOS := $(addprefix $(FRONTEND_SOURCE_DIR)/,$(RDEF_BEOS))

#RDEF_IMP_BEOS := res_import.rdef
#RDEF_IMP_BEOS := $(addprefix $(OBJROOT)/,$(subst /,_,$(RDEF_IMP_BEOS)))

#RDEP_BEOS := \
#	adblock.css beosdefault.css default.css internal.css quirks.css	\
#	netsurf.png favicon.png ca-bundle.txt \
#	credits.html licence.html welcome.html maps.html SearchEngines

#RDEP_BEOS := $(addprefix $(FRONTEND_RESOURCES_DIR)/,$(RDEP_BEOS))	\
#	$(wildcard $(FRONTEND_RESOURCES_DIR)/icons/*.png)		\
#	$(wildcard $(FRONTEND_RESOURCES_DIR)/??/*)			\
#	$(wildcard $(FRONTEND_RESOURCES_DIR)/throbber/throbber*.png)

#RSRC_BEOS = $(addprefix $(OBJROOT)/,$(subst /,_,$(patsubst %.rdef,%.rsrc,$(RDEF_BEOS))))
#RESOURCES = $(RSRC_BEOS)

#$(RDEF_IMP_BEOS): $(RDEP_BEOS)
	#$(VQ)echo "     GEN: $@"
	#$(Q)n=5000; for f in $^; do echo "resource($$n,\"$${f#$(FRONTEND_RESOURCES_DIR)/}\") #'data' import \"$${f#$(FRONTEND_SOURCE_DIR)/}\";"; n=$$(($$n+1)); done > $@

#$(RSRC_BEOS): $(RDEF_BEOS) $(RDEF_IMP_BEOS)
#	$(VQ)echo "      RC: $<"
	#$(Q)$(BEOS_RC) -I $(FRONTEND_SOURCE_DIR) -o $@ $^


# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

install-frontier:

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

package-frontier: $(PKGNAME)
	$(VQ)echo Creating $(PKGNAME)

$(PKGNAME): $(EXETARGET)


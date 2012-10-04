SRCDIR = src
TARGETS = all prcat clean

$(TARGETS):
	@$(MAKE) -C $(SRCDIR) $@ $(MAKEFLAGS)


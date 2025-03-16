#############################################################################
# Makefile for building: ssws (simple stupid web server)
#############################################################################
CFLAGS = -g -O2 -Wall -pipe -Wstrict-prototypes -Wdeclaration-after-statement \
	 -fsanitize=address
OBJECTS = main.o http_header_parser.o ssws_core.o
PROGRAM = ssws
CPPFLAGS := -D_FORTIFY_SOURCE=3

ifndef ($(findstring $(MAKEFLAGS),s),s)
ifndef V
	E_CC	= @echo ' CC    '$@;
	E_LD	= @echo ' LD    '$@;
endif
endif

all: $(PROGRAM)

dep_files := $(foreach f, $(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))

$(dep_dirs):
	@mkdir -p $@

missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
dep_file = $(dir $@).depend/$(notdir $@).d
dep_args = -MF $(dep_file) -MQ $@ -MMD -MP

dep_files_present := $(wildcard $(dep_files))
ifneq ($(dep_files_present),)
include $(dep_files_present)
endif

%.o: %.c $(missing_dep_dirs)
	$(E_CC)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(dep_args) $< -o $@

$(PROGRAM): $(OBJECTS)
	$(E_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM) $(OBJECTS) $(LDLIBS)

.PHONY: clean
clean:
	-$(RM) $(OBJECTS) *~ core.*
	-$(RM) -r $(dep_dirs)

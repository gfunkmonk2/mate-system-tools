##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = mate-services
bin_PROGRAMS = mate-services-admin

## SUBDIRS =
INCLUDES += $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

mate_services_admin_LDADD = $(GST_TOOL_LIBS)
mate_services_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES)
mate_services_admin_SOURCES = \
	main.c \
	callbacks.c \
	callbacks.h \
	service.c \
	service.h \
	table.c \
	table.h \
	service-settings-table.c \
	service-settings-table.h \
	services-tool.c \
	services-tool.h

toolpixmaps =

-include $(top_srcdir)/git.mk

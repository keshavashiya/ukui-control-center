NULL =

themedir = $(datadir)/icons/ukui-icon-theme
size = 22x22
context = status

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA = 			 	      	\
	audio-input-microphone-high.png		\
	audio-input-microphone-low.png		\
	audio-input-microphone-medium.png	\
	audio-input-microphone-muted.png	\
	$(NULL)

EXTRA_DIST =		\
	$(icons_DATA)	\
	$(NULL)


-include $(top_srcdir)/git.mk
clean-local:
	rm -f *~ Makefile.in Makefile

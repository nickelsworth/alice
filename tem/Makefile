FLAGS=   -Wno-return-type -Wno-format -Wno-int-to-pointer-cast

OBJS=	template.o tokentable.o actiontable.o classtable.o nodetable.o

SRCS=   template.c tokentable.c actiontable.c classtable.c nodetable.c

CC = clang -I ../h -std=gnu89

# ../bsrc/bas.tpl: btemplate basic.src
#	btemplate basic.src ../bsrc/bas.tpl

template: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o template

$(OBJS):
	$(CC) $(FLAGS) -c $*.c

aptempla.suf: pascal.src
	template -l F=130 L=132 pascal.src aptempla.suf
	cp aptempla.suf ../src/alice.tpl

download: $(SRCS) *.src *.h
	isend $?
	touch download

pas.tpl: pascal.src
	template F=130 L=132 pascal.src pas.tpl

nlbegin.suf: nlbegin.src
	template -l F=130 L=132 nlbegin.src nlbegin.suf

showend.suf: showend.src
	template -l F=130 L=132 showend.src showend.suf

caps.suf: caps.src
	template -l F=130 L=132 caps.src caps.suf

all: showend.suf nlbegin.suf aptempla.suf

dos: showend.suf nlbegin.suf aptempla.suf
	doscp -r showend.suf nlbegin.suf aptempla.suf a:

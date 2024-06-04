# Para revisar las opciones de compilacion y ejecucion,
# ingrese en el terminal el comando: make

ALUM=pedir.c
PROF=test-pedir.c pss.c spin-locks.c swap.s
HDRS=pedir.h pss.h spin-locks.h
XTRA=

NAME = $(patsubst %.c,%,$(ALUM))

SHELL=bash -o pipefail

INCLUDE=
CFLAGS=$(XTRA) -Wall -Werror -Wno-unused-function -pedantic -std=c18 $(INCLUDE)
LDLIBS=-pthread

MAK=make --no-print-directory

readme:
	@less README.txt

$(NAME).bin $(NAME).bin-g $(NAME).bin-mem: $(ALUM) $(PROF) $(HDRS)

run-mem: $(NAME).bin-mem
	./$(NAME).bin-mem

run-g: $(NAME).bin-g
	./$(NAME).bin-g

run: $(NAME).bin
	./$(NAME).bin

ddd: $(NAME).ddd

ddd-san: $(NAME).ddd-san

zip:
	@rm -f resultados.txt $(NAME).zip
	@echo "Sistema operativo utilizado" > resultados.txt
	@uname -a >> resultados.txt
	@cat resultados.txt        
	@echo ==== make run ==== | tee -a resultados.txt
	@$(MAK) -B run | tee -a resultados.txt
	@echo ==== make run-g ==== | tee -a resultados.txt
	@$(MAK) -B run-g | tee -a resultados.txt
	@echo ==== make run-mem ==== | tee -a resultados.txt
	@$(MAK) -B run-mem | tee -a resultados.txt
	@echo ==== zip ====
	zip -r $(NAME).zip resultados.txt $(NAME).c
	@echo "Entregue por u-cursos el archivo $(NAME).zip"


%.bin: %.c
	gcc -O -DOPT=1 $(CFLAGS) $^ $(LDLIBS) -o $@

%.bin-g: %.c
	gcc -g $(CFLAGS) $^ $(LDLIBS) -o $@

%.bin-san: %.c
	gcc -g -DSAN=1 -fsanitize=thread -DSANITIZE $(CFLAGS) $^ $(LDLIBS) -o $@

%.bin-mem: %.c
	gcc -g -DSAN=1 -fsanitize=address -fsanitize=undefined -DSANITIZE $(CFLAGS) $^ $(LDLIBS) -o $@

%.ddd: %.bin-g
	ddd $(*F).bin-g &

%.ddd-san: %.bin-san
	ddd $(*F).bin-san &

FORCE:

clean:
	rm -f *.o *.log *.bin* core

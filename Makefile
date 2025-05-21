# Cible par défaut
all: test programme test_tp5_exec test_tp6_exec

# Nettoyer tous les fichiers générés
clean:
	rm -f *.o *.s compilateur test test.s programme sortie.s sortie_tp5.s sortie_tp6.s tokeniser.cpp tokeniser.o test_tp5_exec test_tp6_exec

# Générer tokeniser.cpp à partir de tokeniser.l avec flex++
tokeniser.cpp: tokeniser.l
	flex++ -d -otokeniser.cpp tokeniser.l

# Compiler tokeniser.cpp en objet
tokeniser.o: tokeniser.cpp
	g++ -c tokeniser.cpp

# Compiler compilateur.cpp et linker avec tokeniser.o et libfl (flex)
compilateur: compilateur.cpp tokeniser.o
	g++ -ggdb -o compilateur compilateur.cpp tokeniser.o -lfl

# Générer le code assembleur test.s à partir de compilateur et test.p (TP3)
test.s: compilateur test.p
	./compilateur < test.p > test.s

# Compiler le code assembleur en exécutable test (TP3)
test: test.s
	gcc -ggdb -no-pie -fno-pie test.s -o test

# Générer le code assembleur sortie.s à partir de compilateur et test_tp4.p (TP4)
sortie.s: compilateur test_tp4.p
	./compilateur < test_tp4.p > sortie.s

# Compiler le code assembleur en exécutable programme (TP4)
programme: sortie.s
	gcc -ggdb -no-pie -fno-pie sortie.s -o programme

# Générer le code assembleur sortie_tp5.s à partir de compilateur et test_tp5.p (TP5)
sortie_tp5.s: compilateur test_tp5.p
	./compilateur < test_tp5.p > sortie_tp5.s

# Compiler le code assembleur en exécutable test_tp5_exec (TP5)
test_tp5_exec: sortie_tp5.s
	gcc -ggdb -no-pie -fno-pie sortie_tp5.s -o test_tp5_exec

# Générer le code assembleur sortie_tp6.s à partir de compilateur et test_tp6.p (TP6)
sortie_tp6.s: compilateur test_tp6.p
	./compilateur  test_tp6.p > sortie_tp6.s

# Compiler le code assembleur en exécutable test_tp6_exec (TP6)
test_tp6_exec: sortie_tp6.s
	gcc -ggdb -no-pie -fno-pie sortie_tp6.s -o test_tp6_exec

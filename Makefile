# Cible par défaut
all: test

# Nettoyer tous les fichiers générés
clean:
	rm -f *.o *.s compilateur test tokeniser.cpp test.s

# Générer tokeniser.cpp à partir de tokeniser.l avec flex++
tokeniser.cpp: tokeniser.l
	flex++ -d -otokeniser.cpp tokeniser.l


# Compiler tokeniser.cpp en objet
tokeniser.o: tokeniser.cpp
	g++ -c tokeniser.cpp

# Compiler compilateur.cpp et linker avec tokeniser.o et libfl (flex)
compilateur: compilateur.cpp tokeniser.o
	g++ -ggdb -o compilateur compilateur.cpp tokeniser.o -lfl

# Générer le code assembleur test.s à partir de compilateur et test.p
test.s: compilateur test.p
	./compilateur < test.p > test.s

# Compiler le code assembleur en exécutable test
test: test.s
	gcc -ggdb -no-pie -fno-pie test.s -o test

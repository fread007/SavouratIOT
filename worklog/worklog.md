# Introduction

This is **your worklog** when discovering programming for embedded systems. 
It should contain what you will need to reopen this project in 6 months 
or a year and be able to understand and evolve it.

Also, as an Appendix, there should be the necessary details about what is
new to you, something like a cheat sheet in some sense. Something you can 
go back to when your memory will not server you well.

# First Sprint

## build

Presentation du Makefile :

*Definition des variable*

- QEMU => specifie quelle QEMU utiliser 
- TOOLCHAIN => toolchaine a utiliser
- BOARD => nom de la carte utiliser 
- MEMSIZE => taille de la memoire en KB

*Specification de QEMU*

- CPU => set de processeur a utiliser 
- VGA => selection du mode graphique
- SERIAL => specifie le mode serial(stdout/stdin)
- MEMORY => specifie la memoire utiliser avec la metrique
MACHINE => specifie le nom de la machine a utiliser

*Flague*

- CFLAGS => Rassemble tout les flague precedent -nostdlib(n'utilise pas les lib standare) -ffreestanding (n'utilise pas de systeme d'exploitation)
- ASFLAGS => flague lié a l'assembleur
- LDFLAGS => flague lié au linker -nostdlib(specifie de ne pas utiliser stdlib) -static(specifie le mode d'ajout des dependance, en mode statique elle sont "collé" au debut du programme)

*machine* 

Il est possible de specifier lors de la commande make une autre machine en nitialisant le flague MACHINE, si ce n'est pas le cas on utilise la machine de base.

*Le dossier build*

Le dossier build regroupe les fichier cree lors de la compilation faite par le makefile. Dans le dossier build on cree des sous dossier portant le nom de la machine utiliser.

*Les differente fonction make*

- clean => efface un sousdossier de build
- clean-all => efface tout le dossier build
- run => lance la machine voulut
- debug => lance la machine et gdb

*lien entre toolchain et QEMU*

La configuration de la toolchain et QEMU sont relié car on specifie le type de processeur, et ils doivent etre les meme.

*Linker*

le linker utilise le scripte versatile pour cree les fichier, il utilise les option de LDFLAGUES expliquer si dessus pour la creation des fichier.

*Demarage*

L'espace dedier au demarage ce situe a l'adresse 0x0 et un espace de 16 Mo est reserver.
Dans notre fichier versatile.ld on vois qu'il copie a l'adresse 0x0 les exeption, dont la premiere est l'exeption "reset_handler_addr" qui amene a l'adresse du code de startup qui lance le programme.

## Execution

*Commande pour gdb*

- make debug
- gdb-multiarch kernel.elf
- dans gdb : gdb tar rem:1234

*Boot sequence*

Les exeption sont contenue dans le vecteur d'exeption qui lors de la reception d'une exeption vas changer le pc a la commande a executer en cas de reception de celle si, puis vas rediriger vers la suite d'instruction voulut.
La section BSS reserve un espace pour les variable statique, elle est donc variable en fonction de la taille de ces variable.
La Stack est alors definit a + 0x1000 qui represente 4Kb soit la taille de la stack. On saute 4Kb car la stack commence a une adresse la plus haute puis desens.

## Main Loop

la loop principal verifie qu'il y a un caractere a lire, si ce n'est pas le cas elle continue a la prochaine boucle (C'est a dire quelle regarde indefiniment si il y a un caractere). Si il y a un caractere il verifie que ce n'est pas le caractere de fin, sa ne l'ai pas il envois le caractere, si non il envois la fin de ligne et le retours a la ligne.

Quand on definie ECHO_ZZZ, on ajoute a la boucle un compteur qui au bout de 50000000 passage dans la boucle, envois ZZZ puis recommence.

## Console

Le programme ne fait que renvoyais le caractere que l'on a envoyais, ce cractere codder en ASCII est ecrit sur un octee. Pour verifier cela j'ai pu ajouter 1 au caractere envoyais, renvoyais 0X41 (le 'A') a n'importe quelle touche caractere recut.

*Ajout de kprintf*

Pour ajouter ce fichier je l'ai ajouter au makefile dans les objet a cree "objs = ... kprintf" 

les fleche envois les suite suivante :
- up:	 27,91,65 -> esc,[,A
- down:  27,91,66 -> esc,[,B
- right: 27,91,67 -> esc,[,C
- left:  27,91,68 -> esc,[,D

et la touche de supresion :
- del:   27,91,51,126 -> exc,[,3,~

On vois que toute c'est action commence par "esc[".

Nous pouvons utiliser c'est commande avec la fonction kprintf et le code dans la chaine e carractere, par exemple pour effacer l'ecran "kprintf("\033[2J\033[H");".

*console.c*

Pour implementer la premiere version de console.c j'ai ajouter un etat pour gere les ligne de commande, et un buffer pour stocker les caractere de la ligne en cours.
Les fonction envoye les caractere a la console, et certainne attende des caractere en retours, en particulier la fonction cursor_position qui attend de recevoir les coordonner du curseur. Cette attente est bloquante car on restent dans celle si temps que l'on a pas recu entierement les coordonner du curseur.


## consigne
- [Understanding the execution](./execution.md)
- [Advanced debugging](./debugging.md)


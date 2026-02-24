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


boucle "_start" de base qui ne fait rien a part initialiser le programme, le travaille est fait dans les handler.
attention de ce qu'il y a dans le handler -> il ne faut pas que sa dure -> enleveer du handler la partie complexe en dehors.
pour uart -> possibiliter de cree un buffer -> conflie sur le buffer
            -> possibilter de cree une liste (ajout a la fin enleve au debut) -> probleme de concurence
            -> interompe les interuction le temps du'une section (halt reactive les interuption) -> probleme alocation memoire n'est pas en section critique -> solution bien implementer une ring/cercular buffer (structure donée) !pas de out sans full et pas de get sans empty!

*consoleListe*

console liste contien la version de console qui implemente une liste d'evenements, quand un evenement (clavier ou timer) intervient il est ajouter a une liste avec un temps de tretement,la fonction a appeler et un argument. 
Dans la boucle while on fait passer le temps en enlever 1 au temps de tretement, on verifie si il y a un evenement a ajouter et on traite les elements a traiter.

## Interuption

Dans le fichier console.c on y trouve la boucle principal du programme, ainsi que le handler et la gestion de l'interuption UART0. Une fois cette interuption relevé on peut dans la boucle principal lire les caractere qui on etais recut.
Afin de limiter le temps dans le handler (car les interuption sont bloquer a ce moment) on ajoute les caractere recut a une ring mais on ne les traite pas. 
Dans le fichier isr.c la fonction principal est appeler quand une interuption est releve, il y est deduit de quelle interuption on parle et ou continuer l'execution (quelle handler est associer a cette interuption). Ensuite on marque que 'on a traiter l'interuption, pour l'UART0 un bit est dedier a cella dans le composant, et on le notife aussi au VIC.
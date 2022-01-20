Projet : planification de tâches
================================

**L3 Informatique - Système**

Il est important de bien lire le sujet jusqu'au bout et de bien y réfléchir
avant de se lancer dans la programmation du projet.

## Sujet : `saturnd` et `cassini`, un outil de planification

Le but du projet est de programmer un couple démon-client permettant à
**un** utilisateur d'automatiser l'exécution périodique de tâches à des
moments spécifiés, à la manière de l'outil `cron`.

Le rôle du démon, `saturnd`, consiste à exécuter les tâches définies
par le client. Pour cela, il doit _indéfiniment_ :
  - attendre les requêtes du client, `cassini`, et y répondre;
  - exécuter les tâches aux dates demandées.

Le client, `cassini`, permet d'envoyer des requêtes au démon :
définition d'une nouvelle tâche, demande de la liste des tâches définies,
demande d'information sur le déroulement d'une tâche...

### Communication

Pour communiquer, `saturnd` et `cassini` utiliseront deux tubes nommés :
  - un _tube nommé de requête_ (messages client -> démon, appelés _requêtes_)
  - un _tube nommé de réponse_ (messages démon -> client, appelés _réponses_)
 
Ces fichiers sont créés par le démon au démarrage s'ils n'existent pas.

Les requêtes et les réponses devront respecter le format décrit dans le
fichier [protocole.md](protocole.md).


### Le démon

Une fois lancé (en tâche de fond), le démon tourne jusqu'à ce qu'une
demande explicite de terminaison lui soit envoyée. Il doit dans ce cas
terminer le plus proprement possible. 

Pour mener à bien son travail, le démon doit impérativement maintenir
des structures contenant les informations nécessaires pour satisfaire les
requêtes du client, qu'il s'agisse des tâches à effectuer, ou du
déroulement des tâches déjà effectuées. On demande par ailleurs que le
démon puisse reprendre son travail après une éventuelle interruption,
(c'est-à-dire si un processus `saturnd` a terminé, et qu'un nouveau
processus est lancé).
Toutes les informations nécessaires doivent donc être stockées sur
disque, dans une arborescence organisée par tâche; chaque tâche à 
effectuer reçoit un identifiant unique, qui sert de nom au répertoire 
contenant tous les fichiers qui la concernent : les arguments de la ligne
de commande à exécuter, les dates à respecter, la liste des valeurs de
retour des exécutions précédentes (datées), la sortie standard et la
sortie erreur standard de la dernière exécution ...


### Le client

Le client recevra en ligne de commande le descriptif de la requête à
envoyer au démon (via le tube de requête). Il ouvrira ensuite le tube
de réponse pour y recevoir la réponse, et affichera le cas échéant cette
réponse sur sa sortie standard avant de terminer.

#### Exemple d'utilisation :

```bash
$ cassini -l
$ cassini -c echo coucou
0
$ cassini -c -m 0-2,5,10 toto.sh
1
$ cassini -l
1: 0-2,5,10 * * toto.sh
0: * * * echo coucou
$ cassini -x 0                # un peu plus tard...
2021-10-27 17:58:00 0
2021-10-27 17:59:00 0
2021-10-27 18:00:00 0
2021-10-27 18:01:00 0
2021-10-27 18:02:00 0
2021-10-27 18:03:00 0
2021-10-27 18:04:00 0
2021-10-27 18:05:00 0
2021-10-27 18:06:00 0
$ cassini -x 1
2021-10-27 18:00:00 0
2021-10-27 18:01:00 0
2021-10-27 18:02:00 0
2021-10-27 18:05:00 0
$ cassini -o 0
coucou
$ cassini -r 0
$ cassini -l
1: 0-2,5,10 * * toto.sh
```

**Remarque** : les options de la ligne de commande devront impérativement
inclure celles définies dans [cassini.c](src/cassini.c) et avoir exactement
le comportement demandé, y compris le format de sortie, à des fins de
test de vos projets. Vous êtes bien entendu libres d'ajouter d'autres
options si vous le souhaitez, par exemple pour produire un autre format
d'affichage.


## Modalités d'exécution (et de test)

Il est demandé de produire un couple démon-client fonctionnel sur
`lulu`, pour un unique utilisateur exécutant les deux programmes. Il
n'est pas demandé que le démon puisse gérer plusieurs requêtes
simultanées, ni s'adapter à un autre client potentiellement
non-coopératif (par exemple envoyant volontairement des requêtes
incomplètes).

Un jeu de tests est fourni pour s'assurer du strict respect du protocole
de communication. Lancer le script `run-cassini-tests.sh` pour effectuer
ces tests.

Pensez également à vérifier la bonne gestion mémoire de vos programmes,
par exemple avec l'outil `valgrind`.


## Modalités de réalisation

Le projet est à faire par équipe de 3 étudiants, exceptionnellement 2.
La composition de chaque équipe devra être envoyée par mail à
l'enseignante responsable du cours de systèmes avec pour sujet `[SY5]
équipe projet` **au plus tard le 15 novembre 2021**, avec copie à chaque
membre de l'équipe.

Chaque équipe doit créer un _fork_ privé de ce dépôt `git` **dès la
lecture du sujet** (maintenant!) et y donner accès en tant que `Reporter`
à tous les enseignants du cours de Système : Guillaume Geoffroy, Colin
Gonzalez, Patrick Lambein-Monette, Anne Micheli et Dominique Poulalhon.
Le dépôt devra contenir un fichier `AUTHORS` donnant la liste des membres
de l'équipe (nom, prénom, numéro étudiant et pseudo(s) sur le gitlab).

En plus du code source de vos programmes, vous devez fournir un `Makefile` tel que :
  - l'exécution de `make` à la racine du dépôt crée (dans ce même répertoire) 
    les exécutables `cassini` et (pour le rendu final) `saturnd`,
  - `make distclean` efface tous les fichiers compilés,

ainsi qu'un mode d'emploi, et un fichier `ARCHITECTURE` (idéalement en
format Markdown, donc avec extension `.md`) expliquant la stratégie
adoptée pour répondre au sujet (notamment l'architecture logicielle, les
structures de données et les algorithmes implémentés).

Les seules interdictions strictes sont les suivantes : plagiat (d'un
autre projet ou d'une source extérieure à la licence), utilisation de
la fonction `system` de `stdlib.h` et des fonctions  de `stdio.h` 
manipulant le type `FILE`.

Pour rappel, l'emprunt de code sans citer sa source est un
plagiat. L'emprunt de code en citant sa source est autorisé, mais bien
sûr vous n'êtes notés que sur ce que vous avez effectivement produit.
Donc si vous copiez l'intégralité de votre projet en le spécifiant
clairement, vous aurez quand même 0 (mais vous éviterez une demande de
convocation de la section disciplinaire).

Pour toute question auxquelle ce document ne répond pas, merci d'utiliser
le salon `questions-projet` du [serveur
discord](https://discord.gg/7ArJtu8Xnv).


## Modalités de rendu

Le projet donnera lieu à deux rendus :

### Premier rendu le 6 décembre

Un premier rendu correspondant au client est demandé pour le 6 décembre,
sous la forme d'un tag `RENDU 1` sur le dépôt git.  Il ne donnera pas
lieu à une soutenance mais à des tests automatiques pour s'assurer du
respect du protocole de communication. Pour que ces tests se déroulent
correctement, `make` doit compiler le client sous le nom `cassini` à la
racine de votre dépôt. 

Ce premier rendu sera noté sur 4 points.

### Deuxième rendu en janvier

Le projet final devra être rendu à une date encore à définir mais au plus
tard mi-janvier, pour des soutenances probablement entre le 18 et le 20
janvier.

### Participation effective

**Les projets sont des travaux de groupe, pas des travaux à répartir entre
vous** ("je fais le projet de Prog fonctionnelle, tu fais celui de
Systèmes"). La participation effective d'un étudiant au projet de son
groupe sera évaluée grâce, entre autres, aux statistiques du dépôt git et
aux réponses aux questions pendant la soutenance, et les notes des
étudiants d'un même groupe pourront être modulées en cas de participation
déséquilibrée. En particulier, un étudiant n'ayant aucun commit sur du
code et incapable de répondre de manière satisfaisante sera noté "DEF".


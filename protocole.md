Protocole de communication `saturnd`-`cassini`
=============================================

Les deux processus communiquent par envoi de messages sur deux tubes
nommés, le _tube de requête_ et le _tube de réponse_.
Chaque message est composé d'une succession de champs concaténés ayant chacun un type :


Les types de champs
===================


#### Les types entiers

`uint8`, `uint16`, `uint32`, `uint64` : entiers non signés sur 1, 2, 4 et
8 octets respectivement, en convention big-endian. 

`int8`, `int16`, `int32`, `int64` : entiers signés sur 1, 2, 4 et 8
octets respectivement, en convention big-endian, complément à 2.

_Remarque : le projet doit fonctionner sur `lulu`, sans exigence de
portabilité sur tous les systèmes POSIX; vous pouvez donc utiliser les
fonctions de conversion disponibles sous linux et listées dans `man 3
endian`_

#### Le type `string`

Un sous-champ de type `uint32` contenant la longueur `L` de la chaîne
(sans le 0 terminal), suivi des `L` octets correspondant au contenu de la
chaîne.

#### Le type `timing`

Décrit un ensemble de minutes (comme les 5 premiers champs d'une ligne de crontab). Composé d'une
succession de 3 sous-champs :

```
MINUTES <uint64>, HOURS <uint32>, DAYSOFWEEK <uint8>
```

Chacun des sous-champs est à voir comme un tableau de bits. 

`MINUTES` : bit de poids faible (n°0) = minute 0, ..., bit n°59 = minute 59.
  
 - Exemple : 0x00002000000007F0 = de la minute 4 à la minute 10, puis à la minute 45
             (équivalent dans crontab : 4-10,45)

`HOURS` : bit n°0 = heure 0, ..., bit n°23 = heure 23

`DAYSOFWEEK` : bit n°0 = dimanche, ..., bit n°6 = samedi.
 
 - Exemple : 0x5C (en binaire : 01011100) = du mardi au jeudi, et le samedi
             (équivalent dans crontab : 2-4,6)


#### Le type `commandline`

```
ARGC <uint32>, ARGV[0] <string>, ..., ARGV[ARGC-1] <string>
```

`ARGC` doit être au moins égal à 1. `ARGV[0]` contient le nom de la commande à appeler et doit être non vide.


Format des requêtes (messages client -> démon)
==============================================

Chaque requête commence par un champ `OPCODE` de type `uint16`, dont la
valeur indique quelle opération est demandée au serveur :

 - 0x4c53 ('LS') : LIST -- lister toutes les tâches
 - 0x4352 ('CR') : CREATE -- créér une nouvelle tâche
 - 0x524d ('RM') : REMOVE -- supprimer une tâche
 - 0x5458 ('TX') : TIMES_EXITCODES -- lister l'heure d'exécution et la valeur de retour
                                      de toutes les exécutions précédentes de la tâche
 - 0x534f ('SO') : STDOUT -- afficher la sortie standard de la dernière exécution de la tâche
 - 0x5345 ('SE') : STDERR -- afficher la sortie erreur standard de la dernière exécution de la tâche
 - 0x4b49 ('TM') : TERMINATE -- terminer le démon
 
Le format de la requête dépend de l'opération :

#### Requête LIST

```
OPCODE='LS' <uint16>
```

#### Requête CREATE

```
OPCODE='CR' <uint16>, TIMING <timing>, COMMANDLINE <commandline>
```

#### Requête REMOVE

```
OPCODE='RM' <uint16>, TASKID <uint64>
```

`TASKID` indique l'identifiant de la tâche à supprimer.

#### Requête TIMES_EXITCODES

```
OPCODE='TX' <uint16>, TASKID <uint64>
```

#### Requête STDOUT

```
OPCODE='SO' <uint16>, TASKID <uint64>
```

#### Requête STDERR

```
OPCODE='SE' <uint16>, TASKID <uint64>
```

#### Requête TERMINATE

```
OPCODE='TM' <uint16>
```



Format des réponses (messages démon -> client)
==============================================

Chaque réponse commence par un champ `REPTYPE` de type `uint16` :
 - 0x4f4b ('OK') : OK -- la requête s'est exécutée normalement
 - 0x4552 ('ER') : ERROR -- la requête ne s'est pas exécutée normalement

Le format de la réponse dépend du type de réponse (REPTYPE) et de la
requête à laquelle elle répond :


#### Réponse à LIST

Seule une réponse OK est possible :

```
REPTYPE='OK' <uint16>, NBTASKS=N <uint32>,
TASK[0].TASKID <uint64>, TASK[0].TIMING <timing>, TASK[0].COMMANDLINE <commandline>,
...
TASK[N-1].TASKID <uint64>, TASK[N-1].TIMING <timing>, TASK[N-1].COMMANDLINE <commandline>
```


#### Réponse à CREATE

Seule une réponse OK est possible :

```
REPTYPE='OK' <uint16>, TASKID <uint64>
```

`TASKID` représente l'identifiant de la tâche nouvellement créée. Cet
identifiant est unique. Les identifiants des tâches supprimées ne sont
pas réutilisés.


#### Réponse à REMOVE

Les réponses OK et ERROR sont possibles :

##### Réponse OK

```
REPTYPE='OK' <uint16>
```

##### Réponse ERROR

```
REPTYPE='ER' <uint16>, ERRCODE <uint16>
```

La seule valeur possible pour ERRCODE est :
 - 0x4e46 ('NF') : il n'existe aucune tâche avec cet identifiant
 

#### Réponse à TIMES_EXITCODE

Les réponses OK et ERROR sont possibles :

##### Réponse OK

```
REPTYPE='OK' <uint16>, NBRUNS=N <uint32>
RUN[0].TIME <int64>, RUN[0].EXITCODE <uint16>
...
RUN[N-1].TIME <int64>, RUN[N-1].EXITCODE <uint16>
```

Chaque champ `TIME` indique le nombre de secondes depuis le point de référence : 1970-01-01 00:00:00 (UTC).

Chaque champ `EXITCODE` contient :
 - soit le code de sortie de la tâche (sur 8 bits) si celle-ci s'est
   terminée par un appel à `exit(3)` ou `_exit(2)` ou par un `return`
   depuis `main()` (cf `man 2 wait`),
 - soit la valeur 0xFFFF, dans tous les autres cas.

##### Réponse ERROR

```
REPTYPE='ER' <uint16>, ERRCODE <uint16>
```

La seule valeur possible pour ERRCODE est :
 - 0x4e46 ('NF') : il n'existe aucune tâche avec cet identifiant


#### Réponse à STDOUT et STDERR

Les réponses OK et ERROR sont possibles :

##### Réponse OK

```
REPTYPE='OK' <uint16>, OUTPUT <string>
```

##### Réponse ERROR

```
REPTYPE='ER' <uint16>, ERRCODE <uint16>
```

Les valeurs possibles pour ERRCODE sont :
 - 0x4e46 ('NF') : il n'existe aucune tâche avec cet identifiant
 - 0x4e52 ('NR') : la tâche n'a pas encore été exécutée au moins une fois


#### Réponse à TERMINATE

Seule une réponse OK est possible :

```
REPTYPE='OK' <uint16>
```



Exemple
=======

Supposons par exemple que le client demande « exécuter la commande `echo test-1` tous les
mercredis à 9h00 et 14h00 » et que le démon réponde « la tâche a été crée avec l'identifiant 26
(0x1A) ». Voici les messages que le client et le démon s'échangeraient :

#### Requête du client

Telle que vue avec `hexdump -C` :

```
00000000  43 52 00 00 00 00 00 00  00 01 00 00 42 00 08 00  |CR..........B...|
00000010  00 00 02 00 00 00 04 65  63 68 6f 00 00 00 06 74  |.......echo....t|
00000020  65 73 74 2d 31                                    |est-1|
00000025
```

Découpée en champs et sous-champs :

```
OPCODE                 : 43 52                     |CR|

TIMING.MINUTES         : 00 00 00 00 00 00  00 01  |........|
TIMING.HOURS           : 00 00 42 00               |..B.|
TIMING.DAYSOFWEEK      : 08                        |.|

COMMAND.ARGC           : 00 00 00 02               |....|
COMMAND.ARGV[0].LENGTH : 00 00 00 04               |....|
COMMAND.ARGV[0].DATA   : 65 63 68 6f               |echo|
COMMAND.ARGV[1].LENGTH : 00 00 00 06               |....|
COMMAND.ARGV[1].DATA   : 74 65 72 74 2d 31         |test-1|
```


#### Réponse du serveur

Telle que vue avec `hexdump -C` :

```
00000000  4f 4b 00 00 00 00 00 00  00 1a                    |OK........|
0000000a
```

Découpée en champs et sous-champs :

```
REPTYPE : 4f 4b                    |OK|
TASKID  : 00 00 00 00 00 00 00 1a  |........|
```

# Introducción a Unix

## Sistema de archivos

Un sistema de archivos es la forma que tiene el sistema operativo de organizar los datos en una estructura o colección de archivos. UNIX considera como archivos no sólo a los archivos normales (en los que se guardan datos, programas, etc) sino también a los directorios y los dispositivos conectados al sistema.

El sistema de archivos en UNIX está organizado en una estructura jerárquica de directorios que comienza en el directorio *root* representado por / .

Los directorios que cuelgan de *root* pueden variar dependiendo del sistema, aunque los mostrados en el arbol a continuación, sí son comunes a todos los sistemas UNIX.

```
                              /(root)
                                 |
 ----------------------------------------------------
 |         |       |       |        |       |       |     
  /bin    /dev    /etc    /home    /lib    /tmp    /usr   
```

- */bin* contiene comandos y utilidades, son archivos ejecutables
- */dev* contiene los archivos que representan a los dispositivos conectados al sistema
- */etc* contiene comandos y archivos usados en la administración del sistema
- */home* contiene los archivos home de cada usuario del sistema
- */lib* contiene librerías utilizadas por diferentes programas y lenguajes
- */tmp* es el directorio donde se guardan los archivos temporales
- */usr* contiene archivos del sistema que son comunes a los usuarios como programas o documentación.

Los archivos pueden clasificarse dentro de las siguientes categorías:

- archivos normales
- Directorios
- archivos especiales
- Pipes

*Los archivos normales* pueden tener contenidos muy diferentes, por ejemplo texto, imágenes, ejecutables, etc. Todos los archivos tienen un propietario, los que se crean desde la cuenta del usuario le pertenecen y cada propietario es el que puede controlar los permisos de acceso a ese archivo.

*Los directorios* son archivos que contienen referencias a otros archivos. Aunque todos los archivos se encuentran dentro de algún directorio, no residen realmente dentro de él. El directorio mantiene dos informaciones básicas sobre cada archivo contenido en él: El nombre y un número llamado *inode number* que es un puntero que le indica al sistema donde encontrar toda la información que necesita del archivo.

A la correspondencia entre el nombre de un archivo y el *inode number* se le denomina *link*. Un mismo *inode number* puede tener asociados varios nombres y nos podemos referir al archivo por cada uno de estos nombres.

*Los dispositivos especiales* son archivos que representan dispositivos físicos como impresoras, dispositivos magnéticos, el ratón, etc. Se encuentran dentro del directorio */dev*. Por ejemplo, si la salida por pantalla de un programa no interesa se puede enviar  al dispositivo /dev/null que en realidad no tiene un dispositivo físico asociado, perdiendose de esta forma la salida del programa.

```
programa1 >/dev/null
```

*Los pipes* permiten enlazar la salida de un comando a la entrada de otro, esto se hace a través de la creación de un archivo temporal donde se guarda la salida del primer programa hasta que es leido por el segundo.

## Sintaxis de los comandos

La forma básica de un comando unix consta de tres partes aunque no todas tienen que estar presentes. La primera, que es necesaria siempre, es el nombre del comando. Luego vienen las opciones que son letras que se ponen precedidas del signo **-** y que le indican al comando que opción deseamos (algunas de estas opciones requieren un argumento). Por último están los argumentos del comando que pueden ser opcionales o no dependiendo del comando en concreto. Si se cometen errores en la sintaxis, como una opción inexistente, se indica por pantalla cual es la sintaxis de ese comando con sus opciones y argumentos, indicando entre corchetes **[]** los que son opcionales. También podemos ver la sintaxis en la ayuda del comando. Un ejemplo de comando con opciones y argumentos es: 

```
ls -la dir1
```

### **Comandos básicos relacionados con directorios

### *pwd*

Muestra el directorio en que está trabajando.

### *ls*

Muestra los archivos contenidos en un directorio. Tiene varias opciones muy útiles como **-l** para un formato largo con una serie de características de cada archivo. La opción **-a** muestra los archivos ocultos (archivos que empiezan por el caracter **.**). Se puede ordenar la salida por diferentes opciones como por fecha o alfabéticamente.

La información que muestra cuando se emplea la opción **-l** es:

- Una serie de 10 caracteres donde el primero nos informa del tipo de archivo que es y los nueve restantes sobre los permisos que tiene el archivo.
- Un número que indica el número de *links* al archivo.
- El propietario.
- El grupo al que pertenece.
- El tamaño en bytes
- La fecha de la última modificación
- El nombre del archivo (si es un *link* simbólico se muestra también el archivo al que hace referencia).

Este comando y otros como **cp**, **rm**, etc pueden usar una serie de caracteres llamados "*wildcards*" que permiten seleccionar archivos que siguen un cierto patrón en su nombre. Estos caracteres son: 

***** Representa cualquier cadena de carcteres 
**?** Representa un sólo caracter 
**[...]** Podemos poner dentro una lista de caracteres que son los posibles para un solo caracter. 

### Ejemplos:

```
cp \* ~/textos
```

Copia todos los archivos del directorio actual al subdirectorio textos del directorio home del usuario. 

Si se tiene una serie de archivos **cap1.tex cap2.tex cap3.tex capitulo.tex**, el comando 

```
mv cap?.tex ~
```

moverá los tres primeros al directorio home del usuario 

```
ls b[Aa]\*
```


hará una lista de todos los archivos cuya segunda letra sea una A o a y empiecen por b. En los corchetes puede incluirse un rango como por ejemplo [0-5] [a-z] [A-Z], que respectivamente significan cualquier número del 0 al 5, cualquier letra minúscula y cualquier letra mayúscula.

### mkdir

Crea un nuevo directorio.

### rmdir

Borra un directorio.

### cd

Cambia de directorio. Se puede dar la ruta completa al directorio (**cd /home/...**) o se puede dar de forma relativa al directorio en que se está (este directorio se representa por el caracter **.** mientras **..** representa el inmediatamente superior). También se puede dar la ruta relativa del directorio home del usuario con el caracter **~ (cd ~/texto)**. Para ir a home del usuario basta con ejecutar **cd** sin argumentos.

### Comandos básicos relacionados con el manejo de archivos

### *cp*

Es el comando para copiar archivos, por ejemplo: 

```
cp unix.tex ~/curso/texto.tex
```

copia el archivo unix.tex a /home/fcabrera/curso/ cambiandole el nombre a texto.tex . La opción **-i** preguntará si se quiere que sobreescriba en el caso de que el nombre del archivo coincida con el de otro archivo que ya existe. La opción **-r** permite copiar directorios incluyendo todos los archivos y directorios que cuelgan de él.

### *mv*

Es similar al comando anterior pero no hace una copia sino mueve el archivo (lo cambia de nombre). También está disponible la opción **-i**.

### *rm*

Borra archivos. También está la opción **-i** que pide confirmación para borrar cada archivo y la opción **-r** para borrar todos los archivos que cuelguen de un directorio.

### *ln*

Permite realizar *links* entre archivos. Cada archivo tiene un *inode number* y al menos un nombre. La correspondencia entre un *inode* y el nombre es un *link* y se pueden asociar varios nombres a un mismo archivo. Para crear un *link* ejecutamos el comando: 

```
ln filename1 linkname
```

Se puede también hacer un *link* de varios archivos a un directorio. Sin embargo no se pueden hacer *links* entre directorios, esto sólo lo puede hacer el super-user. Si se quiere hacer un *link* entre directorios hay que utilizar la opción **-s** que crea lo que se llama un *link* simbólico. Por ejemplo para hacer un *link* simbólico a un directorio llamado **/net/oso/scratch/fcabrera** desde el directorio home llamandolo **scratch** sólo se tiene que situarnos en el directorio home y ejecutar: 

```
ln -s /net/oso/scratch/fcabrera scratch
```

Ahora se tendría en el directorio home un directorio scratch que al acceder a él se accede a **/net/oso/scratch/fcabrera** . No es una copia de este directorio sino que físicamente sólo existe el directorio en el disco. También es necesario usar la opción **-s** entre archivos normales, si se encuentran en dispositivos físicos distintos. Para borrar un *link* se usa el comando **rm** que no borra el archivo sino el *link* a ese archivo. Por ejemplo: 

```
rm scratch 
```


no borra el directorio sino el *link* a **/net/oso/scratch/fcabrera**.

### *find*

Permite buscar un archivo en la estructura de directorios. Hay varias opciones de búsqueda como por ejemplo para buscar los archivos con un determinado nombre, archivos que tengan un determinado tamaño, que hayan sido modificados en los últimos **n** días etc. Por ejemplo: 

```
find . -name text
```

buscará cualquier archivo que se llame text que se encuentre en el directorio actual o cualquiera que cuelgue de él. 

Otro ejemplo que muestra una sintaxis más complicada es el siguiente: 

```
find . \( -name a.out -o -name '*.o' \) -mtime -3 -exec rm {} \;
```

Que borrara los archivos a.out y los que terminen en .o que han sido modificados en los últimos 3 días. La barra \ antes del paréntesis es para evitar que la shell le de un significado especial, las comillas son necesarias cuando se utilizan *wildcards* en los patrones de búsqueda.

##  Comandos útiles para trabajar con archivos de texto

### *page* y *more*

Para mostrar un archivo por pantalla de forma que espere cuando el terminal este lleno hasta que pulse alguna tecla.

### *cat*

Encadena una lista de archivos escribiéndolos uno a continuación de otro en la salida estandar.

### *grep*

Busca un patrón de caracteres en uno o varios archivos. Nos mostrara las líneas en que encuentre el patrón y si la búsqueda se realiza en más de un archivo indica primero el nombre del archivo. Por ejemplo el comando: 

```
grep -i galaxia \*.tex
```

Mostrará cada línea que contenga la palabra galaxia en cualquier archivo con extensión **.tex**, indicando en qué archivo se encuentran. La opción **-i** indica que no importa si es en mayusculas o minúsculas.

### *sort*

Permite ordenar las líneas de uno o varios archivos, hay varias opciones para realizar la ordenación. Por ejemplo: 

```
sort l1 l2 +1
```

ordena los archivos l1 y l2 por la segunda palabra (+1) y la salida es dirigida a la salida estándar.

### *diff*

Muestra las diferencias línea pínea de dos archivos de texto.

### *cmp*

Compara dos archivos y nos muestra el byte y número de línea donde ocurre la primera diferencia.

### *head* y *tail*

El comando *head* muestra las n primeras líneas de un archivo y *tail* las últimas líneas.

### Comandos útiles para examinar el espacio en disco memoria, procesos, usuarios, etc

### *df*

Para ver la cantidad de espacio en disco. La opcion **-k** nos lo da en kilobytes.

### *du*

Nos da la cantidad de disco usado por el directorio y todos los subdirectorios que cuelgan del directorio especificado. La opción **-k** nos lo da en kilobytes.

### *quota*

Con la opción **-v** nos muestra la cantidad de espacio que tenemos de cuenta en el home y la que tenemos ocupada.

### *top*

Nos muestra en pantalla los procesos que ocupan más CPU con el porcentaje correspondiente, así como otras informaciones como memoria de la máquina, memoria utilizada. La información se va actualizando periodicamente.

### *ps*

Nos permite ver los procesos que se están ejecutando.

### *who*

Para saber quien está conectado a la máquina en la que estamos trabajando.

### *rusers*

Nos muestra quien está en cada máquina del sistema. La opción **-l** nos da información más detallada de cada uno.

### *whereis* o *which*

Localiza un comando, es decir nos muestra la ruta de acceso del archivo ejecutable.

### *finger*

Muestra información sobre los usuarios como *username*, nombre completo, cuando a leido su mail por última vez, si está o no conectado y desde donde, etc. Podemos crear un archivo en nuestro home con el nombre **.plan** cuyo contenido se mostrará cuando alguien haga un finger. Podemos utilizar el *username* si lo sabemos, o bien buscar por el nombre o apellidos reales.

## Redirección de comandos y pipes

### Redirección de comandos

Cualquier dispositivo conectado al sistema (impresoras, terminal, dispositivo de audio, mouse, etc), es considerado por UNIX como un archivo.

Los comandos pueden necesitar leer y escribir información. Normalmente lo hacen en la *standard input* y *standard output* que es el terminal. Sin embargo ambas pueden redireccionarse a otros archivos, tanto archivos normales como dispositivos físicos.

Para redireccionar la entrada estándar se usa **<** seguido del nombre del archivo. Por ejemplo: 

```
mail fcabrera < file1
```

Envía un mail a fcabrera tomando como texto el contenido del archivo file1.

Para redireccionar la salida estandar hay varias opciones **(>, >&, >!, >&!, >>, >>&, >>!, >>&!)** seguido del archivo al que se dirige la salida.

**>** Envía el resultado del comando a un archivo reescribiendolo, si se añade **&** se envían también los erroes, si se añade **!** se asegura que se pueda escribir al archivo aunque tengamos definida la variable **noclobber** que evita la sobrescritura.

**>>** Envía el resultado a un archivo pero añadiendolo al final. En este caso si esta definida la variable **noclobber** se produciría un error si el archivo no existe previamente, a menos que añadamos **!**.

Ejemplos: 

```
cat sound.au > /dev/audio
```

Enviará el archivo de sonido sound.au a **/dev/audio**, que es el archivo asociado al dispositivo de audio. 
**crypt zyxst < texto > texto.crypt**

Este comando toma como entrada el archivo **texto**, lo codifica con la clave **zyxst** y lo escribe en el archivo **texto.crypt**.

### Pipes

Se denomina un "*pipeline*" a una serie de comandos separados por **|** o **|&**. En el primer caso la salida estandar del primer comando es direccionada a la entrada del segundo, mientras **|&** redirecciona la salida estandar y los errores.

Los *pipelines* y los comandos pueden separarse por **;** y se ejecutan secuencialmente, o pueden separarse con condicionales. **&&** ejecuta el segundo *pipe* si el primero se ejecuta sin errores (*standard error* es 0). **||** ejecuta el segundo *pipe* si se produce un error en la ejecución del primero (*standard error* distinto de 0).

Ejemplos: 

Para contar el número de usuarios de la máquina en que estamos trabajando 

```
who | wc -l
```

**who** tiene como salida una línea con información por cada usuario conectado a la máquina. Esto es enviado a la entrada del comando **wc -l**, que cuenta el número de líneas.

El siguiente ejemplo muestra los procesos ordenados por el porcentaje de CPU que emplean. 

```
ps -feao ruser,pid,pcpu,etime,comm | sort -r +2 | more
```

**ps** muestra los procesos en un determinado formato, el resultado es ordenado por el comando **sort** y se muestra por pantalla empleando el comando **more**.

El ejemplo: 

```
tar -cvf /dev/rmt/0lbn . || (tar -cvf /scfratch/fcabrera/backup .; compress backup; cp ) &
```

Primero produce un backup del directorio en que estamos en un dispositivo magnético. En caso de que fallara se haría un backup al disco de scratch y lo comprime para ahorrar espacio.

## El uso de las comillas en UNIX

UNIX trata de forma diferente los caracteres especiales dependiendo de si sencuentran entre comillas, y del tipo de comillas que sean. Vamos a ver varios ejemplos utilizando el comando **echo**, que simplemente escribe sus argumentos en la salida estandard separados por espacios y terminando con un retorno de línea.

### Las dobles comillas 

Veamos los siguientes ejemplos: 

```
echo \*
echo "\*"
```

En el primer caso la shell sustituye ***** por los nombres de todos los archivos en ese directorio separados por espacios. En el segundo caso el resultado nos muestra por pantalla el caracter *****. Esto se debe a que las comillas impiden la sustitución de ***** por nombres de archivos tratándolo como un caracter sin significado especial.

Otros caracteres que pierden su significado especial entre las comillas son ***, ?, >, <, >>, |**. Además las dobles comillas también mantienen el significado de los espacios en blanco.

### Comillas simples

El uso de las comillas simples puede producir resultados diferentes como por ejemplo:

```
echo $shell
echo "$shell"
echo '$shell'
```

Los dos primeros casos tienen el mismo efecto y muestra por pantalla el valor de la variable shell. Esto se debe a que el caracter **$**, que sirve para referirse al valor de una variable, no pierde su significado dentro de las dobles comillas. Sin embargo esto si sucede dentro de las comillas simples y el resultado en el tercer caso es mostrar por pantalla **$shell**. También pierden su signicado los caracteres especiales que pierden su significado en las dobles comillas, así como las dobles comillas.

### El caracter \

Este caracter sirve para quitar cualquier significado especial que tenga para la shell el caracter que va a continuación de \ . Por ejemplo: 

```
echo \$shell
```

como resultado muestra por pantalla **$shell**.

### Las comillas simples invertidas

El significado de las comillas invertidas es completamente diferente. Se emplea encerrando en estas comillas un comando y el resultado es ejecutar el comando insertando la salida en el punto de la línea de comandos donde se encuentra el comando. Por ejemplo: 

```
echo La fecha de hoy es `date`
man `ls`
```

En el primer caso se ejecuta el comando date insertando el resultado al final de la frase. El segundo dará probablemente error, pues tratará de buscar una página del manual de ayuda para comandos que se llamen como cada uno de los archivos del directorio. A no ser que algún archivo tenga nombre de comando, el resultado será que no encuentra la página del manual para cada nombre de archivo. Las comillas invertidas mantienen su significado dentro de las dobles comillas, pero no dentro de las comillas simples.

## Referencia

* [Introducción a Unix](https://research.iac.es/sieinvens/SINFIN/CursoUnix/index.php)
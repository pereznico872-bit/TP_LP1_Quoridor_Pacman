QUORIDOR PAC-MAN
Proyecto desarrollado en lenguaje C99 utilizando la librería gráfica raylib.
Autor:
Hugo Nicolás Pérez Rodríguez

1. DESCRIPCIÓN DEL PROYECTO

Este proyecto consiste en una versión jugable de Quoridor Pac-Man.
El juego enfrenta a Pac-Man contra cuatro fantasmas. Pac-Man debe comer las 4 pac-bolas del mapa antes de quedarse sin vidas.
Los fantasmas intentan atraparlo para hacerle perder sus 3 vidas.

El programa incluye:
* Pantalla de configuración.
* Modo IA vs Player.
* Modo Player vs Player.
* Tablero con tamaño configurable según el mapa.
* Pac-Man con 3 vidas.
* 4 fantasmas habilitables o deshabilitables.
* 4 pac-bolas.
* Muros permanentes del mapa.
* Muros temporales colocados durante la partida.
* Editor de mapas.
* Guardado y carga de mapas mediante archivo.
* Pantalla final con ganador y nivel alcanzado.
* Imagen personalizada de Pac-Man con la camiseta de Paraguay.


2. ARCHIVOS DEL PROYECTO

El proyecto incluye los siguientes archivos:
* quoridor_pacman.c
* pacman_paraguay.png
* README.txt

Opcionalmente, si se crea un mapa desde el editor, también se puede incluir:
* custom_map.txt

3. COMPILACIÓN Y EJECUCIÓN

El programa fue probado usando raylib en Windows con el entorno que trae Notepad++.
Pasos para ejecutar:
1. Abrir el archivo quoridor_pacman.c en el Notepad++ de raylib.
2. Presionar Fn + F6.
3. Seleccionar el script raylib_compile_execute.
4. Presionar OK.
5. Se abrirá la ventana del juego.

4. CONTROLES DEL MENÚ DE CONFIGURACIÓN

Flechas ARRIBA/ABAJO:
* Seleccionar una opción del menú.

Flechas IZQUIERDA/DERECHA o ESPACIO:
* Cambiar la opción seleccionada.

ENTER:
* Iniciar la partida.

TAB:
* Cambiar el mapa seleccionado.

E:
* Entrar al editor de mapas.

En el menú se pueden configurar:
* Modo de juego.
* Fantasmas habilitados o deshabilitados.
* Dificultad de cada fantasma.
* Cantidad de muros de Pac-Man.
* Cantidad de muros de los fantasmas.
* Vida de los muros temporales.
* Mapa a utilizar.

5. MODOS DE JUEGO

IA vs Player:
Un jugador controla a Pac-Man y la computadora controla a los fantasmas.

Player vs Player:
Un jugador controla a Pac-Man y otro jugador controla a los fantasmas.

En Player vs Player, los fantasmas no se mueven todos al mismo tiempo. El programa les da turno uno por uno:
1. Blinky
2. Inky
3. Pinky
4. Clyde
El nombre del fantasma actual aparece en pantalla.

6. CONTROLES DURANTE LA PARTIDA

Pac-Man:

Flechas:
* Mover a Pac-Man una casilla.

Click en el borde entre dos casillas:
* Colocar un muro temporal.

ENTER:
* Terminar el turno.

Fantasmas en modo Player vs Player:
W:
* Mover hacia arriba.

A:
* Mover hacia la izquierda.

S:
* Mover hacia abajo.

D:
* Mover hacia la derecha.

Click en el borde entre dos casillas:
* Colocar un muro temporal del equipo de fantasmas.

ENTER:
* Pasar el turno del fantasma actual.

Otros controles:

R:
* Reiniciar partida.

ESC:
* Volver al menú.

7. REGLAS PRINCIPALES DEL JUEGO

Pac-Man tiene 2 acciones por turno.

Cada acción puede ser:
* Moverse una casilla.
* Colocar un muro temporal.

Si Pac-Man come una pac-bola, gana una acción extra ese turno.
Pac-Man gana si come las 4 pac-bolas.
Los fantasmas ganan si atrapan a Pac-Man hasta dejarlo sin vidas.

Cundo un fantasma atrapa a Pac-Man:
* Pac-Man pierde una vida.
* Pac-Man vuelve a su posición inicial
* Los fantasmas vuelven a sus posiciones iniciales.
* Las pac-bolas comidas no vuelven a aparecer.
* Los muros temporales se mantienen en el tablero.

8. TIPOS DE MUROS

El juego utiliza tres tipos de muros:

Azul:
* Muro fijo del mapa.
* No desaparece.
* Forma parte del diseño del mapa.

Amarillo:
* Muro temporal colocado por Pac-Man.
* Tiene una cantidad limitada de turnos.
* Cuando su contador llega a cero, desaparece y vuelve a la mano de Pac-Man.

Rojo:
* Muro temporal colocado por los fantasmas.
* También tiene una duración limitada.
* Cuando su contador llega a cero, desaparece y vuelve a la mano de los fantasmas.

El número que aparece sobre un muro temporal indica cuántos turnos le quedan antes de desaparecer.

9. DIFICULTAD DE LOS FANTASMAS

La dificultad afecta solamente al modo IA vs Player.

Dificultad 1 - Ndahasyi:
El fantasma se mueve de forma aleatoria.

Dificultad 2 - Hasy:
El fantasma mezcla movimientos aleatorios con persecución.

Dificultad 3 - Hasyeterei:
El fantasma intenta acercarse mejor a Pac-Man.

Para perseguir a Pac-Man, la IA utiliza la distancia Manhattan, que calcula qué movimiento deja al fantasma más cerca de Pac-Man.

10. EDITOR DE MAPAS

El proyecto incluye un editor de mapas.

Controles del editor:

P:
* Seleccionar herramienta para colocar Pac-Man.

1, 2, 3, 4:
* Seleccionar y colocar cada fantasma.

B:
* Colocar pac-bolas.

M:
* Colocar muros permanentes.

Click:
* Colocar el elemento seleccionado.

S:
* Guardar el mapa como custom_map.txt.

+:
* Aumentar filas.

-:
* Disminuir filas.

[:
* Aumentar columnas.

]:
* Disminuir columnas.

ESC:
* Volver al menú.

El mapa guardado puede ser seleccionado luego desde la pantalla de configuración.

11. ESTRUCTURAS Y VARIABLES PRINCIPALES

El programa usa estructuras para organizar la información.

Pos:
Guarda una posición del tablero mediante fila y columna.

Map:
Guarda el tamaño del tablero, posiciones iniciales, pac-bolas y muros permanentes.

Ghost:
Guarda la información de cada fantasma, como posición, color, dificultad, estado habilitado y si está vivo.

TempWall:
Guarda los muros temporales, su posición, dueño, dirección y vida restante.

Game:
Guarda el estado general de la partida: Pac-Man, fantasmas, vidas, turnos, muros, pac-bolas y ganador.

Config:
Guarda la configuración elegida antes de iniciar la partida.

12. PERSONALIZACIÓN DEL PROYECTO

El proyecto fue personalizado con una temática paraguaya.
Se agregó una imagen de Pac-Man usando la camiseta de Paraguay en el menú principal. Además, el menú de configuración utiliza colores rojo, blanco y azul como referencia a Paraguay.

13. OBSERVACIÓN FINAL

El proyecto integra contenidos vistos en Lenguajes de Programación 1:
* Condicionales.
* Ciclos.
* Funciones.
* Arreglos.
* Matrices.
* Estructuras.
* Manejo de archivos.

El objetivo principal fue crear un juego funcional, entendible y defendible, aplicando conceptos básicos y medios del lenguaje C.
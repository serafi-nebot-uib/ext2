######## MIEMBROS DEL GRUPO ########

Serafí Nebot Ginard
Jaume Galmés Ramis
Ignasi Paredes Casasnovas




######## MODIFICACIONES Y MEJORAS ########

La modificación principal es la estructura de carpetas del proyecto y el makefile que se ha hecho principalmente para mantener el proyecto organizado a lo largo del desarrollo. El código fuente se encuentra dentro de la carpeta src. Los tests dentro de de la carpeta test (se han modificado para adaptarse a la estructura de carpetas y se puedan llamar desde la carpeta principal del proyecto). Los archivos temporales de compilación y los programas ejecutables se encuentran dentro de la carpeta build, generada y borrada automáticamente por el makefile. Además, dentro de la carpeta src, se han subdividido los archivos de código fuente entre las carpetas util (archivos con funciones de utilidad genérica; no son sólo útiles para este proyecto) y core (archivos que tienen la lógica propia del sistema de ficheros). Los programas que utilizan las funcionalidades de core no tienen subcarpeta.

También hemos modificado los mensajes de debug. Debido a que hemos ido añadiendo mensajes durante pruebas nuestras, ha llegado un punto en el que salían demasiados y no eran útiles. Filtrar-los por nivel de desarrollo (niveles semanales) no era útil porque a veces necesitábamos mensajes de varios niveles. Por este motivo, hemos decidido cambiar a un sistema de mensajes con niveles de "verbosidad", los mensajes más comunes/útiles se les asigna un nivel 1, los siguientes un nivel 2, etc. y basta con seleccionar el nivel de debug global con la macro DEBUG_LVL en el fichero logging.h.

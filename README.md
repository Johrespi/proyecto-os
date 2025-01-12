# Proyecto Sistemas Operativos

## Objetivos Específicos

1.	Diseñar una Cadena de Procesamiento (Pipeline), en la cual se apliquen las técnicas de IPC y de sincronización usando las herramientas de la librería estándar de C. Para representar su diseño deben usar el Diagrama de Actividades UML junto con el texto que describa su diseño. 
2.	Escribir el programa o el conjunto de programas que implementen la solución diseñada en el punto anterior y que use las técnicas de IPC y de sincronización entre procesos de la librería estándar de C.
3.	Realizar pruebas y recopilar los resultados para evaluar y verificar el correcto funcionamiento de la solución. 

## Especificaciones

La solución a construir debe tener los siguientes componentes: 
1. Un programa a manera de Publicador que lea una imagen de entrada, formato bmp, y la coloque en un recurso de memoria compartida. El programa debe solicitar recurrentemente al usuario la ruta donde se encuentra la imagen cargada.
2. Un segundo programa, que llamaremos Desenfocador, que lee cada imagen colocada en el recurso compartido y aplique una transformación de desonfoque. Debe usar el mismo procesamiento utilizado en el program assigment anterior (pa3), es decir, debe usar un kernel de desenfoque (o blur). El desenfoque debe ser aplicado únicamente a la primera mitad de la imagen original y el número de hilos debe ser configurable o definido como parámetro antes de lanzar este programa.
3. Un tercer programa, que llamaremos Realzador, que realice las mismas operaciones que el segundo, pero debe usar un kernel de realce de bordes (edge detection) y transformar la segunda mitad de la imagen.
La solución debe considerar los siguientes requerimientos mínimos de sincronización.
1. Cuando el Publicador haya finalizado de leer y cargar la imagen en el recurso compartido, se deben ejecutar, como dos procesos independientes los dos programas, el Desenfocador y el Realzador.  
2. Cuando hayan finalizado, el Desenfocador y el Realzador, se debe combinar las dos porciones procesadas de la imagen original en una sola imagen y grabar esta imagen en disco. La ruta en la que se debe grabar la imagen se debe definir mediante parámetro antes de lanzar a los programas.
3. Todos los programas deben ejecutarse de forma concurrente. 

## Especificaciones de las salidas
Todos los programas deberán ser ejecutados desde consola o desde la terminal y las salidas por pantalla deben demostrar el inicio y fin de cada etapa del procesamiento.  



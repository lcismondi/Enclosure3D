# Encoder Rotativo Ky-040 de 20 Pasos por vuelta

<img src="./Imagenes/Rotary%20encoder.jpg" width="40%">

Los códigos tradicionales lo que hacen es leer el estado de una señal (CLK) y cuando esta cambia de estado leen el estado de la otra línea (DT). Dependiendo del valor de esta última interpretan si el cursor se mueve en un sentido u el otro.
Esta técnica funciona siempre y cuando el encoder tenga una buena calidad, en el caso de los componentes económicos típicamente chinos, no es tan así, teniendo falsos resultados sin que el encoder haya completado un paso.
Normalmente implementado como el siguiente código:

```
  currentStateCLK = digitalRead(CLK); 
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) { 
  currentStateDT = digitalRead(DT); 
    if (currentStateDT != currentStateCLK) { 
      counter++; 
      currentDir = "CCW"; 
    } else { 
      counter--; 
      currentDir = "CW"; 
    } 
  } 
  lastStateCLK = currentStateCLK; 
```

La señal obtenida en condiciones normales ([código grey](https://es.wikipedia.org/wiki/C%C3%B3digo_Gray)) son las que se muestran más abajo. 
Si se prueba en un sistema aislado lo más probable es que funcione correctamente, pero al ir incorporando más funcionalidades al microcontrolador y que este tenga que atender a más tareas, van a comenzar a aparecer los errores.

<img src="./Imagenes/giro%20horario.png" width="100%">
<img src="./Imagenes/giro%20anti%20horario.png" width="100%">




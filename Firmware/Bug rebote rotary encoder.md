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
Si se prueba en un sistema aislado lo más probable es que funcione correctamente, pero al ir incorporando más funcionalidades al microcontrolador y que este tenga que atender a más tareas, van a comenzar a aparecer los errores. Los invito a analizar paso a paso todas las posibilidades teniendo como referencia el código anterior y las señales para encontrar los casos de posibles fallas.

**Señal con giro horario:**
<img src="./Imagenes/giro%20horario.png" width="100%">

**Señal con giro antihorario:**
<img src="./Imagenes/giro%20anti%20horario.png" width="100%">

## Error de secuencia
Existen casos en los que sí el potenciómetro no llega al próximo punto de giro y vuelve, esto genera un falso paso lo que puede provocar una lectura errónea. La señal que se obtiene en estos casos son las que están debajo:

**Error con giro horario:**
<img src="./Imagenes/error%20anti%20horario.png" width="100%">

**Señal con giro antihorario:**
<img src="./Imagenes/error%20horario.png" width="100%">

La solución es mirar el cambio de estado de ambas variables cuando la primera o una de ellas modifica su estado, esperar a que la segunda también lo haga, solo mientras la anterior se mantenga en su estado y dependiendo del sentido del cambio de la segunda será la dirección de giro.
Implementado se puede ver en el siguiente código:

```
  currentStateCLK = digitalRead(CLK);
  currentStateDT = digitalRead(DT);

  if (currentStateCLK == LOW && currentStateCLK != lastStateCLK) {
    flagCLK = HIGH;
    lastStateCLK = currentStateCLK;
  } else if (currentStateCLK == HIGH && currentStateCLK != lastStateCLK) {
    flagCLK = LOW;
    lastStateCLK = currentStateCLK;
  }

  if (flagCLK == HIGH) {
    if (currentStateDT == LOW && currentStateDT != lastStateDT) {
      counter++;
      currentDir = "CW";
      flagCLK = LOW;
    } else if (currentStateDT == HIGH && currentStateDT != lastStateDT) {
      counter--;
      currentDir = "CCW";
      flagCLK = LOW;
    }
  }
  else{
    lastStateDT = currentStateDT;
  }
```




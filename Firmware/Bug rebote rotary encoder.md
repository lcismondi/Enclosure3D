Encoder Rotativo Ky-040 de 20 Pasos por vuelta:

Los códigos tradicionales lo que hacen es leer el estado de una señal (CLK) y cuando esta cambia de estado leen el estado de la otra línea (DT). Dependiendo del valor de esta última interpretan si el cursor se mueve en un sentido u el otro.
Esta técnica funciona siempre y cuando el encoder tenga una buena calidad, en el caso de los componentes económicos típicamente chinos, no es tan así, teniendo falsos resultados sin que el encoder haya completado un paso.
Típicamente implementado como el siguiente código:

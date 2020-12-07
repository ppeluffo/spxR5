/*
 * main.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 *
 *  avrdude -v -Pusb -c avrispmkII -p x256a3 -F -e -U flash:w:spxR4.hex
 *  avrdude -v -Pusb -c avrispmkII -p x256a3 -F -e
 *
 *  REFERENCIA: /usr/lib/avr/include/avr/iox256a3b.h
 *
 *  El watchdog debe estar siempre prendido por fuses.
 *  FF 0A FD __ F5 D4
 *
 *  PROGRAMACION FUSES:
 *  /usr/bin/avrdude -px256a3b -cavrispmkII -Pusb -u -Uflash:w:spx.hex:a -Ufuse0:w:0xff:m -Ufuse1:w:0x0:m -Ufuse2:w:0xff:m -Ufuse4:w:0xff:m -Ufuse5:w:0xff:m
 *  /usr/bin/avrdude -px256a3b -cavrispmkII -Pusb -u -Ufuse0:w:0xff:m -Ufuse1:w:0x0:m -Ufuse2:w:0xff:m -Ufuse4:w:0xff:m -Ufuse5:w:0xff:m
 *
 *  /usr/bin/avrdude -px256a3b -cavrispmkII -Pusb -Uflash:w:/home/pablo/Spymovil/workspace/spxR5/Release/spxR5.hex:a
 *  /usr/bin/avrdude -p x256a3b -P /dev/ttyUSB0 -b 9600 -c avr109 -v -v -e -V -Uflash:w:/home/pablo/Spymovil/workspace/spxR5/Release/spxR5.hex
 *
 *  Para ver el uso de memoria usamos
 *  avr-nm -n spxR5.elf | more
 *  avr-nm -Crtd --size-sort spxR5.elf | grep -i ' [dbv] '
 *
 *  https://e2019.uy/buscar
 *  http://www.CABRERA/MONS/ENRIQUE/Montevideo/Padron/2354U
 *  Literatura,adscriptores
 *  Liceo 58 Mario Benedetti Fundado 1999
 *  18488618
 *  CMA 37384 515 111
 *  22923645
 *
 *	https://stackoverflow.com/questions/36884514/get-at-command-response
 *
 *
 * -Manual
 *  Test:Reintentos de mandar un SMS que falla.
 *  TEst: scan discover
 *  Test: comms gprs mas fluida ( sin caidas del socket)
 * -GUI
 *  Revisar en el servidor que grabe el UID en los inits. !!!
 * ------------------------------------------------------------------------
 * Version 3.0.6.a( MASTER ) @2020-12-02
 * Modficamos la parte de transmision de datos de modo de no usar el modo
 * transparente sino tener siempre el control de terminal del modem.
 * 1- Cambiamos el comando CIPMODE a CIPMODE=0
 * 2- Eliminamos pasar a modo comando
 * 3- Modifico los comandos gprs netopen/netclose/netstatus/linkopen/linkclose
 *    linkstatus.
 * 4- Modifico los comando xCOMMS_netopen/netclose/netstatus/linkopen/
 *    linkclose/linkstatus.
 *
 * ------------------------------------------------------------------------
 * Version 3.0.5.d( MASTER ) @2020-11-23
 * Modbus: Cambio la forma de transmitir de modo que no use interrupcion
 * sino byte a byte.
 * defino xnprintf_MBUS() y frtos_write_modbus.
 *
 * ------------------------------------------------------------------------
 * Version 3.0.5.b,c( MASTER ) @2020-11-23
 * Arreglo el problema que cuando arranca con los contadores midiendo se resetea.
 * Pongo una flag global que bloquea a contar (ISR) hasta que todo este
 * inicializado.
 * ------------------------------------------------------------------------
 * Version 3.0.5.a( MASTER ) @2020-10-26
 * - Agrego la aplicacion EXTERNAL_POLL.
 *
 * ------------------------------------------------------------------------
 * Version 3.0.4.c( MASTER ) @2020-10-23
 * - Otros buffers de hash resultan justos en ciertos casos particulares por
 *   lo que defino una variable global de 64 bits y lo uso en general para
 *   todos los calculos de hash.
 *
 * ------------------------------------------------------------------------
 * Version 3.0.4.b( MASTER ) @2020-10-07
 * - Corregimos el tamaño del buffer de hash base ya que quedaba muy justo.
 *
 * ------------------------------------------------------------------------
 * Version 3.0.4.b( MASTER ) @ 2020-09-01
 * - Incorporo MODBUS Master.
 *   - Rutinas de trasmision / recepcion por puerto AUX ( modo cmd )
 *   	Tarea de aux1_RX.
 *   - Manejo del RTS
 *   - Actualmente no se puede trasmitir un 0x00 por los puertos seriales ya que
 *   se toma como NULL. Debemos crear una funcion que trasnmita por el puerto AUX en
 *   modo transparente.
 *   Defino la funcion void frtos_putchar( file_descriptor_t fd ,const char cChar ).
 *
 *
 * ------------------------------------------------------------------------
 * Version 3.0.4.a( MASTER ) @ 2020-08-28
 * 1- Ajusto el protocolo para que en c/RX_OK de un frame me mande el CLOCK.
 *    Si el RTC esta desfasado mas de 60s lo ajusto
 * 2- En el SPX8, siempre mando el canal de la bateria.(igual que en los SPX5)
 * 3- Incremento la informacion de errores de I2C
 * ------------------------------------------------------------------------
 * Version 3.0.3.f( MASTER ) @ 2020-08-18
 * -Agrego la calibracion de los canales analogicos por 2 puntos.
 * -Agrego en el protocolo que el datalogger le mande al server la calibracion
 * -Ajusto un bug que no permitia configurar la MMIN en los canales analogicos
 *  que no fuese entero.
 * ------------------------------------------------------------------------
 * Version 3.0.3.c( MASTER ) @ 2020-08-05
 * - Ajustes menores en logs
 * - Ajustes menores en funciones
 * ------------------------------------------------------------------------
 * Version 3.0.3.b( MASTER ) @ 2020-07-30
 * En los printf controlo el largo con strlen() y no todo el tamanio del buffer
 * Las trasmisiones seriales las hago por poleo !! y no mas interrupcion.
 * Normalizo los comandos AT usando tablas de flash.
 *
 * ------------------------------------------------------------------------
 * Version 3.0.2.m ( MASTER ) @ 2020-07-27
 *
 * 1- Reescribo una FSM para el prender el modem.
 * 3- Revisar el reseteo del modem por dar error de configuracion
 * 4- Revisar manejo de errores en abrir el socket.
 * 5- Normalizo los mensajes de log para poder analizarlos con software
 * 6- Todos los comandos salen por OK/ERR o timeout.
 * 7- Agrego el comando AT_CBC para leer el voltaje del modem.
 * 8- Agrego el comando CPOF para apagar el modem 'soft'
 * 9- Agrego un comando AT que lo envio cuando un comando no responde. De este modo
 *    puedo deducir si el modem esta respondiendo o no
 * 10- Normalizo la forma de enviar todos los comandos y sus logs.
 * 11- Agrego el comando ATI para sustituir los comandos CGMM,CGMR, CGMI, IMEI.
 * 12- En NETOPEN se asigna la IP y la leo con IPADDR.
 *     Arreglo que cada vez que se usa el primero, si es ok se da el segundo asi siempre
 *     se que IP tengo.
 * 13- Modifico algunas funciones de gprs para que evaluen OK y ERROR primero.
 *     Luego en los strings que se evaluan, recorto las +
 * 14- Arreglo el comando +++ que al final no lleva /r.
 * 15- En gprs_opensocket y gprs_closesocket NO paso a modo comando. Lo hago en capas superiores.
 * 16- Elimino gprs_net_connect().
 * 17- tcpclose() terminaba en \n y no en \r.
 * 18- Elimino gprs_net_connect: el set apn lo paso a configurar y el netopen/ipaddress a
 *     process_frame.
 * 19- Agrego una funcion para determinar el estado del netopen().
 * 20- Reescribo la funcion FSM de procesar_frame.
 * 21- Pongo un #ifdef BETA_TEST para dejar los mesajes de debug solo en estas versiones
 * 22- Si NETOPEN da ERROR salgo enseguida.
 * 23- Reescribo el modulo de SCAN
 * 24- Reescribo el XCOMM_process_frame:
 *     Si un socket se cierra esperando una respuesta, no debo reabrirlo para esperar ya que el server
 *     no va a mandar mas la respuesta por dicho socket. Debo salir a reenviar el query !!.
 * 25- Modifico gprs_close_link para que primero chequee si ya esta cerrado y no dar mensaje de error.
 * 26- De acuerdo a TELIT, espero 50ms antes de c/comando nuevo.
 * 27- Cambio el MON_SQE para antes de CONFIGURAR ya que el tiempo que demora en registrarse en la red
 *     depende de la calidad de senal y si da error, tenemos entonces un parametro para evaluarlo.
 * 28- Sigo los flowchart de UBLOX para armar los estados.
 * 29- El comando AT+CIPTIMEOUT nos da los timeouts de NETOPEN / TCPOPEN.
 *     Por defecto son 120s.
 *     Los configuro en 60s.!!
 *
 * ------------------------------------------------------------------------
 * Version 3.0.2.l ( MASTER ) @ 2020-07-04
 * 1-Encontramos un datalogger FTEST03 reseteado a DEFAULT pero dando
 *   continuamente el mensaje: ERROR: Checksum sVarsComms failed: calc[0x1b], sto[0x82].
 *   Sin embargo al resetearlo por comando a default y resetearlo no da mas el error.
 *   El problema estaria en Loading defaults.
 *   Luego de cargar los defaults los salvo en la EE.
 * 2-Luego de hacer un scan correcto, salvo los parametros y me reseteo.
 *   Elimino setear el timerdial a 10s. Esto genera un loop con el punto 1.
 * 3-Agrego la definicion y control del pin DTR.,
 *   Defino gprs_switch_to_command_mode().
 * 4-RTS y CTS estaban intercambiados (input/output). Corregido.
 *   Agrego la funcion gprs_IFC() para mostrar el control de flujo configurado.
 *   Los modems estan por default sin control de flujo !!!.
 * 5-Al prender el modem espero el PB DONE que indica que termino el boot process.
 * 6-Agrego que al comenzar a configurar el modem muestro su identificacion y version.
 * 7-Mejoro la deteccion del imei y el ccid.
 * 8-Simplifico algunos modulos de tkComms.
 * 9-Modifico la rutina de gprs_net_connect() para tener c/paso bien diferenciado
 * 10-En gprs_netopen() agrego un NETCLOSE previo y cuando da error para resetear
 *    todas las condiciones del modem.
 * 11-Defino una FSM xCOMMS_process_frame() para mandar todo tipo de frames (init/data)
 *    que resuelve el tema de los intentos y los problemas de socket.
 * 12-Reescribo el modulo de initframe.c
 * 13-Reescribo el modulo de dataframe.c
 * 14-Se inicializan algunas variables en el calculo de hashes que se usaban en +=.
 * 15-Cambio la lectura del IMEI y CCID al modulo de configurar.
 *    Si no los puedo leer no apago y salgo sino que continuo.
 *
 * ------------------------------------------------------------------------
 * Version 3.0.2.k ( MASTER ) @ 2020-07-02
 * 1-ERR: En  frtos_uart_ioctl al borrar el TXbuffer estaba borrando el RXbuffer.
 * 2-WARN: En el frtos_io no uso los semaforos de los uart ya que los pase al printf asi que
 * los elimino.
 * 3-WARN: modifico xfputChar para que escriba directamente con frtos_write.
 * 4-WARN: agrego un control por semaforo a xnprint.
 * 5-Al prender el modem, si no puedo leer el imei o el ccid doy error y salgo.
 * 6-AT+IPADDR
 *   +IADDR: 10.204.2.106
 *   GPRS: ERROR: ip no asignada !!.
 *   El problema esta que en vez de responder IPADDR respondio IADDR !!!
 *
 * ------------------------------------------------------------------------
 * Version 3.0.2.j ( MASTER ) @ 2020-06-29
 * 1- En el scan, antes de ver si necesito hacerlo seteo las variables server_script
 * y esto hace que no use las que tengo configuradas.
 * 2- Al calcular los hases de configuracion, en alguno casos se puede llegar
 * a usar mas tamaño del buffer.
 * Se redimensionan los buffer y se controla el overflow.
 *------------------------------------------------------------------------
 * Version 3.0.2.i ( MASTER ) @ 2020-06-25
 * 1-Problema:
 * Vemos que hay momentos en que se transmiten mensajes vacios o cortados
 * y que ciertos comandos del GPRS quedan en blanco.
 * Solucion:
 * En las rutinas del driver de uart, al definir los ringbuffers poniamos
 * los txringbuffers size en RXSTORAGE_SIZE con lo cual en el caso del GPRS
 * el TXringbuffer definido estatico de 128 bytes quedaba con 512 bytes.
 * Esto hacia que al borrarlo con memset, se sobreescribieran mas posiciones
 * de memoria ( mem_r5_3_0_2_h.xlsx ).
 * En particular se le pasa por arriba a 'uart_gprs'.
 * 2- Elimino el comando gprs_CFGRI() porque no esta implementado en el modem.
 *
 *------------------------------------------------------------------------
 * Version 3.0.2.h ( MASTER ) @ 2020-06-24
 * 1- Con cierta combinacion de datos se procesa la senal de frames entre que
 * envio los datos y que llega la respuesta por lo que el dlg. sale de esperar
 * la respuesta y se pierde, por lo que se llena la memoria.
 * Sol: Modifico el procesamiento de las senales de modo que en st_dataframe ignoro
 * la senal de data_frame_ready.
 *------------------------------------------------------------------------
 * Version 3.0.2.g ( MASTER ) @ 2020-06-22
 * Se detectaron 3 problemas:
 * 1- Hay veces que en los frames de inits, se loguean como correctos pero
 *    al server SPY ( y apache ) llegan menos campos. ( Counters, psensor, etc)
 *    H) El buffer de printf es de 256b pero el del gprs_uart de 128b.
 *    Cuando saco los datos en el TERM no tengo problemas porque no hay control de flujo
 *    pero el modem si.
 *    Da la impresion que el modem queda sobreescrito. No pasa con DATAFRAME porque entre
 *    c/frame espero 250ms que se vacie
 *    Hago lo mismo en los frames de INIT. Agrego una espera para que los buffers se vacien.
 *    Al mandar un frame de INIT GLOBAL incoporo delays entre las partes del frame.
 *
 * 2- En los SCAN se va por timeout de comms.
 *    COMMS: GPRS_SCAN SCRIPT ERROR !!.
 *    Luego del TO comms, hace un load default y queda siempre con el error
 *    ERROR: Checksum sVarsComms failed: calc[0x83], sto[0x82].
 *    No puede recuperarse.
 *    El problema estaba en el server que en la funcion check_auth ponia los UID en 0 entonces
 *    luego no podia recuperarse.
 *
 * 3- Error de CPIN.
 *    Agrego que al dar errores de comunicaciones y esperar, luego de la espera
 *    resetea al micro.
 *------------------------------------------------------------------------
 * Version 3.0.2.f ( MASTER ) @ 2020-06-16
 * a) Incorporo un contador de errores de comunicaciones que se va incrementando
 * Luego de un limite, se resetea el micro.
 * Si logro llegar a inicializarme, reseteo el contador a 0.
 * b) En c/estado muestro el valor del contador junto al valor del gprs prendido/inicializado
 *
 *------------------------------------------------------------------------
 * Version 3.0.2.e ( MASTER ) @ 2020-06-15
 * a) En gprs_CPIN hago una espera progresiva aumentando el tiempo c/ciclo
 *
 * --------------------------------------------------------------------------------------------------
 * Version 3.0.2.d ( MASTER )
 * Seguimos viendo que luego de algún CPIN se cuelga el modem y ya aunque lo apague, luego al encenderlo
 * no responde al PIN.
 * En https://www.libelium.com/forum/viewtopic.php?t=21623 reportan algo parecido.
 * https://en.wikipedia.org/wiki/SIM_Application_Toolkit
 * Parece que el problema es que al enviar algun comando al SIM, este interactua con el STK (algun menu ) y lo bloquea.
 * Hasta no conocer bien como se hace lo dejamos sin usar.
 * " la tarjeta SIM es un ordenador diminuto con sistema operativo y programa propios.
 *   STK responde a comandos externos, por ejemplo, al presionar un botón del menú del operador,
 *   y hace que el teléfono ejecute ciertas acciones
 * "
 * https://www.techopedia.com/definition/30501/sim-toolkit-stk
 * http://www.bladox.cz/devel-docs/gen_stk.html
 * https://www.cooking-hacks.com/forum/viewtopic.php?f=20&t=3813
 *
 * El mensaje +STIN: 25 es un mensaje no solicitado que emite el PIN STK.
 *
 * a) Cambio chequeo del PIN (gprs_CPIN) y solo verifico que lo tenga instalado: READY.
 * b) Agrego en todos los estados del tkComms mostrar a la entrada / salida los valores de gprs prendido/inicializado
 * c) Los SMS los chequeo solo en aplicacion PLANTAPOT de modo de no interactuar con el SIM si no
 *    es necesario.
 * d) Hago la funcion gprs_SAT_set(). Se corre solo en modo comando.
 * e) Agrego un control en el formato del RTC recibido en el INIT.
 * f) En status, gprs state muestro el valor de gprs prendido/inicializado.
 *
 * --------------------------------------------------------------------------------------------------
 * Version 3.0.2.b ( MASTER )
 *
 * Detectamos 2 problemas:
 * 1-Si se queda en default no puede conectarse al servidor ya que da el mensaje
 * cmd>COMMS: gprs NETOPEN OK !.
 * COMMS: GPRS_SCAN SCRIPT ERROR !!.
 * Solucionado: Problemas del servidor
 *
 * 2- Hay veces que da COMMS: pin ERROR: Reconfiguro timerdial y no puede discar mas, pero
 * al resetearlo si puede.
 *
 * 3- En XCOMMS uso una variable dispositivo_prendido pero debo difereciar si es gprs o aux1.
 * Manejo XBEE o GPRS pero en realidad debo manejar GPRS y AUX1.
 *
 * ELIMINO TODO LO REFERENTE A XBEE EN EL MODULO DE COMMS y vuelvo a un sistema basico con solo gprs.
 * Agrego antes de CPIN ERROR un mensaje del estado de gprs_prendido / gprs_inicializado.
 *
 * Problema 1:
 * memory: rcdSize=62, wrPtr=27878,rdPtr=-23999,delPtr=-24004,r4wr=791,r4rd=228,r4del=5
 * Esto hace que comienzen a aparecer nan.
 * Se arregla controlando que los punteros no se salgan de los limites c/vez que cambian.

 * Problema 2:
 * No entra en tickless
 * El problema estaba en el hardware de la fuente del equipo de test.
 *
 * --------------------------------------------------------------------------------------------------
 *
 * Version 3.0.1c
 * Se presenta un bug que hace que los equipos no se conecten del todo y se reseteen al transmitir el
 * frame de init global.
 * - Incorporo mecanismos para monitorear el stack.( en la funcion main. Para leer la profundidad del
 * stack uso vTaskGetInfo porque devuelve el stack en 16 bits, en cambio la funcion uxTaskGetStackHighWaterMark()
 * esta desactualizada y lo devuelve en 8 bits con lo que hace rollover.
 * !! Confirmo que el stack no es el problema.
 * El problema estaba en que leian una configuración del servidor en que los contadores estaban
 * mal configurados y esto hacia que luego el micro se reseteara
 *
 * --------------------------------------------------------------------------------------------------
 * Version 3.0.1b @ 20200511
 * - Agrego a XCOMMS el manejo de un timer ( desde tkCTL ) para monitorear que el enlace xbee este activo
 *   El timer expira c/3 minutos si no recibio nada por el xbee.
 *   Apaga y prende el mdulo xbee.
 *   Cuando recibe un frame reinicia el timer.
 *   En XBEE trabajamos en modo continuo, con un timerpoll de 1 minuto.
 *
  * --------------------------------------------------------------------------------------------------
 * Version 3.0.1a @ 20200430
 * - Modifico los defines COUNTERS_TYPE_A por COUNTERS_HW_SIMPLE y COUNTERS_TYPE_B por COUNTERS_HW_OPTO
 * - Agregamos a los contadores la posibilidad de configurarlos que diparen por flanco de subida o bajada.
 *
 * --------------------------------------------------------------------------------------------------
 * Version 2.9.9u @ 20200328
 * - Cuando sale de INIT por timeout en modo CONTINUO, pasa al estado espera prendido. Al polear un nuevo frame,
 *   pasa a DATA y lo transmite pero antes debería haber completado el INIT.
 *   En modo DISCRETO no pasa porque siempre queda en apagado y luego vuelve a intentar el INIT.
 *
 *
 * --------------------------------------------------------------------------------------------------
 * Version 2.9.9l
 * Cambio el modelo de trabajo de las comunicaciones
 * Genero un branch en git llamado 'comms'.
 * 1-Modifico los nombres de xCom_printf por xfprintf en todas sus variantes.
 * 2-Defino una nueva funcion de printf que es un wrapper de xprintf_P
 *   int xfprintf_V( file_descriptor_t fd, const char *fmt, va_list argp )
 *
 * int xCOMMS_printf_P() se usa a nivel de aplicacion de usuario
 * int xGPRS_printf_P() y int xXBEE_printf_P() se usan a nivel de LLC en las funciones que
 * proveen servicios a la capa de aplicacion.
 *
 * --------------------------------------------------------------------------------------------------
 * Version 2.9.9.k
 * - Como en la aplicacion TANQUE se requiere de una lista de SMS, para ahorrar memoria la vamos
 * a tener en comun con la aplicacion de PLANTAPOT y por lo tanto la sacamos de la estructura de  tanques_conf
 * y alarmas_conf y la ponemos aparte de modo que queda compartida
 * - En la aplicacion ALARMAS cuando un canal esta deshabilitado transmito 99.
 * --------------------------------------------------------------------------------------------------
 * Version 2.9.9.d
 * - Revisar configuracion del PSENSOR desde el server. Que al apagarlo no quede en OFF !!

 * --------------------------------------------------------------------------------------------------
 *  Version 2.0.5 @ 20190812
 *  REVISAR:
 *  Semaforo de la FAT. ( xbee, gprs )
 *
 *  - BUG timerpoll: Al reconfigurarse on-line, si estaba en modo continuo, luego del init pasa al modo data
 *  y hasta que no caiga el enlace y pase por 'esperar_apagado', no va a releer los parametros.
 *  La solucion es mandar una señal de redial.
 *
 *  - Modifico la estructura de archivos fuentes para segmentar el manejo
 *  de las salidas como general, consignas, perforaciones y pilotos
 *  - Agrego las rutinas de regulacion por piloto.
 *  - Modifico el formato del frame de datos de modo de tener en c/intervalo cuantas
 *  operaciones se hicieron de apertura/cierre de valvulas.
 *  - Como la lectura de las presiones las hago desde mas de una tarea, incorporo un
 *  semaforo.
 *  - Agrego a la estructura io5 el valor de VA/B_cnt para trasmitirlo y ver como opera
 *  el piloto.
 *  - Corrigo un bug en la trasmision de los INITS en el modulo del rangemeter
 *  - Agrego que los canales digitales tienen un timerpoll y si es > 0 los poleo.
 *
 *
 *  Version 2.0.5 @ 20190702
 *  - Inicializo todas las variables en todos los modulos.
 *  - Borro mensajes de DEBUG de modulo de contadores.
 *  - En tkData y tkGprs_data  pasamos las variables df, dr, fat a modo global
 *    de modo de inicializarlas y sacarlas del stack de c/tarea.
 *  - Cuando timerDial < 900 no lo pongo en 0 para que no se reconfigure c/vez
 *    que se conecta.
 *  - Implemento la funcion peek y poke
 *  - Agrego la funcion ICAL que calibra la corriente en 4 y en 20mA para generar
 *    correcciones en las instalaciones. ( solo por cmdmode ).
 *
 *  Version 2.0.0 @ 20190604
 *  -Contadores:
 *   Implementamos que espere un ancho minimo y un periodo minimo ( filtro )
 *   Si el nombre del canal es 'X' no debe interrumpir.
 *   Al configurarlo, prendo o apago la flag de enable.
 *  -En GPRS_init agrego un comando de reset DEFAULT=(NONE,SPY,OSE,UTE)
 *  -Elimino los canales digitales como timers.
 *
 # Revisar CMD read ach / data_read_frame
 # Revisar configuracion desde el servidor de paramentros que son NULl ( contadores)
 #

 *  Version 1.0.6 @ 20190524
 *  Agrego las modificaciones del driver y librerias de I2C que usamos en spxR3_IO8
 *  Agrego las modificaciones del manejo de las salidas
 *  Agrego la funcion u_gprs_modem_link_up() para saber cuando hay enlace en el control de las salidas
 *  Modifico las tareas de los contadores agregado una para c/u. Al quitar los latches la polaridad
 *  de reposo cambia ( ver consumos ).
 *  El SPX_IO8 solo implementa PERFORACIONES. El SPX_IO5 implementa todos.
 *  * Ver en el servidor que cuando no tenga un canal digital, lo mande apagarse si el datalogger lo manda
 *
 *  Version 1.0.5 @ 20190517
 *  - Corregimos que en el init mande el dbm en vez del csq
 *
 *  Version 1.0.4 @ 20190510
 *  Correcciones del testing efectuado por Yosniel.
 *  - No se calcula el caudal, solo se multiplica la cantidad de pulsos por el magpp:
 *    Efectivamente es asi. El valor del contador solo se multiplica por magpp de modo que si queremos
 *    medir pulsos, magpp = 1.
 *    Para medir otra magnitud ( volumen o caudal ) debemos usar el magpp adecuado.
 *  - Los 12 voltios se mantienen constantes todo el tiempo, sin embargo, si pongo un timer poll  alto (600)
 *    Se corrige para que en las placas IO5CH se apaguen luego de c/poleo.
 *  - En DRV8814_set_consigna cambio el orden de apertura y cierre para que las valvulas no
 *    queden a contrapresion
 *
 *  Version 1.0.3 @ 20190425
 *  Modifico el frame de scan para que envie el DLGID.
 *  Paso los inits de las tareas a funciones publicas que las corro en tkCTL.
 *  En tkCTL utilizo un semaforo solo para el WDG porque sino hay bloqueos que hacen
 *  que el micro demore mas en estado activo.
 *  Modifico el loop de tkCTL para que si no hay terminal conectada no pase por
 *  ninguna fucion con lo que bajo el nivel activo.
 *
 *  Version 1.0.1 @ 20190424
 *  Cambio el nombre del parametro de la bateria de BAT a bt para ser coherente con el server
 *  En tCtl, cada 5s solo reviso el watchdog y los timers y luego c/60s el resto.
 *  En u_control_string controlo que el largo sea > 0.
 *  No prende los sensores.!!!!. Faltaba inicializar el pin de 12V_CTL
 *
 *
 *  Version 0.0.6.R2 @ 20190405
 *  El tamanio del io5 es de 58 bytes y el del io8 es de 56 bytes.
 *  Al formar una union en un dataframe, el tamanio usado es de 58 bytes que con los 7 bytes del rtc
 *  quedan en 65 bytes que excede el tamanio del registro.
 *  El maximo deben ser 62 bytes para usar el 63 de checksum y el 64 de write tag.
 *  Esto hace que para los datos queden: 64 - 1(wirteTag) -1(checksum) - 7(rtc) = 55 bytes
 *  1) Las entradas digitales en io5 las hago int8_t o sea que quedan 36 bytes. OK
 *  2) Las entradas digitales de io8 hago 4 de uint8_t y 4 de uint16_t ( digital timers ) con lo que queda en 52 bytes. OK
 *  Cambio el manejo del timerDial.
 *
 *  Version 0.0.2.R1 @ 20190311
 *  - Modifico la tarea de GPRS para poder hacer un scan de los APN.
 *    Saco de gprs_configurar el paso de configurar el APN en gprs_ask_ip.
 *  # Considerar que el dlgId no es DEFAULT pero no esta en la BD ( mal configurado por operador )
 *  # Ver respuesta a IPSCAN donde el server no tiene el UID en la BD ( NOT_ALLOWED ? )
 *  - Cambio en pwrsave el modo por un bool pwrs_enabled.
 *
 *  Version 0.0.1.R1 @ 20190218
 *  - Las funciones de manejo de la NVM son las tomadas del AVR1605 y usadas en spxboot.
 *  - Mejoro las funciones de grabar el DLGID para que pongan un \0 al final del string.
 *  - En reset default el dlgid queda en DEFAULT
 *
 */

#include "spx.h"

//------------------------------------------------------------------------------------
int main( void )
{
	// Leo la causa del reset para trasmitirla en el init.
	wdg_resetCause = RST.STATUS;
	RST.STATUS = wdg_resetCause;
	//RST_PORF_bm | RST_EXTRF_bm | RST_BORF_bm | RST_WDRF_bm | RST_PDIRF_bm | RST_SRF_bm | RST_SDRF_bm;

	// Clock principal del sistema
	u_configure_systemMainClock();
	u_configure_RTC32();
	sysTicks = 0;

	// Configuramos y habilitamos el watchdog a 8s.
	WDT_EnableAndSetTimeout(  WDT_PER_8KCLK_gc );
	if ( WDT_IsWindowModeEnabled() )
		WDT_DisableWindowMode();

	set_sleep_mode(SLEEP_MODE_PWR_SAVE);

	initMCU();

	// Inicializacion de los devices del frtos-io
//	if ( BAUD_PIN_115200() ) {
//		frtos_open(fdTERM, 115200 );
//		frtos_open(fdTERM, 9600 );
//	} else {
		frtos_open(fdTERM, 9600 );
//	}

	frtos_open(fdGPRS, 115200);
	frtos_open(fdAUX1, 9600);		// Usado por xbee o modbus o camara
	frtos_open(fdI2C, 100 );

	// Creo los semaforos
	sem_SYSVars = xSemaphoreCreateMutexStatic( &SYSVARS_xMutexBuffer );
	sem_WDGS = xSemaphoreCreateMutexStatic( &WDGS_xMutexBuffer );

	xprintf_init();
	FAT_init();

	startTask = false;

	dinputs_setup();
	counters_setup();

	// Creamos las tareas
	xHandle_tkCtl = xTaskCreateStatic(tkCtl, "CTL", tkCtl_STACK_SIZE, (void *)1, tkCtl_TASK_PRIORITY, xTask_Ctl_Buffer, &xTask_Ctl_Buffer_Ptr );
	xHandle_tkCmd = xTaskCreateStatic(tkCmd, "CMD", tkCmd_STACK_SIZE, (void *)1, tkCmd_TASK_PRIORITY, xTask_Cmd_Buffer, &xTask_Cmd_Buffer_Ptr );
	xHandle_tkInputs = xTaskCreateStatic(tkInputs, "IN", tkInputs_STACK_SIZE, (void *)1, tkInputs_TASK_PRIORITY, xTask_Inputs_Buffer, &xTask_Inputs_Buffer_Ptr );
	xHandle_tkAplicacion = xTaskCreateStatic(tkAplicacion, "APP", tkAplicacion_STACK_SIZE,(void *)1, tkAplicacion_TASK_PRIORITY,xTask_Aplicacion_Buffer, &xTask_Aplicacion_Buffer_Ptr );
	xHandle_tkComms = xTaskCreateStatic(tkComms, "COMMS", tkComms_STACK_SIZE, (void *)1, tkComms_TASK_PRIORITY, xTask_Comms_Buffer, &xTask_Comms_Buffer_Ptr );
	xHandle_tkCommsRX = xTaskCreateStatic(tkCommsRX, "RX", tkCommsRX_STACK_SIZE, (void *)1, tkCommsRX_TASK_PRIORITY, xTask_CommsRX_Buffer, &xTask_CommsRX_Buffer_Ptr );
	xHandle_tkAuxRX = xTaskCreateStatic(tkAuxRX, "AUX", tkAuxRX_STACK_SIZE, (void *)1, tkAuxRX_TASK_PRIORITY, xTask_AuxRX_Buffer, &xTask_AuxRX_Buffer_Ptr );

	/* Arranco el RTOS. */
	vTaskStartScheduler();

	// En caso de panico, aqui terminamos.
	exit (1);

}
//------------------------------------------------------------------------------------
void vApplicationIdleHook( void )
{
	// Como trabajo en modo tickless no entro mas en modo sleep aqui.
//	if ( sleepFlag == true ) {
//		sleep_mode();
//	}
}
//------------------------------------------------------------------------------------
void vApplicationTickHook( void )
{
	sysTicks++;

}
//------------------------------------------------------------------------------------
// Define the function that is called by portSUPPRESS_TICKS_AND_SLEEP().
//------------------------------------------------------------------------------------
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	// Es invocada si en algun context switch se detecta un stack corrompido !!
	// Cuando el sistema este estable la removemos.
	// En FreeRTOSConfig.h debemos habilitar
	// #define configCHECK_FOR_STACK_OVERFLOW          2

	xprintf_P( PSTR("PANIC:%s !!\r\n\0"),pcTaskName);

}
//------------------------------------------------------------------------------------
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
//------------------------------------------------------------------------------------
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
//------------------------------------------------------------------------------------
// FUNCIONES PARA CONTROLAR EL STACK DE C/TAREA
//------------------------------------------------------------------------------------
void debug_full_print_stack_watermarks(void)
{
uint16_t free_stack_size;
uint16_t stack_size;
uint16_t used_stack_size;
TaskStatus_t xTaskDetails;

	// tkIdle
	stack_size = configMINIMAL_STACK_SIZE;
	vTaskGetInfo(xHandle_idle, &xTaskDetails, pdTRUE, eInvalid );
	free_stack_size = xTaskDetails.usStackHighWaterMark;
	used_stack_size = stack_size - free_stack_size;
	xprintf_P( PSTR("IDLE:size=%d,free=%d,used=%d\r\n\0"), stack_size, free_stack_size, used_stack_size);

	// tkControl
	stack_size = tkCtl_STACK_SIZE;
	vTaskGetInfo(xHandle_tkCtl, &xTaskDetails, pdTRUE, eInvalid );
	free_stack_size = xTaskDetails.usStackHighWaterMark;
	used_stack_size = stack_size - free_stack_size;
	xprintf_P( PSTR("CTL:size=%d,free=%d,used=%d\r\n\0"), stack_size, free_stack_size, used_stack_size);

	// tkCmd
	stack_size = tkCmd_STACK_SIZE;
	vTaskGetInfo(xHandle_tkCmd, &xTaskDetails, pdTRUE, eInvalid );
	free_stack_size = xTaskDetails.usStackHighWaterMark;
	used_stack_size = stack_size - free_stack_size;
	xprintf_P( PSTR("CMD:size=%d,free=%d,used=%d\r\n\0"), stack_size, free_stack_size, used_stack_size);

	// tkData
	stack_size = tkInputs_STACK_SIZE;
	vTaskGetInfo(xHandle_tkInputs, &xTaskDetails, pdTRUE, eInvalid );
	free_stack_size = xTaskDetails.usStackHighWaterMark;
	used_stack_size = stack_size - free_stack_size;
	xprintf_P( PSTR("IN:size=%d,free=%d,used=%d\r\n\0"), stack_size, free_stack_size, used_stack_size);

	// tkComms
	stack_size = tkComms_STACK_SIZE;
	vTaskGetInfo(xHandle_tkComms, &xTaskDetails, pdTRUE, eInvalid );
	free_stack_size = xTaskDetails.usStackHighWaterMark;
	used_stack_size = stack_size - free_stack_size;
	xprintf_P( PSTR("COMMS:size=%d,free=%d,used=%d\r\n\0"), stack_size, free_stack_size, used_stack_size);

	// tkRx
	stack_size = tkCommsRX_STACK_SIZE;
	vTaskGetInfo(xHandle_tkCommsRX, &xTaskDetails, pdTRUE, eInvalid );
	free_stack_size = xTaskDetails.usStackHighWaterMark;
	used_stack_size = stack_size - free_stack_size;
	xprintf_P( PSTR("RX:size=%d,free=%d,used=%d\r\n\0"), stack_size, free_stack_size, used_stack_size);

	// tkApp
	stack_size = tkAplicacion_STACK_SIZE;
	vTaskGetInfo(xHandle_tkAplicacion, &xTaskDetails, pdTRUE, eInvalid );
	free_stack_size = xTaskDetails.usStackHighWaterMark;
	used_stack_size = stack_size - free_stack_size;
	xprintf_P( PSTR("APP:size=%d,free=%d,used=%d\r\n\0"), stack_size, free_stack_size, used_stack_size);
	xprintf_P( PSTR("\r\n\0"));


}
//------------------------------------------------------------------------------------
void debug_print_stack_watermarks(char *id)
{
	// Monitorea cada 5s cuanto han alcanzado las watermarks de los stacks de c/tarea.
	// Guarda el minimo.
	// Si en algun caso se supera el minimo, se imprime.

	//portENTER_CRITICAL();

st_stack_size_t stack_wmk;

	debug_read_stack_watermarks(&stack_wmk);

	xprintf_P(PSTR("STACK MONITOR(%s): idle[%d],ctl[%d],cmd[%d],in[%d],comms[%d],rx[%d],app[%d]\r\n\0"),id,stack_wmk.idle,stack_wmk.ctl,stack_wmk.cmd,stack_wmk.in,stack_wmk.comms,stack_wmk.rx,stack_wmk.app);
	xprintf_P( PSTR("\r\n\0"));

	//portEXIT_CRITICAL();

}
//------------------------------------------------------------------------------------
void debug_read_stack_watermarks(st_stack_size_t *stack_wmk )
{

TaskStatus_t xTaskDetails;

	// tkIdle
	vTaskGetInfo(xHandle_idle, &xTaskDetails, pdTRUE, eInvalid );
	stack_wmk->idle = xTaskDetails.usStackHighWaterMark;

	// tkControl
	vTaskGetInfo(xHandle_tkCtl, &xTaskDetails, pdTRUE, eInvalid );
	stack_wmk->ctl = xTaskDetails.usStackHighWaterMark;

	// tkCmd
	vTaskGetInfo(xHandle_tkCmd, &xTaskDetails, pdTRUE, eInvalid );
	stack_wmk->cmd = xTaskDetails.usStackHighWaterMark;

	// tkData
	vTaskGetInfo(xHandle_tkInputs, &xTaskDetails, pdTRUE, eInvalid );
	stack_wmk->in = xTaskDetails.usStackHighWaterMark;

	// tkComms
	vTaskGetInfo(xHandle_tkComms, &xTaskDetails, pdTRUE, eInvalid );
	stack_wmk->comms = xTaskDetails.usStackHighWaterMark;

	// tkRx
	vTaskGetInfo(xHandle_tkCommsRX, &xTaskDetails, pdTRUE, eInvalid );
	stack_wmk->rx = xTaskDetails.usStackHighWaterMark;

	// tkApp
	vTaskGetInfo(xHandle_tkAplicacion, &xTaskDetails, pdTRUE, eInvalid );
	stack_wmk->app = xTaskDetails.usStackHighWaterMark;

}
//------------------------------------------------------------------------------------
void debug_monitor_stack_watermarks(char *id)
{
	// Monitorea cada 5s cuanto han alcanzado las watermarks de los stacks de c/tarea.
	// Guarda el minimo.
	// Si en algun caso se supera el minimo, se imprime.

	//portENTER_CRITICAL();

st_stack_size_t stack_wmk;
bool print_min_stack_flag = false;

st_stack_size_t stack_wmk_min;
static bool init = true;

	if (init == true) {
		init = false;
		stack_wmk_min.app = 512;
		stack_wmk_min.cmd = 512;
		stack_wmk_min.comms = 512;
		stack_wmk_min.ctl = 512;
		stack_wmk_min.idle = 512;
		stack_wmk_min.in = 512;
		stack_wmk_min.rx = 512;
	}

	debug_read_stack_watermarks(&stack_wmk);

	if ( stack_wmk.app < stack_wmk_min.app ) {
		stack_wmk_min.app = stack_wmk.app;
		print_min_stack_flag = true;
	}
	if ( stack_wmk.cmd < stack_wmk_min.cmd ) {
		stack_wmk_min.cmd = stack_wmk.cmd;
		print_min_stack_flag = true;
	}
	if ( stack_wmk.comms < stack_wmk_min.comms ) {
		stack_wmk_min.comms = stack_wmk.comms;
		print_min_stack_flag = true;
	}
	if ( stack_wmk.ctl < stack_wmk_min.ctl ) {
		stack_wmk_min.ctl = stack_wmk.ctl;
		print_min_stack_flag = true;
	}
	if ( stack_wmk.idle < stack_wmk_min.idle ) {
		stack_wmk_min.idle = stack_wmk.idle;
		print_min_stack_flag = true;
	}
	if ( stack_wmk.in < stack_wmk_min.in ) {
		stack_wmk_min.in = stack_wmk.in;
		print_min_stack_flag = true;
	}
	if ( stack_wmk.rx < stack_wmk_min.rx ) {
		stack_wmk_min.rx = stack_wmk.rx;
		print_min_stack_flag = true;
	}

	if ( print_min_stack_flag ) {
		xprintf_P(PSTR("STACK MONITOR(%s): app[%d],cmd[%d],comms[%d],ctl[%d],idle[%d],in[%d],rx[%d]\r\n\0"),id, stack_wmk.app,stack_wmk.cmd,stack_wmk.comms,stack_wmk.ctl,stack_wmk.idle,stack_wmk.in,stack_wmk.rx );
	}

	//portEXIT_CRITICAL();

}
//------------------------------------------------------------------------------------
int debug_freeRam(void)
{
	// https://jeelabs.org/2011/05/22/atmega-memory-use/
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
//------------------------------------------------------------------------------------


